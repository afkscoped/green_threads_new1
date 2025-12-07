#include "../core/scheduler.hpp"
#include "../core/thread.hpp"
#include <cstdio>

// Initialize the runtime
void runtime_init() {
    Scheduler::initialize();
}

// Create and schedule a new thread
void spawn(void (*entry)()) {
    Thread* thread = new Thread(entry);
    Scheduler::schedule(thread);
}

// Yield the current thread
void yield() {
    Scheduler::yield();
}

// Start the scheduler
void runtime_run() {
    Scheduler::run();
}
