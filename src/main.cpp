#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>

static bool g_noclip = false;
static bool g_flight = false;
static bool g_verify = false;
static bool g_noCopy = false;
static bool g_gun = false;
static bool g_pearl = false;
static bool g_rocket = false;
static bool g_penis = false;

static bool g_panelOpen = false;

class AdminPanel;

class FloatingButton : public CCNode {
protected:
    CCMenuItemSpriteExtra* m_button = nullptr;

    bool init() {
        if (!CCNode::init()) return false;

        auto spr = CCSprite::create("GJ_plusBtn_001.png");
        if (!spr) spr = CCSprite::create("GJ_infoIcon_001.png");
        if (!spr) return false;

        spr->setScale(0.65f);

        auto item = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(FloatingButton::onPress)
        );

        auto menu = CCMenu::create();
        menu->addChild(item);
        menu->setPosition(CCPointZero);
        this->addChild(menu);

        m_button = item;
        this->setContentSize({45.f, 45.f});

        auto win = CCDirector::get()->getWinSize();
        float x = Mod::get()->getSavedValue<float>("panel-x", 35.f);
        float y = Mod::get()->getSavedValue<float>("panel-y", win.height / 2);

        this->setPosition({x, y});
        this->setTouchEnabled(true);

        return true;
    }

    void onPress(CCObject*) {
        if (g_panelOpen) return;
        g_panelOpen = true;
        if (auto scene = CCDirector::get()->getRunningScene()) {
            scene->addChild(AdminPanel::create(), 100000);
        }
    }

public:
    static FloatingButton* create() {
        auto ret = new FloatingButton();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class AdminPanel : public FLAlertLayer {
protected:
    CCMenu* m_menu = nullptr;

    bool init() {
        if (!FLAlertLayer::init(120.f)) return false;

        this->m_noElasticity = true;
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::get()->getWinSize();

        auto bg = cocos2d::extension::CCScale9Sprite::create(
            "GJ_square01.png"
        );
        bg->setContentSize({300.f, 310.f});
        bg->setPosition(win / 2);
        this->addChild(bg);

        auto title = CCLabelBMFont::create("ADMIN PANEL", "bigFont.fnt");
        title->setScale(0.55f);
        title->setPosition({win.width / 2, win.height / 2 + 135.f});
        this->addChild(title);

        m_menu = CCMenu::create();
        m_menu->setPosition(win / 2);
        this->addChild(m_menu);

        addToggle("NoClip", g_noclip, 100.f, menu_selector(AdminPanel::onNoClip));
        addToggle("Flight", g_flight, 72.f, menu_selector(AdminPanel::onFlight));
        addToggle("No C Mark", g_noCopy, 44.f, menu_selector(AdminPanel::onNoCopy));
        addToggle("Verify Hack", g_verify, 16.f, menu_selector(AdminPanel::onVerify));
        addToggle("Gun", g_gun, -12.f, menu_selector(AdminPanel::onGun));
        addToggle("Pearl", g_pearl, -40.f, menu_selector(AdminPanel::onPearl));
        addToggle("Rocket", g_rocket, -68.f, menu_selector(AdminPanel::onRocket));
        addToggle("Penis", g_penis, -96.f, menu_selector(AdminPanel::onPenis));

        addButton("Open Dev", -124.f, menu_selector(AdminPanel::onDev));
        addButton("Change Stats", -152.f, menu_selector(AdminPanel::onStats));

        auto close = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
            this,
            menu_selector(AdminPanel::onClose)
        );
        close->setPosition({145.f, 145.f});
        m_menu->addChild(close);

        return true;
    }

    void addToggle(
        const char* name,
        bool value,
        float y,
        SEL_MenuHandler handler
    ) {
        auto label = CCLabelBMFont::create(name, "bigFont.fnt");
        label->setScale(0.35f);

        auto item = CCMenuItemToggler::createWithStandardSprites(
            this, handler, 0.5f
        );

        item->toggle(value);

        auto row = CCMenu::create();
        row->addChild(item);
        row->setPosition({-75.f, y});
        m_menu->addChild(row);

        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({-55.f, y});
        m_menu->addChild(label);
    }

    void addButton(
        const char* name,
        float y,
        SEL_MenuHandler handler
    ) {
        auto label = CCLabelBMFont::create(name, "goldFont.fnt");
        label->setScale(0.38f);

        auto item = CCMenuItemSpriteExtra::create(
            label, this, handler
        );

        item->setPosition({0.f, y});
        m_menu->addChild(item);
    }

    void onNoClip(CCObject* s) {
        g_noclip = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onFlight(CCObject* s) {
        g_flight = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onNoCopy(CCObject* s) {
        g_noCopy = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onVerify(CCObject* s) {
        g_verify = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onGun(CCObject* s) {
        g_gun = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onPearl(CCObject* s) {
        g_pearl = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onRocket(CCObject* s) {
        g_rocket = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onPenis(CCObject* s) {
        g_penis = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    }

    void onDev(CCObject*) {
        this->keyBackClicked();
        geode::openSettingsPopup(Mod::get());
    }

    void onStats(CCObject*) {
        FLAlertLayer::create(
            "Change Stats",
            "Use the game's account settings to manage your statistics.",
            "OK"
        )->show();
    }

    void onClose(CCObject*) {
        this->keyBackClicked();
    }

    void keyBackClicked() override {
        g_panelOpen = false;
        this->removeFromParentAndCleanup(true);
    }

public:
    static AdminPanel* create() {
        auto ret = new AdminPanel();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class AdminPanelManager : public CCNode {
protected:
    bool init() {
        if (!CCNode::init()) return false;
        if (auto btn = FloatingButton::create()) {
            this->addChild(btn);
        }
        return true;
    }

public:
    static AdminPanelManager* create() {
        auto ret = new AdminPanelManager();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(AdminMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (auto manager = AdminPanelManager::create()) {
            this->addChild(manager, 99999);
        }
        return true;
    }
};

class $modify(AdminPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        if (auto manager = AdminPanelManager::create()) {
            this->addChild(manager, 99999);
        }

        return true;
    }
};

class $modify(AdminPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        if (g_flight && !m_isDead) {
            if (this->m_isPlatformer) {
                this->m_yVelocity = 0.f;
            } else {
                this->m_yVelocity = 0.f;
            }
        }

        if (g_noclip) {
            this->m_isCollidingWithObject = false;
        }
    }
};
