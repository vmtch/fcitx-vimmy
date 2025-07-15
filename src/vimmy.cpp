#include "vimmy.h"

void VimmyEngine::keyEvent(const fcitx::InputMethodEntry &entry, fcitx::KeyEvent &keyEvent) {
    checkExternalEditor();
    if (keyEvent.isRelease()) return;
    auto key = keyEvent.key();
    auto inputContext = keyEvent.inputContext();
    if (!inputContext) {
        return;
    }
    switch (currentMode) {
    case NORMAL:
        switch (currentSubMode) {
        case SUBNORMAL:
            // number
            {
                int digit = getDigit(key);
                if (digit >= 0) {
                    multiplier = std::min(100000, 10 * multiplier + digit);
                    keyEvent.filterAndAccept();
                    break;
                }
            }
            // other
            if (multiplier == 0) {
                multiplier = 1;
            }
            if (key.check(fcitx::Key("Escape")) ||
                key.check(fcitx::Key("Control+bracketleft"))) {
                    multiplier = 0;
            } else if (key.check(fcitx::Key("i"))) {
                FCITX_INFO() << "Switched to Insert mode(i)";
                currentMode = INSERT;
                startExternalEditor(inputContext);
            } else if (key.check(fcitx::Key("a"))) {
                FCITX_INFO() << "Switched to Insert mode(a)";
                if (cursorPosition < preeditText.size()) {
                    ++cursorPosition;
                }
                currentMode = INSERT;
            } else if (key.check(fcitx::Key("I"))) {
                FCITX_INFO() << "Switched to Insert mode(I)";
                cursorPosition = 0;
                currentMode = INSERT;
            } else if (key.check(fcitx::Key("A"))) {
                FCITX_INFO() << "Switched to Insert mode(a)";
                cursorPosition = preeditText.size();
                currentMode = INSERT;
            } else if (key.check(fcitx::Key("h"))) {
                cursorPosition = std::max(0, int(cursorPosition - multiplier));
                multiplier = 0;
            } else if (key.check(fcitx::Key("l"))) {
                cursorPosition = std::min(int(preeditText.size()), int(cursorPosition + multiplier));
                multiplier = 0;
            } else if (key.check(fcitx::Key("f"))) {
                singleChar = 'f';
                currentSubMode = SINGLECHAR;
            } else if (key.check(fcitx::Key("t"))) {
                singleChar = 't';
                currentSubMode = SINGLECHAR;
            }
            updatePreedit(inputContext);
            keyEvent.filterAndAccept();
            break;
        case SINGLECHAR:
            if (key.isSimple()) {
                switch (singleChar) {
                int p;
                case 'f':
                    p = findNthOccurrence(preeditText, 'a', multiplier);
                    if (p > 0) {
                        cursorPosition = p;
                    }
                    multiplier = 0;
                    break;
                case 't':
                    p = findNthOccurrence(preeditText, 'a', multiplier) - 1;
                    if (p > 0) {
                        cursorPosition = p;
                    }
                    multiplier = 0;
                    break;
                }
            }
            currentSubMode = SUBNORMAL;
            updatePreedit(inputContext);
            keyEvent.filterAndAccept();
            break;
        }
        break;
    case INSERT:
        if (key.check(fcitx::Key("Escape")) ||
            key.check(fcitx::Key("Control+bracketleft"))) {
            if (multiplier > 1) {
                std::string repeatedText;
                for (int i = 1; i < multiplier; ++i) {
                    repeatedText += tmpText;
                }
                preeditText.insert(cursorPosition, repeatedText);
                cursorPosition += repeatedText.size();
                updatePreedit(inputContext);
            }
            FCITX_INFO() << "Switched to Normal mode";
            currentMode = NORMAL;
            multiplier = 0;
            tmpText = "";
            keyEvent.filterAndAccept();
        } else if (key.check(fcitx::Key("BackSpace"))) {
            if (!preeditText.empty() && cursorPosition > 0) {
                    preeditText.erase(cursorPosition - 1, 1);
                    tmpText += '\b';
                    --cursorPosition;
                    updatePreedit(inputContext);
            } else {
                //PASSTHROUGH BACKSPACE
                FCITX_INFO() << "BAKCSPACE PASSTHROUGH";
                inputContext->forwardKey(key);
                break;
            }
            keyEvent.filterAndAccept();
        } else if (key.check(fcitx::Key("Delete"))) {
            if (!preeditText.empty()) {
                if (cursorPosition < preeditText.size()) {
                    preeditText.erase(cursorPosition, 1);
                    updatePreedit(inputContext);
                }
            }
            keyEvent.filterAndAccept();
        } else if (key.check(fcitx::Key("Return"))) {
            if (!preeditText.empty()) {
                std::string left = preeditText.substr(0, cursorPosition);
                std::string right = preeditText.substr(cursorPosition);
                inputContext->commitString(left);
                preeditText = right;
                cursorPosition = 0;
                updatePreedit(inputContext);
            }
        } else if (key.isSimple()) {
            char inputChar = key.sym();
            preeditText.insert(cursorPosition, 1, inputChar);
            tmpText += inputChar;
            ++cursorPosition;
            updatePreedit(inputContext);
            keyEvent.filterAndAccept();
        }
        break;
    }
    FCITX_INFO() << "Mode:" << currentMode
        << " SubMode:" << currentSubMode
        << " Multiplier:" << multiplier
        << " keycoode:" << key;
}

void VimmyEngine::startExternalEditor(fcitx::InputContext *ic) {
    // This is a placeholder for starting an external editor.
    // In a real implementation, you would launch the editor here.
    if (externalPid > 0) {
        int status = 0;
        pid_t r = waitpid(externalPid, &status, WNOHANG);
        if (r == 0) {
            FCITX_INFO() << "External editor is still running.";
            return;
        }
        FCITX_INFO() << "External editor exited with status:" << status;
        externalPid = -1;
        if (!externalTmpFile.empty()) {
            unlink(externalTmpFile.c_str());
            externalTmpFile.clear();
        }
    }
    char tmpFile[] = "/tmp/vimmyXXXXXX";
    int fd = mkstemp(tmpFile);
    if (fd < 0) {
        FCITX_ERROR() << "Failed to create temporary file for external editor.";
        return;
    }
    close(fd);
    externalTmpFile = tmpFile;

    externalIC = ic;
    pid_t pid = fork();
    const char *term = getenv("TERM");
    if (!term || !*term) {
        term = "alacritty";
    }
    if (pid == 0) {
        setsid();
        execlp(
            term,
            term,
            "-e", "vim",
            externalTmpFile.c_str(),
            (char *)nullptr
        );
        _exit(127);
    } else if (pid > 0) {
        externalPid = pid;
        FCITX_INFO() << "External editor started with PID:" << externalPid;
    } else {
        FCITX_ERROR() << "Failed to fork for external editor.";
        unlink(externalTmpFile.c_str());
        externalTmpFile.clear();
        return;
    }
}

void VimmyEngine::checkExternalEditor() {
    if (externalPid <= 0) {
        return;
    }
    int status = 0;
    pid_t r = waitpid(externalPid, &status, WNOHANG);
    if (r == 0) {
        FCITX_INFO() << "External editor is still running.";
        return;
    }
    FCITX_INFO() << "External editor exited with status:" << status;
    externalPid = -1;


    std::string text;
    if (!externalTmpFile.empty()) {
        std::ifstream ifs(externalTmpFile);
        text.assign((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
        unlink(externalTmpFile.c_str());
        externalTmpFile.clear();
    }

    commitString(externalIC, text);

    externalPid = -1;
}

void VimmyEngine::commitString(fcitx::InputContext *ic, const std::string &text) {
    if (ic) {
        if (!text.empty()) {
            ic->commitString(text);
        }
        ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }
    currentMode = NORMAL;
    currentSubMode = SUBNORMAL;
    multiplier = 0;
    preeditText.clear();
    cursorPosition = 0;
    if (externalIC) {
        updatePreedit(externalIC);
    }
}

void VimmyEngine::updatePreedit(fcitx::InputContext *inputContext) {
    auto &inputPanel = inputContext->inputPanel();
    inputPanel.reset();

    fcitx::Text preedit(preeditText, fcitx::TextFormatFlag::HighLight);
    /*
    if (!preeditText.empty()) {
        fcitx::TextFormatFlag hilightFlag = fcitx::TextFormatFlag::HighLight;
        preedit.setFormat(cursorPosition, cursorPosition + 1, highlightFlag);
    }
    */

    preedit.setCursor(cursorPosition);

    if (inputContext->capabilityFlags().test(fcitx::CapabilityFlag::Preedit)) {
        inputPanel.setClientPreedit(preedit);
    } else {
        inputPanel.setPreedit(preedit);
    }

    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    inputContext->updatePreedit();
}

FCITX_ADDON_FACTORY(VimmyEngineFactory)
