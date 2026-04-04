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
                if (base) base->handleKeyDown(it->second);
            }
            continue;
        }

        std::cout << "error unknown: " << line << "\n";
    }

    // stdin closed (parent process died) — close the window if still alive.
    if (auto win = winWeak.lock())
        win->close();
}

void startTestThread(std::weak_ptr<uc::Window> win)
{
    std::thread t(testLoop, std::move(win));
    t.detach();
}
