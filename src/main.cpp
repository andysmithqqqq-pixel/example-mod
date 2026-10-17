#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/file.hpp>

using namespace geode::prelude;

struct BanConfig {
    bool disabled = false;
    std::string title = "Banned";
    std::string message =
        "You have been banned for <cr>30 days</c>.\n\n"
        "Reason: <cy>cheating</c>";
};

static BanConfig loadConfig() {
    BanConfig cfg;
    auto path = Mod::get()->getConfigDir() / "ban_screen.json";

    if (!std::filesystem::exists(path)) {
        (void)file::writeString(path, R"({
  "disable_screen": false,
  "title": "Banned",
  "message": "You have been banned for <cr>30 days</c>.\n\nReason: <cy>cheating</c>"
})");
        return cfg;
    }

    auto res = file::readJson(path);
    if (!res) return cfg;

    auto json = res.unwrap();
    cfg.disabled = json["disable_screen"].asBool().unwrapOr(false);
    cfg.title = json["title"].asString().unwrapOr(cfg.title);
    cfg.message = json["message"].asString().unwrapOr(cfg.message);
    return cfg;
}

class BanLayer : public CCLayerColor {
protected:
    bool init(BanConfig const& cfg) {
        if (!CCLayerColor::initWithColor({0, 0, 0, 150})) return false;

        auto winSize = CCDirector::get()->getWinSize();
        CCPoint center = ccp(winSize.width / 2, winSize.height / 2);

        auto bg = CCScale9Sprite::create("GJ_square01.png", {0.f, 0.f, 80.f, 80.f});
        bg->setContentSize({360.f, 200.f});
        bg->setPosition(center);
        this->addChild(bg);

        auto title = CCLabelBMFont::create(cfg.title.c_str(), "goldFont.fnt");
        title->setScale(1.0f);
        title->setPosition(center + ccp(0.f, 72.f));
        this->addChild(title);

        auto text = TextArea::create(
            cfg.message, "chatFont.fnt", 1.f, 320.f, {0.5f, 0.5f}, 22.f, false
        );
        text->setPosition(center - ccp(0.f, 12.f));
        this->addChild(text);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -9999, true);
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }

    void keyBackClicked() override {}

public:
    static BanLayer* create(BanConfig const& cfg) {
        auto ret = new BanLayer();
        if (ret && ret->init(cfg)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(BanMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        static bool shown = false;
        if (shown) return true;
        shown = true;

        auto cfg = loadConfig();
        if (cfg.disabled) return true;

        if (auto layer = BanLayer::create(cfg)) {
            layer->setZOrder(10000);
            SceneManager::get()->keepAcrossScenes(layer);
        }
        return true;
    }
};
