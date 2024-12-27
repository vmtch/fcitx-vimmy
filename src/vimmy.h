#pragma once

#include <fcitx/inputmethodengine.h>
#include <fcitx/addonfactory.h>
#define NORMAL 0
#define INSERT 1
#define VIDUAL 2


class VimmyEngine : public fcitx::InputMethodEngineV2 {
public:
    void keyEvent(const fcitx::InputMethodEntry &entry, fcitx::KeyEvent &keyEvent) override;

private:
    int currentMode = 0;
};

class VimmyEngineFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
        FCITX_UNUSED(manager);
        return new VimmyEngine;
    }
};
