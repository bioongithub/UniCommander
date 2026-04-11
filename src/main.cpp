#include "ui/window.h"
#include "ui/base_window.h"
#include "test_runner.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    bool testMode = false;
    std::string initialDir;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--test")
            testMode = true;
        else if (initialDir.empty())
            initialDir = arg;
    }

    if (!initialDir.empty())
    {
        std::error_code ec;
        std::filesystem::current_path(initialDir, ec);
        if (ec)
        {
            std::cerr << "error: cannot set directory '" << initialDir
                      << "': " << ec.message() << "\n";
            return 1;
        }
    }

    // shared_ptr so a weak_ptr can be handed to the test thread.
    std::shared_ptr<uc::Window> window(createWindow().release());

    if (!window->create("UniCommander", 800, 600))
        return 1;

    window->show();

    std::thread testThread;
    if (testMode)
    {
        auto* base = dynamic_cast<uc::BaseWindow*>(window.get());
        testThread = startTestThread(base ? base->testWakeup() : [](){});
    }

    window->run();

    // Detach the test thread instead of joining: when the app exits via F10
    // the test thread may be blocking on std::getline(std::cin) waiting for
    // the Python driver that is itself waiting for the process to exit.
    // Joining here would deadlock. Detach makes the thread non-joinable so
    // the destructor does not call std::terminate; the OS terminates all
    // threads when main() returns.
    if (testThread.joinable())
        testThread.detach();

    return 0;
}
