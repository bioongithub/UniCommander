#include "test_runner.h"
#include <iostream>
#include <string>
#include <thread>

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
