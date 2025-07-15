#pragma once

#include <algorithm>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/addonfactory.h>
#include <fcitx/inputcontext.h>
#include <fcitx-utils/log.h>
#include <fcitx/inputpanel.h>
#include <fcitx/addonmanager.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/event.h>

#include "normal.h"

#define NORMAL 0
#define INSERT 1
#define VISUAL 2

#define SUBNORMAL   100
#define SINGLECHAR  101

int getDigit(const fcitx::Key &key)
{
    if (key.sym() >= FcitxKey_0 && key.sym() <= FcitxKey_9) {
        return key.sym() - FcitxKey_0;
    } else {
        return -1;
    }
}

class VimmyEngine : public fcitx::InputMethodEngineV2 {
public:
    void keyEvent(const fcitx::InputMethodEntry &entry, fcitx::KeyEvent &keyEvent) override;
    void startExternalEditor(fcitx::InputContext* ic);

private:
    int currentMode = NORMAL;
    int currentSubMode = SUBNORMAL;
    int multiplier = 0;
    size_t cursorPosition = 0;
    std::string tmpText;
    std::string preeditText;
    char singleChar;
    /* * The pid of the external editor process.
     * If it is -1, then no external editor is running.
     */
    pid_t externalPid = -1;
    /* * The file that the external editor is editing.
     * If it is empty, then no external editor is running.
     */
    std::string externalTmpFile;
    fcitx::InputContext *externalIC = nullptr;
    fcitx::EventSource *externalTimer = nullptr;

    void checkExternalEditor();
    void commitString(fcitx::InputContext *ic, const std::string &str);
    void updatePreedit(fcitx::InputContext *inputContext);
};

class VimmyEngineFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
        FCITX_UNUSED(manager);
        return new VimmyEngine;
    }
};
