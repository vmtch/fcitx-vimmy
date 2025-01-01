#include "vimmy.h"

void VimmyEngine::keyEvent(const fcitx::InputMethodEntry &entry, fcitx::KeyEvent &keyEvent) {
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