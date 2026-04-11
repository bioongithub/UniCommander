#include "test_runner.h"
#include "ui/base_window.h"
#include <iostream>
#include <map>
#include <string>
#include <thread>

using Key = uc::BaseWindow::Key;

static const std::map<std::string, Key> KEY_NAMES = {
    {"up",     Key::Up},
    {"down",   Key::Down},
    {"return", Key::Return},
    {"tab",    Key::Tab},
    {"escape", Key::Escape},
    {"q",      Key::Q},
    {"f1",     Key::F1},  {"f2",  Key::F2},  {"f3",  Key::F3},  {"f4",  Key::F4},
    {"f5",     Key::F5},  {"f6",  Key::F6},  {"f7",  Key::F7},  {"f8",  Key::F8},
    {"f9",     Key::F9},  {"f10", Key::F10},
};

static void testLoop(std::weak_ptr<uc::Window> winWeak)
{
    // Unbuffer stdout so every write reaches the parent process immediately.
    std::cout << std::unitbuf;
    std::cout << "ready\n";

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty()) continue;

        // Lock on every command — exits loop if window was already destroyed.
        auto win = winWeak.lock();
        if (!win) return;

        if (line == "quit")
        {
            win->close();
            return;
        }

        if (line == "state")
        {
            std::cout << win->stateSnapshot() << "\n";
            continue;
        }

        if (line.substr(0, 6) == "reset ")
        {
            std::string dir = line.substr(6);
            auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
            if (base)
            {
                base->resetState(dir);
                std::cout << win->stateSnapshot() << "\n";
            }
            continue;
        }

        if (line.substr(0, 7) == "dialog ")
        {
            std::string answer = line.substr(7);
            auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
            if (base)
            {
                if (answer == "yes")       base->setDialogAnswer(true);
                else if (answer == "no")   base->setDialogAnswer(false);
                else std::cout << "error unknown dialog answer: " << answer << "\n";
            }
            continue;
        }

        if (line.substr(0, 8) == "setpath ")
        {
            // format: setpath left|right <absolute-path>
            std::string rest = line.substr(8);
            auto sp = rest.find(' ');
            if (sp != std::string::npos)
            {
                std::string side = rest.substr(0, sp);
                std::string path = rest.substr(sp + 1);
                auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
                if (base)
                {
                    auto* panel = (side == "left") ? base->leftPanel()
                                                   : base->rightPanel();
                    if (panel) panel->setPath(path);
                    std::cout << win->stateSnapshot() << "\n";
                }
            }
            continue;
        }

        if (line.substr(0, 8) == "keydown ")
        {
            std::string keyName = line.substr(8);
            auto it = KEY_NAMES.find(keyName);
            if (it == KEY_NAMES.end())
            {
                std::cout << "error unknown key: " << keyName << "\n";
            }
            else
            {
                auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
                if (base)
                {
                    // scheduleKeyDown() delivers the key on the platform's
                    // main thread (serialised with rendering) to prevent data
                    // races on panel state.  On Win32 it uses SendMessage so
                    // this call blocks until the key is fully processed.
                    base->scheduleKeyDown(it->second);
                    // If this key triggered a quit, exit now so the test
                    // thread does not block on std::cin while the CRT shuts down.
                    if (base->isClosing()) return;
                }
            }
            continue;
        }

        if (line.substr(0, 10) == "fkeyclick ")
        {
            std::string nstr = line.substr(10);
            int n = 0;
            try { n = std::stoi(nstr); } catch (...) {}
            if (n < 1 || n > 10)
            {
                std::cout << "error fkeyclick out of range: " << nstr << "\n";
            }
            else
            {
                auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
                if (base)
                {
                    Key key = static_cast<Key>(static_cast<int>(Key::F1) + (n - 1));
                    base->scheduleKeyDown(key);
                    if (base->isClosing()) return;
                }
            }
            continue;
        }

        if (line.substr(0, 9) == "modclick ")
        {
            std::string modname = line.substr(9);
            auto* base = dynamic_cast<uc::BaseWindow*>(win.get());
            if (base)
            {
                using Mod = uc::BaseWindow::Mod;
                if      (modname == "alt")   base->toggleModifierSticky(Mod::Alt);
                else if (modname == "shift") base->toggleModifierSticky(Mod::Shift);
                else if (modname == "ctrl")  base->toggleModifierSticky(Mod::Ctrl);
                else std::cout << "error unknown modifier: " << modname << "\n";
            }
            continue;
        }

        std::cout << "error unknown: " << line << "\n";
    }

    // stdin closed (parent process died) — close the window if still alive.
    if (auto win = winWeak.lock())
        win->close();
}

std::thread startTestThread(std::weak_ptr<uc::Window> win)
{
    return std::thread(testLoop, std::move(win));
}
