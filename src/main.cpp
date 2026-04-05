#include "ui/window.h"
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
        testThread = startTestThread(std::weak_ptr<uc::Window>(window));

    window->run();

    // Join the test thread before returning so the CRT never sees a live
    // thread during process shutdown (which would call std::terminate).
    if (testThread.joinable())
        testThread.join();

    return 0;
}
