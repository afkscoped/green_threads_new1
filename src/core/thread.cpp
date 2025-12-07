#include "thread.hpp"
#include "scheduler.hpp"
#include "../util/assert.hpp"
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <functional>

// Initialize thread-local storage for the current thread pointer
thread_local Thread* Thread::current_thread_ = nullptr;

// Initialize the thread ID counter
std::atomic<uint64_t> Thread::next_id_ = 1;

Thread::Thread(EntryFunc entry, size_t stack_size)
    : stack_size_(stack_size), entry_(entry) {
    // Allocate stack with page alignment
    const size_t page_size = 4096;
    stack_ = std::aligned_alloc(page_size, stack_size_);
    ASSERT(stack_ != nullptr, "Failed to allocate stack");
    
    // Initialize stack with a pattern for debugging (0xDEADBEEF)
    std::memset(stack_, 0xDE, stack_size_);
    
    // Assign a unique ID to this thread
    id_ = next_id_++;
    
    // Initialize the context to start in our trampoline
    context_init(&context_, stack_, stack_size_, 
                reinterpret_cast<void (*)()>(&Thread::start_trampoline));
    
    // Set a default name based on the thread ID
    static char default_name[32];
    snprintf(default_name, sizeof(default_name), "thread-%lu", id_);
    name_ = default_name;
}

Thread::~Thread() {
    // Run cleanup handlers if not already run
    if (state_ != State::FINISHED) {
        for (auto& handler : cleanup_handlers_) {
            if (handler) handler();
        }
        cleanup_handlers_.clear();
    }
    
    // Free the stack
    if (stack_) {
        // Wipe the stack to catch use-after-free bugs
        std::memset(stack_, 0xDE, stack_size_);
        std::free(stack_);
        stack_ = nullptr;
    }
    
    // Clear the name
    name_ = nullptr;
}

/* static */ Thread* Thread::current() {
    return current_thread_;
}

/* static */ void Thread::start_trampoline() {
    // Entry point used by the assembly trampoline: dispatch to thread_entry
    Thread* self = Scheduler::current();
    thread_entry(self);
}

/* static */ void Thread::thread_entry(Thread* self) {
    // Set the current thread
    current_thread_ = self;
    
    // Call the user's entry function if not requested to exit
    if (!self->should_exit() && self->entry_) {
        try {
            self->entry_();
        } catch (const std::exception& e) {
            fprintf(stderr, "Thread %s (%lu) crashed: %s\n", 
                   self->name(), self->id(), e.what());
        } catch (...) {
            fprintf(stderr, "Thread %s (%lu) crashed with unknown exception\n",
                   self->name(), self->id());
        }
    }
    
    // Run cleanup handlers
    for (auto& handler : self->cleanup_handlers_) {
        if (handler) {
            try {
                handler();
            } catch (...) {
                // Ignore exceptions in cleanup handlers
            }
        }
    }
    self->cleanup_handlers_.clear();
    
    // Mark as finished and yield back to scheduler
    self->set_state(State::FINISHED);
    Scheduler::yield();
    
    // We should never get here
    ASSERT(false, "Thread resumed after finishing");
}
