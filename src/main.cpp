#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <random>
#include <cmath>

using namespace geode::prelude;

static float frand(float a, float b) {
    static std::mt19937 gen{std::random_device{}()};
    return std::uniform_real_distribution<float>(a, b)(gen);
}

struct Effect {
    std::string name, desc;
    int cat;
    float speed = 1.f, rot = 0.f, zoom = 1.f;
    float flipX = 1.f, flipY = 1.f, playerScale = 1.f;
    ccColor4B tint = {0, 0, 0, 0};
    bool hidePlayer = false, hideUI = false, chaos = false;
    bool oneLife = false, shake = false, disco = false, blink = false;

    Effect(std::string n, std::string d, int c)
        : name(std::move(n)), desc(std::move(d)), cat(c) {}

    Effect& Speed(float v) { speed = v; return *this; }
    Effect& Rot(float v) { rot = v; return *this; }
    Effect& Zoom(float v) { zoom = v; return *this; }
    Effect& FlipX() { flipX = -1.f; return *this; }
    Effect& FlipY() { flipY = -1.f; return *this; }
    Effect& Player(float v) { playerScale = v; return *this; }
    Effect& Tint(int r, int g, int b, int a) { tint = ccc4(r, g, b, a); return *this; }
    Effect& HidePlayer() { hidePlayer = true; return *this; }
    Effect& HideUI() { hideUI = true; return *this; }
    Effect& Chaos() { chaos = true; return *this; }
    Effect& OneLife() { oneLife = true; return *this; }
    Effect& Shake() { shake = true; return *this; }
    Effect& Disco() { disco = true; return *this; }
    Effect& Blink() { blink = true; return *this; }
};

static char const* kCatName[6] = {"SPEED", "SCREEN", "PLAYER", "COLOR", "CHAOS", "LUCKY"};
static ccColor3B kCatColor[6] = {
    {60, 200, 90}, {60, 130, 230}, {240, 200, 40},
    {170, 80, 220}, {230, 60, 60}, {90, 220, 220}
};

static std::vector<Effect> const& effects() {
    static std::vector<Effect> v;
    if (!v.empty()) return v;

    auto add = [&](char const* n, char const* d, int c) -> Effect& {
        v.emplace_back(n, d, c);
        return v.back();
    };

    // SPEED
    add("Snail", "The game crawls at 0.3x speed.", 0).Speed(0.3f);
    add("Slow Motion", "The game runs at half speed.", 0).Speed(0.5f);
    add("Chill", "A relaxed 0.75x speed.", 0).Speed(0.75f);
    add("Quick", "A bit faster: 1.25x speed.", 0).Speed(1.25f);
    add("Fast", "The game runs at 1.5x speed.", 0).Speed(1.5f);
    add("Turbo", "Double speed.", 0).Speed(2.f);
    add("Hyper", "2.5x speed. Good luck.", 0).Speed(2.5f);
    add("Ludicrous", "Triple speed.", 0).Speed(3.f);
    add("Speed Roulette", "Speed changes randomly every few seconds.", 0).Chaos();
    add("Rubber Band", "Slower base speed, random changes.", 0).Speed(0.7f).Chaos();

    // SCREEN
    add("Upside Down", "The whole screen is flipped.", 1).Rot(180.f);
    add("Sideways Left", "The screen is turned left.", 1).Rot(90.f);
    add("Sideways Right", "The screen is turned right.", 1).Rot(-90.f);
    add("Tilt Left", "The screen is tilted left.", 1).Rot(-15.f);
    add("Tilt Right", "The screen is tilted right.", 1).Rot(15.f);
    add("Mirror", "Left and right are swapped.", 1).FlipX();
    add("Flip Vertical", "Up and down are swapped.", 1).FlipY();
    add("Double Flip", "Mirrored both ways.", 1).FlipX().FlipY();
    add("Zoom In", "The camera is closer.", 1).Zoom(1.3f);
    add("Close Up", "The camera is very close.", 1).Zoom(1.6f);
    add("Zoom Out", "The camera is further away.", 1).Zoom(0.75f);
    add("Tiny View", "Everything is small.", 1).Zoom(0.5f);
    add("Earthquake", "The screen shakes.", 1).Shake();
    add("No UI", "The interface is hidden.", 1).HideUI();

    // PLAYER
    add("Big Icon", "Your icon is 1.5x bigger.", 2).Player(1.5f);
    add("Giant", "Your icon is double size.", 2).Player(2.f);
    add("Colossus", "Your icon is triple size.", 2).Player(3.f);
    add("Small Icon", "Your icon is a bit smaller.", 2).Player(0.7f);
    add("Tiny", "Your icon is half size.", 2).Player(0.5f);
    add("Micro", "Your icon is very tiny.", 2).Player(0.3f);
    add("Invisible", "You cannot see your icon.", 2).HidePlayer();
    add("Giant Slowpoke", "Giant icon, slower game.", 2).Player(2.f).Speed(0.7f);
    add("Tiny Rocket", "Tiny icon, faster game.", 2).Player(0.5f).Speed(1.5f);
    add("Ghost Mode", "No icon and no interface.", 2).HidePlayer().HideUI();

    // COLOR
    add("Red Alert", "The screen is tinted red.", 3).Tint(255, 0, 0, 70);
    add("Ocean", "The screen is tinted blue.", 3).Tint(0, 110, 255, 70);
    add("Toxic", "The screen is tinted green.", 3).Tint(0, 255, 60, 60);
    add("Sunset", "The screen is tinted orange.", 3).Tint(255, 140, 0, 70);
    add("Pink Haze", "The screen is tinted pink.", 3).Tint(255, 0, 200, 70);
    add("Night", "It gets dark.", 3).Tint(0, 0, 0, 150);
    add("Deep Night", "It gets very dark.", 3).Tint(0, 0, 0, 200);
    add("Fog", "A white fog covers the screen.", 3).Tint(255, 255, 255, 120);
    add("Whiteout", "Almost everything is white.", 3).Tint(255, 255, 255, 170);
    add("Gold Rush", "The screen is tinted gold.", 3).Tint(255, 215, 0, 70);
    add("Disco", "The colors keep changing.", 3).Tint(255, 0, 0, 70).Disco();
    add("Strobe", "White flashes. Careful with your eyes.", 3).Tint(255, 255, 255, 160).Blink();
    add("Red Strobe", "Red flashes.", 3).Tint(255, 0, 0, 150).Blink();

    // CHAOS
    add("Drunk Pilot", "Shaking, tilted and unstable speed.", 4).Shake().Rot(5.f).Chaos();
    add("Disco Fever", "Changing colors and faster game.", 4).Tint(255, 0, 0, 70).Disco().Speed(1.3f);
    add("Nightmare", "Dark and too close.", 4).Tint(0, 0, 0, 180).Zoom(1.3f);
    add("Total Chaos", "Everything at once.", 4).Shake().Chaos().Disco().Tint(255, 0, 0, 60).Rot(10.f);
    add("Mirror Madness", "Mirrored screen, random speed.", 4).FlipX().Chaos();
    add("Vertigo", "Upside down and shaking.", 4).Rot(180.f).Shake();
    add("Blackout", "Almost no light, slower game.", 4).Tint(0, 0, 0, 230).Speed(0.7f);
    add("One Life", "Any death throws you out of the level.", 4).OneLife();
    add("One Life Turbo", "One life at 1.5x speed.", 4).OneLife().Speed(1.5f);
    add("One Life Invisible", "One life and no icon.", 4).OneLife().HidePlayer();
    add("Earthquake Rush", "Shaking screen, faster game.", 4).Shake().Speed(1.5f);
    add("Funhouse", "Mirrored, tilted and zoomed.", 4).FlipX().Rot(8.f).Zoom(1.2f);

    // LUCKY
    add("Clean Run", "Nothing happens. Enjoy.", 5);
    add("Lucky Charm", "No effect this time.", 5);
    add("Practice Pace", "Slightly slower with a wider view.", 5).Speed(0.8f).Zoom(0.85f);
    add("Zen", "Slow and calm with a soft green tint.", 5).Speed(0.6f).Tint(0, 255, 120, 40);
    add("Golden Hour", "A bit slower with a golden glow.", 5).Speed(0.9f).Tint(255, 215, 0, 40);
    add("Jackpot", "Half speed, wide view, golden glow.", 5).Speed(0.5f).Zoom(0.8f).Tint(255, 215, 0, 50);

    return v;
}

static int s_current = -1;
static float s_chaosLeft = 0.f;
static float s_chaosMul = 1.f;

static Effect const* current() {
    if (!PlayLayer::get()) return nullptr;
    auto& list = effects();
    if (s_current < 0 || s_current >= static_cast<int>(list.size())) return nullptr;
    return &list[s_current];
}

class $modify(WheelScheduler, CCScheduler) {
    void update(float dt) {
        float mul = 1.f;
        if (auto e = current()) {
            mul = e->speed;
            if (e->chaos) {
                s_chaosLeft -= dt;
                if (s_chaosLeft <= 0.f) {
                    s_chaosLeft = frand(1.5f, 4.f);
                    s_chaosMul = frand(0.5f, 2.f);
                }
                mul *= s_chaosMul;
            }
        }
        CCScheduler::update(dt * mul);
    }
};

class $modify(WheelPlayLayer, PlayLayer) {
    struct Fields {
        bool quitting = false;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        s_current = -1;
        int idx = Mod::get()->getSavedValue<int>("active", -1);
        auto& list = effects();
        if (idx < 0 || idx >= static_cast<int>(list.size())) return;

        Mod::get()->setSavedValue<int>("active", -1);
        s_current = idx;
        auto const& e = list[idx];

        auto win = CCDirector::get()->getWinSize();

        bool transform = e.rot != 0.f || e.zoom != 1.f || e.flipX != 1.f
            || e.flipY != 1.f || e.shake;
        if (transform) {
            this->ignoreAnchorPointForPosition(false);
            this->setAnchorPoint({0.5f, 0.5f});
            this->setPosition({win.width / 2, win.height / 2});
            this->setRotation(e.rot);
            this->setScaleX(e.zoom * e.flipX);
            this->setScaleY(e.zoom * e.flipY);
        }
        if (e.shake) {
            this->schedule(schedule_selector(WheelPlayLayer::shakeTick));
        }

        if (e.tint.a > 0) {
            auto overlay = CCLayerColor::create(e.tint, win.width * 3, win.height * 3);
            overlay->setPosition({-win.width, -win.height});
            this->addChild(overlay, 1000);

            if (e.disco) {
                overlay->runAction(CCRepeatForever::create(CCSequence::create(
                    CCTintTo::create(0.6f, 255, 0, 0),
                    CCTintTo::create(0.6f, 0, 255, 0),
                    CCTintTo::create(0.6f, 0, 0, 255),
                    nullptr
                )));
            }
            if (e.blink) {
                overlay->runAction(CCRepeatForever::create(CCSequence::create(
                    CCFadeTo::create(0.12f, e.tint.a),
                    CCFadeTo::create(0.12f, 0),
                    nullptr
                )));
            }
        }

        if (e.hideUI && m_uiLayer) m_uiLayer->setVisible(false);

        Notification::create("Effect: " + e.name, NotificationIcon::None, 2.f)->show();
    }

    void shakeTick(float) {
        auto e = current();
        if (!e || !e->shake) return;
        auto win = CCDirector::get()->getWinSize();
        this->setPosition({win.width / 2 + frand(-5.f, 5.f), win.height / 2 + frand(-5.f, 5.f)});
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto e = current();
        if (!e) return;

        if (e->hidePlayer) {
            if (m_player1) m_player1->setVisible(false);
            if (m_player2) m_player2->setVisible(false);
        }
        if (e->playerScale != 1.f) {
            if (m_player1) m_player1->setScale(e->playerScale);
            if (m_player2) m_player2->setScale(e->playerScale);
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        auto e = current();
        if (e && e->oneLife && !m_fields->quitting) {
            m_fields->quitting = true;
            this->scheduleOnce(schedule_selector(WheelPlayLayer::quitNow), 1.3f);
        }
    }

    void quitNow(float) {
        this->onQuit();
    }

    void onQuit() {
        s_current = -1;
        PlayLayer::onQuit();
    }
};

class WheelLayer : public CCLayerColor {
protected:
    CCNode* m_wheel = nullptr;
    CCLabelBMFont* m_status = nullptr;
    bool m_spinning = false;
    int m_sector = 0;

    bool init() {
        if (!CCLayerColor::initWithColor({0, 0, 0, 160})) return false;

        auto win = CCDirector::get()->getWinSize();
        CCPoint c = ccp(win.width / 2, win.height / 2);
        float R = 90.f;

        auto bg = CCScale9Sprite::create("GJ_square01.png", {0.f, 0.f, 80.f, 80.f});
        bg->setContentSize({420.f, 300.f});
        bg->setPosition(c);
        this->addChild(bg);

        auto title = CCLabelBMFont::create("Wheel of Fortune", "goldFont.fnt");
        title->setScale(0.8f);
        title->setPosition(c + ccp(0.f, 132.f));
        this->addChild(title);

        // Wheel
        m_wheel = CCNode::create();
        m_wheel->setPosition(c + ccp(0.f, 14.f));
        this->addChild(m_wheel);

        auto draw = CCDrawNode::create();
        m_wheel->addChild(draw);
        int n = 12;
        float step = 360.f / n;
        for (int i = 0; i < n; i++) {
            std::vector<CCPoint> pts;
            pts.push_back(ccp(0.f, 0.f));
            for (int k = 0; k <= 10; k++) {
                float a = (i * step + step * k / 10.f) * 3.14159265f / 180.f;
                pts.push_back(ccp(sinf(a) * R, cosf(a) * R));
            }
            auto col = kCatColor[i % 6];
            ccColor4F fill = {col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.f};
            ccColor4F border = {0.f, 0.f, 0.f, 1.f};
            draw->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, 1.5f, border);
        }
        // Outer ring
        std::vector<CCPoint> ring;
        for (int k = 0; k < 72; k++) {
            float a = k * 5.f * 3.14159265f / 180.f;
            ring.push_back(ccp(sinf(a) * R, cosf(a) * R));
        }
        draw->drawPolygon(ring.data(), 72, {0.f, 0.f, 0.f, 0.f}, 3.f, {1.f, 1.f, 1.f, 1.f});

        for (int i = 0; i < n; i++) {
            float mid = i * step + step / 2.f;
            float rad = mid * 3.14159265f / 180.f;
            auto lbl = CCLabelBMFont::create(kCatName[i % 6], "bigFont.fnt");
            lbl->setScale(0.32f);
            lbl->setPosition({sinf(rad) * R * 0.62f, cosf(rad) * R * 0.62f});
            lbl->setRotation(mid - 90.f + (mid > 180.f ? 180.f : 0.f));
            m_wheel->addChild(lbl);
        }

        // Fixed hub and pointer
        auto fixed = CCDrawNode::create();
        fixed->setPosition(c + ccp(0.f, 14.f));
        this->addChild(fixed);
        std::vector<CCPoint> hub;
        for (int k = 0; k < 24; k++) {
            float a = k * 15.f * 3.14159265f / 180.f;
            hub.push_back(ccp(sinf(a) * 9.f, cosf(a) * 9.f));
        }
        fixed->drawPolygon(hub.data(), 24, {1.f, 0.85f, 0.2f, 1.f}, 2.f, {0.f, 0.f, 0.f, 1.f});
        CCPoint tri[3] = {ccp(-10.f, R + 14.f), ccp(10.f, R + 14.f), ccp(0.f, R - 6.f)};
        fixed->drawPolygon(tri, 3, {1.f, 1.f, 1.f, 1.f}, 2.f, {0.f, 0.f, 0.f, 1.f});

        // Status
        m_status = CCLabelBMFont::create("", "chatFont.fnt");
        m_status->setPosition(c + ccp(0.f, -92.f));
        this->addChild(m_status);
        updateStatus();

        // Buttons
        auto menu = CCMenu::create();
        menu->setPosition(c);
        menu->setTouchPriority(-405);
        this->addChild(menu);

        auto spinSpr = ButtonSprite::create("SPIN");
        spinSpr->setScale(0.8f);
        auto spinBtn = CCMenuItemSpriteExtra::create(spinSpr, this, menu_selector(WheelLayer::onSpin));
        spinBtn->setPosition({0.f, -122.f});
        menu->addChild(spinBtn);

        auto closeSpr = ButtonSprite::create("Close");
        closeSpr->setScale(0.55f);
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(WheelLayer::onClose));
        closeBtn->setPosition({-160.f, -122.f});
        menu->addChild(closeBtn);

        auto clearSpr = ButtonSprite::create("Clear");
        clearSpr->setScale(0.55f);
        auto clearBtn = CCMenuItemSpriteExtra::create(clearSpr, this, menu_selector(WheelLayer::onClear));
        clearBtn->setPosition({160.f, -122.f});
        menu->addChild(clearBtn);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);
        return true;
    }

    void updateStatus() {
        int idx = Mod::get()->getSavedValue<int>("active", -1);
        auto& list = effects();
        std::string t = (idx >= 0 && idx < static_cast<int>(list.size()))
            ? "Next level: " + list[idx].name
            : "No effect queued";
        m_status->setString(t.c_str());
    }

    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -400, true);
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }

    void keyBackClicked() override {
        if (!m_spinning) this->removeFromParentAndCleanup(true);
    }

    void onClose(CCObject*) {
        if (!m_spinning) this->removeFromParentAndCleanup(true);
    }

    void onClear(CCObject*) {
        if (m_spinning) return;
        Mod::get()->setSavedValue<int>("active", -1);
        updateStatus();
    }

    void onSpin(CCObject*) {
        if (m_spinning) return;
        m_spinning = true;

        m_sector = static_cast<int>(frand(0.f, 11.999f));
        float jitter = frand(-10.f, 10.f);

        float cur = fmodf(m_wheel->getRotation(), 360.f);
        if (cur < 0.f) cur += 360.f;
        float target = fmodf(360.f - (m_sector * 30.f + 15.f + jitter) + 720.f, 360.f);
        float delta = 360.f * 6.f + fmodf(target - cur + 720.f, 360.f);

        m_wheel->runAction(CCSequence::create(
            CCEaseOut::create(CCRotateBy::create(5.f, delta), 3.f),
            CCCallFunc::create(this, callfunc_selector(WheelLayer::onDone)),
            nullptr
        ));
    }

    void onDone() {
        m_spinning = false;

        auto& list = effects();
        std::vector<int> pool;
        for (int i = 0; i < static_cast<int>(list.size()); i++) {
            if (list[i].cat == m_sector % 6) pool.push_back(i);
        }
        if (pool.empty()) return;
        int idx = pool[static_cast<size_t>(frand(0.f, static_cast<float>(pool.size()) - 0.001f))];

        Mod::get()->setSavedValue<int>("active", idx);
        updateStatus();

        std::string body = list[idx].desc + "\n\n<cy>Applies to your next level.</c>";
        FLAlertLayer::create(nullptr, list[idx].name.c_str(), body, "OK", nullptr, 320.f)->show();
    }

public:
    static WheelLayer* create() {
        auto ret = new WheelLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(WheelMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto spr = ButtonSprite::create("Wheel");
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(WheelMenuLayer::onWheel)
        );
        btn->setID("wheel-button"_spr);
        menu->addChild(btn);
        menu->updateLayout();
        return true;
    }

    void onWheel(CCObject*) {
        this->addChild(WheelLayer::create(), 10000);
    }
};
