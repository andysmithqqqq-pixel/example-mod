#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <functional>
#include <vector>
#include <string>
#include <cmath>

using namespace geode::prelude;

static constexpr float kBlock = 30.f;
static constexpr float kFlyBlocks = 23.f;
static constexpr float kLieSeconds = 12.f;
static constexpr float kHitRadius = 70.f;
static constexpr float kGravity = 900.f;
static constexpr float kLaunchVy = 420.f;
static constexpr float kGhostScale = 0.45f;

static bool isPaused() {
    auto sc = CCDirector::get()->getRunningScene();
    if (!sc || !sc->getChildren()) return false;
    for (auto c : CCArrayExt<CCNode*>(sc->getChildren())) {
        if (c && typeinfo_cast<PauseLayer*>(c)) return true;
    }
    return false;
}

static void collectRemote(CCNode* node, std::vector<CCNode*>& out, int depth) {
    if (!node || depth > 4) return;
    auto kids = node->getChildren();
    if (!kids) return;
    for (auto c : CCArrayExt<CCNode*>(kids)) {
        if (!c) continue;
        if (typeinfo_cast<CCSpriteBatchNode*>(c) || typeinfo_cast<GameObject*>(c)) continue;
        std::string name = geode::cocos::getObjectName(c);
        if (name.find("RemotePlayer") != std::string::npos
            || name.find("ComplexVisualPlayer") != std::string::npos) {
            if (c->isVisible()) out.push_back(c);
            continue;
        }
        collectRemote(c, out, depth + 1);
    }
}

static CCNode* makeHand() {
    auto root = CCNode::create();
    auto d = CCDrawNode::create();
    ccColor4F skin = {1.f, 0.80f, 0.35f, 1.f};
    ccColor4F edge = {0.35f, 0.2f, 0.05f, 1.f};
    CCPoint palm[4] = {ccp(-10.f, 0.f), ccp(10.f, 0.f), ccp(10.f, 16.f), ccp(-10.f, 16.f)};
    d->drawPolygon(palm, 4, skin, 1.f, edge);
    float h[4] = {10.f, 12.f, 11.f, 8.f};
    for (int i = 0; i < 4; i++) {
        float x = -10.f + i * 5.f;
        CCPoint f[4] = {ccp(x, 16.f), ccp(x + 4.2f, 16.f),
                        ccp(x + 4.2f, 16.f + h[i]), ccp(x, 16.f + h[i])};
        d->drawPolygon(f, 4, skin, 1.f, edge);
    }
    CCPoint th[4] = {ccp(-10.f, 3.f), ccp(-18.f, 10.f), ccp(-15.f, 14.f), ccp(-10.f, 10.f)};
    d->drawPolygon(th, 4, skin, 1.f, edge);
    root->addChild(d);
    return root;
}

static void playSlapSound() {
    auto fmod = FMODAudioEngine::sharedEngine();
    if (fmod) fmod->playEffect("explode_11.ogg", 2.2f, 1.f, 0.8f);
}

// Draggable on-screen button
class SlapButton : public CCLayer {
protected:
    CCNode* m_spr = nullptr;
    CCPoint m_grab = ccp(0.f, 0.f);
    CCPoint m_last = ccp(0.f, 0.f);
    float m_total = 0.f;
    bool m_drag = false;
    std::function<void()> m_cb;

    bool init(std::function<void()> cb) {
        if (!CCLayer::init()) return false;
        m_cb = std::move(cb);
        auto win = CCDirector::get()->getWinSize();
        auto spr = ButtonSprite::create("SLAP");
        float x = Mod::get()->getSavedValue<float>("btn-x", win.width / 2);
        float y = Mod::get()->getSavedValue<float>("btn-y", win.height / 2);
        spr->setPosition({x, y});
        this->addChild(spr);
        m_spr = spr;
        this->setTouchEnabled(true);
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -510, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!this->isVisible() || isPaused()) return false;
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_spr->boundingBox().containsPoint(p)) return false;
        m_drag = true;
        m_total = 0.f;
        m_last = p;
        m_grab = m_spr->getPosition() - p;
        m_spr->setScale(0.9f);
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (!m_drag) return;
        auto p = this->convertToNodeSpace(t->getLocation());
        m_total += ccpDistance(p, m_last);
        m_last = p;
        if (m_total > 10.f) {
            auto win = CCDirector::get()->getWinSize();
            auto np = p + m_grab;
            np.x = std::max(0.f, std::min(win.width, np.x));
            np.y = std::max(0.f, std::min(win.height, np.y));
            m_spr->setPosition(np);
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_drag) return;
        m_drag = false;
        m_spr->setScale(1.f);
        if (m_total > 10.f) {
            Mod::get()->setSavedValue<float>("btn-x", m_spr->getPositionX());
            Mod::get()->setSavedValue<float>("btn-y", m_spr->getPositionY());
        } else if (m_cb) {
            m_cb();
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_drag = false;
        if (m_spr) m_spr->setScale(1.f);
    }

public:
    static SlapButton* create(std::function<void()> cb) {
        auto r = new SlapButton();
        if (r && r->init(std::move(cb))) {
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }
};

struct Ghost {
    Ref<CCNode> real;
    Ref<SimplePlayer> sp;
    CCPoint vel = ccp(0.f, 0.f);
    float av = 0.f;
    float dir = 1.f;
    float groundY = 0.f;
    bool landed = false;
    float lie = 0.f;
};

class SlapController : public CCNode {
protected:
    PlayLayer* m_pl = nullptr;
    CCNode* m_hand = nullptr;
    SlapButton* m_btn = nullptr;
    bool m_on = false;
    bool m_swinging = false;
    float m_facing = 1.f;
    float m_lastX = 0.f;
    std::vector<Ghost> m_ghosts;

    bool init(PlayLayer* pl) {
        if (!CCNode::init()) return false;
        m_pl = pl;
        auto win = CCDirector::get()->getWinSize();
        m_on = Mod::get()->getSavedValue<bool>("enabled", false);

        m_hand = makeHand();
        m_hand->setScale(1.2f);
        m_hand->setVisible(false);
        pl->m_objectLayer->addChild(m_hand, 1999);

        m_btn = SlapButton::create([this]() { this->slap(); });
        this->addChild(m_btn, 5);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->setTouchPriority(-505);
        this->addChild(menu, 6);

        auto tog = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(SlapController::onToggle), 0.5f
        );
        tog->setPosition({win.width - 24.f, 24.f});
        tog->toggle(m_on);
        menu->addChild(tog);

        auto lbl = CCLabelBMFont::create("SLAP", "bigFont.fnt");
        lbl->setScale(0.3f);
        lbl->setPosition({win.width - 54.f, 24.f});
        this->addChild(lbl, 6);

        if (pl->m_player1) m_lastX = pl->m_player1->getPositionX();
        this->schedule(schedule_selector(SlapController::tick));
        return true;
    }

    void onToggle(CCObject* s) {
        m_on = !static_cast<CCMenuItemToggler*>(s)->isToggled();
        Mod::get()->setSavedValue<bool>("enabled", m_on);
    }

    void tick(float dt) {
        if (!m_pl || !m_pl->m_player1) return;
        auto p1 = m_pl->m_player1;

        float x = p1->getPositionX();
        if (fabsf(x - m_lastX) > 0.05f) m_facing = (x > m_lastX) ? 1.f : -1.f;
        m_lastX = x;

        bool show = m_on && !p1->m_isDead;
        m_hand->setVisible(show);
        m_btn->setVisible(m_on);

        if (show && p1->getParent() && m_hand->getParent()) {
            auto wp = p1->getParent()->convertToWorldSpace(p1->getPosition());
            auto lp = m_hand->getParent()->convertToNodeSpace(wp);
            m_hand->setPosition(lp + ccp(m_facing * 22.f, -6.f));
            m_hand->setScaleX(m_facing * 1.2f);
            m_hand->setScaleY(1.2f);
            if (!m_swinging) m_hand->setRotation(m_facing * -15.f);
        }

        updateGhosts(dt);
    }

    void updateGhosts(float dt) {
        for (auto it = m_ghosts.begin(); it != m_ghosts.end();) {
            auto& g = *it;
            if (g.real) g.real->setVisible(false);
            if (!g.landed) {
                g.vel.y -= kGravity * dt;
                auto p = g.sp->getPosition();
                p.x += g.vel.x * dt;
                p.y += g.vel.y * dt;
                if (p.y <= g.groundY && g.vel.y < 0.f) {
                    p.y = g.groundY;
                    g.landed = true;
                    g.sp->setRotation(g.dir * 90.f);
                } else {
                    g.sp->setRotation(g.sp->getRotation() + g.av * dt);
                }
                g.sp->setPosition(p);
            } else {
                g.lie += dt;
                if (g.lie >= kLieSeconds) {
                    if (g.real) g.real->setVisible(true);
                    g.sp->removeFromParentAndCleanup(true);
                    it = m_ghosts.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    void slap() {
        if (!m_on || m_swinging || !m_pl || !m_pl->m_player1) return;
        m_swinging = true;
        float f = m_facing;
        m_hand->stopAllActions();
        m_hand->runAction(CCSequence::create(
            CCRotateTo::create(0.07f, f * -45.f),
            CCEaseIn::create(CCRotateTo::create(0.06f, f * 110.f), 2.f),
            CCCallFunc::create(this, callfunc_selector(SlapController::doHit)),
            CCDelayTime::create(0.06f),
            CCEaseOut::create(CCRotateTo::create(0.18f, f * -15.f), 2.f),
            CCCallFunc::create(this, callfunc_selector(SlapController::swingDone)),
            nullptr
        ));
    }

    void swingDone() { m_swinging = false; }

    bool alreadyFlying(CCNode* n) {
        for (auto& g : m_ghosts) if (g.real == n) return true;
        return false;
    }

    void doHit() {
        playSlapSound();
        auto p1 = m_pl->m_player1;
        if (!p1 || !p1->getParent()) return;
        auto me = p1->getParent()->convertToWorldSpace(p1->getPosition());
        auto tip = me + ccp(m_facing * 40.f, 0.f);

        std::vector<CCNode*> found;
        collectRemote(m_pl, found, 0);
        for (auto c : found) {
            if (!c->getParent() || alreadyFlying(c)) continue;
            auto tw = c->getParent()->convertToWorldSpace(c->getPosition());
            if (ccpDistance(tw, tip) > kHitRadius) continue;
            float dir = (tw.x >= me.x) ? 1.f : -1.f;
            launch(c, tw, dir);
        }
    }

    void launch(CCNode* target, CCPoint world, float dir) {
        auto layer = m_pl->m_objectLayer;
        auto lp = layer->convertToNodeSpace(world);

        auto gm = GameManager::get();
        auto sp = SimplePlayer::create(gm->getPlayerFrame());
        sp->setColor(gm->colorForIdx(gm->getPlayerColor()));
        sp->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        sp->setScale(kGhostScale);
        sp->setPosition(lp);
        layer->addChild(sp, 2000);

        float flight = 2.f * kLaunchVy / kGravity;
        Ghost g;
        g.real = target;
        g.sp = sp;
        g.dir = dir;
        g.vel = ccp(dir * (kFlyBlocks * kBlock) / flight, kLaunchVy);
        g.av = dir * 720.f;
        g.groundY = lp.y;
        m_ghosts.push_back(std::move(g));
        target->setVisible(false);

        auto lbl = CCLabelBMFont::create("SLAP!", "bigFont.fnt");
        lbl->setScale(0.45f);
        lbl->setColor({255, 60, 60});
        lbl->setPosition(lp + ccp(0.f, 30.f));
        layer->addChild(lbl, 2001);
        lbl->runAction(CCSequence::create(
            CCSpawn::create(CCMoveBy::create(0.6f, ccp(0.f, 30.f)), CCFadeOut::create(0.6f), nullptr),
            CCRemoveSelf::create(),
            nullptr
        ));
    }

public:
    static SlapController* create(PlayLayer* pl) {
        auto r = new SlapController();
        if (r && r->init(pl)) {
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }
};

class $modify(SlapPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (auto c = SlapController::create(this)) this->addChild(c, 9999);
    }
};
