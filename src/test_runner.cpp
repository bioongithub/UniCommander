#include "test_runner.h"
#include "ui/base_window.h"
#include <iostream>
#include <map>
#include <string>
#include <thread>

using Key = uc::Key;
using Mod = uc::Mod;

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

// --- Parser ---

[[noreturn]] static void parseError(const std::string& line)
{
    std::cout << "error parse: " << line << "\n";
    std::exit(1);
}

TestCommand TestCommandParser::parse(const std::string& line)
{
    if (line == "quit")  return TestCmd::Quit{};
    if (line == "state") return TestCmd::State{};

    if (line.substr(0, 6) == "reset ")
        return TestCmd::Reset{ line.substr(6) };

    if (line.substr(0, 7) == "dialog ")
    {
        std::string answer = line.substr(7);
        if (answer == "yes") return TestCmd::Dialog{true};
        if (answer == "no")  return TestCmd::Dialog{false};
        parseError(line);
    }

    if (line.substr(0, 8) == "setpath ")
    {
        std::string rest = line.substr(8);
        auto sp = rest.find(' ');
        if (sp != std::string::npos)
            return TestCmd::SetPath{ rest.substr(0, sp), rest.substr(sp + 1) };
        parseError(line);
    }

    if (line.substr(0, 8) == "keydown ")
    {
        auto it = KEY_NAMES.find(line.substr(8));
        if (it != KEY_NAMES.end())
            return TestCmd::KeyDown{it->second};
        parseError(line);
    }

    if (line.substr(0, 10) == "fkeyclick ")
    {
        int n = 0;
        try { n = std::stoi(line.substr(10)); } catch (...) {}
        if (n >= 1 && n <= 10)
            return TestCmd::FKeyClick{n - 1};   // convert to 0-based slot
        parseError(line);
    }

    if (line.substr(0, 9) == "modclick ")
    {
        std::string mod = line.substr(9);
        if (mod == "alt")   return TestCmd::ModClick{Mod::Alt};
        if (mod == "shift") return TestCmd::ModClick{Mod::Shift};
        if (mod == "ctrl")  return TestCmd::ModClick{Mod::Ctrl};
        parseError(line);
    }

    parseError(line);
}

// --- Singleton queue ---

static TestQueue g_queue;

TestQueue& getTestQueue() { return g_queue; }

// --- Main-thread consumer ---

void drainTestQueue(uc::Window* win)
{
    // Pop one command at a time under the lock so that a recursive call
    // (e.g. from pumpEventsUntil inside a modal opened by a prior command)
    // can still dequeue the next command from the shared queue.
    auto* base = dynamic_cast<uc::BaseWindow*>(win);

    for (;;)
    {
        TestCommand cmd;
        {
            std::lock_guard<std::mutex> lk(g_queue.mtx);
            if (g_queue.commands.empty()) break;
            cmd = std::move(g_queue.commands.front());
            g_queue.commands.pop();
        }

        bool stop = std::visit([&](auto&& c) -> bool
        {
            using T = std::decay_t<decltype(c)>;

            if constexpr (std::is_same_v<T, TestCmd::Quit>)
            {
                if (win) win->close();
                return true;   // do not process commands enqueued after quit
            }
            else if constexpr (std::is_same_v<T, TestCmd::State>)
            {
                if (win) std::cout << win->stateSnapshot() << "\n";
            }
            else if constexpr (std::is_same_v<T, TestCmd::Reset>)
            {
                if (base)
                {
                    base->resetState(c.dir);
                    std::cout << win->stateSnapshot() << "\n";
                }
            }
            else if constexpr (std::is_same_v<T, TestCmd::Dialog>)
            {
                if (base) base->setDialogAnswer(c.yes);
            }
            else if constexpr (std::is_same_v<T, TestCmd::SetPath>)
            {
                if (base)
                {
                    auto* panel = (c.side == "left") ? base->leftPanel()
                                                     : base->rightPanel();
                    if (panel) panel->setPath(c.path);
                    std::cout << win->stateSnapshot() << "\n";
                }
            }
            else if constexpr (std::is_same_v<T, TestCmd::KeyDown>)
            {
                if (base) base->handleKeyDown(c.key);
            }
            else if constexpr (std::is_same_v<T, TestCmd::FKeyClick>)
            {
                if (base)
                    base->handleKeyDown(
                        static_cast<Key>(static_cast<int>(Key::F1) + c.slot));
            }
            else if constexpr (std::is_same_v<T, TestCmd::ModClick>)
            {
                if (base)
                {
                    base->toggleModifierSticky(c.mod);
                    base->invalidate();
                }
            }
            return false;
        }, cmd);

        if (stop) break;
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
            g_queue.commands.push(TestCommandParser::parse(line));
        }
        wakeup();
    }

    // stdin closed (parent process died) — ask the main thread to close.
    {
        std::lock_guard<std::mutex> lk(g_queue.mtx);
        g_queue.commands.push(TestCmd::Quit{});
    }
    wakeup();
}

std::thread startTestThread(std::function<void()> wakeup)
{
    return std::thread(testLoop, std::move(wakeup));
}
