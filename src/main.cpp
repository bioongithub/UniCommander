#include "ui/window.h"
#include "test_runner.h"
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    const bool testMode = argc > 1 && std::string(argv[1]) == "--test";

    // shared_ptr so a weak_ptr can be handed to the test thread.
    std::shared_ptr<uc::Window> window(createWindow().release());

    if (!window->create("UniCommander", 800, 600))
        return 1;

    window->show();

    if (testMode)
        startTestThread(std::weak_ptr<uc::Window>(window));

    window->run();
    return 0;
}
