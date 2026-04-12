#pragma once
#include "ui/window.h"
#include "ui/events.h"
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

// --- Structured command types ---
// Produced by TestCommandParser from raw stdin text; stored in the queue.

namespace TestCmd {
    struct Quit     {};
    struct State    {};
    struct Reset    { std::string dir; };
    struct Dialog   { bool yes; };
    struct SetPath  { std::string side; std::string path; };
    struct KeyDown  { uc::Key key; };
    struct FKeyClick{ int slot; };   // 0-based (0 = F1 ... 9 = F10)
    struct ModClick { uc::Mod mod; };
}

using TestCommand = std::variant<
    TestCmd::Quit,
    TestCmd::State,
    TestCmd::Reset,
    TestCmd::Dialog,
    TestCmd::SetPath,
    TestCmd::KeyDown,
    TestCmd::FKeyClick,
    TestCmd::ModClick
>;

// --- Parser ---
// Converts one raw stdin line into a TestCommand.
// Called on the test thread; no locking required.

class TestCommandParser
{
public:
    static TestCommand parse(const std::string& line);
};

// --- Queue ---

struct TestQueue
{
    std::mutex               mtx;
    std::queue<TestCommand>  commands;
};

TestQueue& getTestQueue();

// Called by the main platform thread to pop and process all pending commands.
void drainTestQueue(uc::Window* win);

// Starts the background stdin-reader thread.
// wakeup() is called (from the test thread) each time a command is enqueued.
std::thread startTestThread(std::function<void()> wakeup);
