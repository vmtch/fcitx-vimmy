#include "vimmy.h"
#include <fcitx-utils/log.h>

void VimmyEngine::keyEvent(const fcitx::InputMethodEntry &entry, fcitx::KeyEvent &keyEvent) {
    auto key = keyEvent.key();
    switch (currentMode) {
        case NORMAL:
            if (key.check(fcitx::Key("i"))) {
                FCITX_INFO() << "Switched to Insert mode";
                currentMode = INSERT;
                keyEvent.filterAndAccept();
            }
            break;
        case INSERT:
            if (key.check(fcitx::Key("Escape"))) {
                FCITX_INFO() << "Switched to Normal mode";
                currentMode = NORMAL;
                keyEvent.filterAndAccept();
            }
            break;
    }
    FCITX_INFO() << key;
    FCITX_INFO() << currentMode;
}

FCITX_ADDON_FACTORY(VimmyEngineFactory)