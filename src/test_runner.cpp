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
    {"left",   Key::Left},
    {"right",  Key::Right},
    {"return", Key::Return},
    {"tab",    Key::Tab},
    {"escape", Key::Escape},
    {"q",      Key::Q},
    {"f1",     Key::F1},  {"f2",  Key::F2},  {"f3",  Key::F3},  {"f4",  Key::F4},
    {"f5",     Key::F5},  {"f6",  Key::F6},  {"f7",  Key::F7},  {"f8",  Key::F8},
    {"f9",     Key::F9},  {"f10", Key::F10},
};

// --- Singleton queue ---

static TestQueue g_queue;

TestQueue& getTestQueue() { return g_queue; }

// --- Main-thread consumer ---

void drainTestQueue(uc::Window* win)
{
    // Swap the queue out under the lock, then process without holding it.
    std::queue<std::string> local;
    {
        std::lock_guard<std::mutex> lk(g_queue.mtx);
        std::swap(local, g_queue.lines);
    }

    auto* base = dynamic_cast<uc::BaseWindow*>(win);

    while (!local.empty())
    {
        std::string line = std::move(local.front());
        local.pop();

        if (line.empty()) continue;

        if (line == "quit")
        {
            if (win) win->close();
            return;  // do not process commands enqueued after quit
        }

        if (line == "state")
        {
            if (win) std::cout << win->stateSnapshot() << "\n";
            continue;
        }

        if (line.substr(0, 6) == "reset ")
        {
            std::string dir = line.substr(6);
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
            if (base)
            {
                if      (answer == "yes") base->setDialogAnswer(true);
                else if (answer == "no")  base->setDialogAnswer(false);
                else std::cout << "error unknown dialog answer: " << answer << "\n";
            }
            continue;
        }

        if (line.substr(0, 8) == "setpath ")
        {
            std::string rest = line.substr(8);
            auto sp = rest.find(' ');
            if (sp != std::string::npos && base)
            {
                std::string side = rest.substr(0, sp);
                std::string path = rest.substr(sp + 1);
                auto* panel = (side == "left") ? base->leftPanel()
                                               : base->rightPanel();
                if (panel) panel->setPath(path);
                std::cout << win->stateSnapshot() << "\n";
            }
            continue;
        }

        if (line.substr(0, 8) == "keydown ")
        {
            std::string keyName = line.substr(8);
            auto it = KEY_NAMES.find(keyName);
            if (it == KEY_NAMES.end())
                std::cout << "error unknown key: " << keyName << "\n";
            else if (base)
                base->handleKeyDown(it->second);
            continue;
        }

        if (line.substr(0, 10) == "fkeyclick ")
        {
            std::string nstr = line.substr(10);
            int n = 0;
            try { n = std::stoi(nstr); } catch (...) {}
            if (n < 1 || n > 10)
                std::cout << "error fkeyclick out of range: " << nstr << "\n";
            else if (base)
                base->handleKeyDown(static_cast<Key>(static_cast<int>(Key::F1) + (n - 1)));
            continue;
        }

        if (line.substr(0, 9) == "modclick ")
        {
            std::string modname = line.substr(9);
            if (base)
            {
                using Mod = uc::BaseWindow::Mod;
                if      (modname == "alt")   base->toggleModifierSticky(Mod::Alt);
                else if (modname == "shift") base->toggleModifierSticky(Mod::Shift);
                else if (modname == "ctrl")  base->toggleModifierSticky(Mod::Ctrl);
                else { std::cout << "error unknown modifier: " << modname << "\n"; continue; }
                base->invalidate();
            }
            continue;
        }

        std::cout << "error unknown: " << line << "\n";
    }
}

// --- Test-thread producer ---

static void testLoop(std::function<void()> wakeup)
{
    std::cout << std::unitbuf;
    std::cout << "ready\n";

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty()) continue;
        {
            std::lock_guard<std::mutex> lk(g_queue.mtx);
            g_queue.lines.push(line);
        }
        wakeup();
    }

    // stdin closed (parent process died) — ask the main thread to close.
    {
        std::lock_guard<std::mutex> lk(g_queue.mtx);
        g_queue.lines.push("quit");
    }
    wakeup();
}

std::thread startTestThread(std::function<void()> wakeup)
{
    return std::thread(testLoop, std::move(wakeup));
}
