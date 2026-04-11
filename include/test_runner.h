#pragma once
#include "ui/window.h"
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// Thread-safe FIFO of raw stdin command lines.
// The test thread is the sole producer; the main platform thread is the
// sole consumer (via drainTestQueue).
struct TestQueue
{
    std::mutex              mtx;
    std::queue<std::string> lines;
};

// Returns the singleton queue.
TestQueue& getTestQueue();

// Called by the main platform thread (from its event handler) to pop and
// process all pending commands. win must be the live Window*.
// Safe to call when the queue is empty (no-op).
void drainTestQueue(uc::Window* win);

// Starts the background stdin-reader thread.
// wakeup() is called (from the test thread) each time a command is enqueued;
// it must be safe to invoke from any thread.
std::thread startTestThread(std::function<void()> wakeup);
