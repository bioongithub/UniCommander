#pragma once
#include "ui/window.h"
#include <memory>

// Starts a background thread that reads commands from stdin and dispatches
// them to the window as native events. Prints "ready\n" when the thread is up.
// Holds a weak_ptr so the thread exits cleanly if the window closes first.
// Call after window->show(), before window->run().
void startTestThread(std::weak_ptr<uc::Window> win);
