#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>
#include <Geode/utils/file.hpp>

using namespace geode::prelude;

constexpr int BAN_TAG = 98431;

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

class $modify(BanAlert, FLAlertLayer) {
    void onBtn1(CCObject* sender) {
        if (this->getTag() == BAN_TAG) return;
        FLAlertLayer::onBtn1(sender);
    }

    void keyBackClicked() {
        if (this->getTag() == BAN_TAG) return;
        FLAlertLayer::keyBackClicked();
    }
};

class $modify(BanMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto cfg = loadConfig();
        if (cfg.disabled) return true;

        auto alert = FLAlertLayer::create(
            nullptr, cfg.title.c_str(), cfg.message, "OK", nullptr, 360.f
        );
        alert->setTag(BAN_TAG);
        alert->setTouchEnabled(true);
        alert->setKeypadEnabled(true);
        this->addChild(alert, 10000);
        return true;
    }
};
