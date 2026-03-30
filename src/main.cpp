#include "ui/window.h"

int main()
{
    auto window = createWindow();

    if (!window->create("UniCommander", 800, 600))
        return 1;

    window->show();
    window->run();
    return 0;
}
