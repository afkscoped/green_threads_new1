#pragma once

#include "context.hpp"
#include <functional>
#include <cstdint>
#include <string>
#include <atomic>
#include <vector>
#include <cstring>

class Thread {
public:
    enum class State {
        READY,     // Ready to run
        RUNNING,   // Currently running
        BLOCKED,   // Blocked on a synchronization primitive
        FINISHED   // Has completed execution
    };

    using EntryFunc = void (*)();
    
    Thread(EntryFunc entry, size_t stack_size = 64 * 1024);
    ~Thread();
    
    // Prevent copying
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    
    // Get the current running thread
    static Thread* current();
    
    // Thread entry point wrapper
    static void thread_entry(Thread* self);
    static void start_trampoline();
    
    // Thread identification
    uint64_t id() const { return id_; }
    const char* name() const { return name_; }
    void set_name(const char* name) { 
        if (name) {
            size_t len = strlen(name);
            if (len > 0) {
                name_ = strdup(name);
            } else {
                name_ = nullptr;
            }
        } else {
            name_ = nullptr;
        }
    }
    
    // State management
    State state() const { return state_; }
    void set_state(State state) { 
        state_ = state; 
        if (state == State::FINISHED) {
            cleanup_handlers_.clear();
        }
    }
    
    // Thread exit and cleanup
    using CleanupHandler = std::function<void()>;
    void add_cleanup_handler(CleanupHandler handler) {
        cleanup_handlers_.push_back(handler);
    }
    
    // Request thread to exit
    void request_exit() { should_exit_ = true; }
    bool should_exit() const { return should_exit_; }
    
    // Stack information
    void* stack_base() const { return stack_; }
    size_t stack_size() const { return stack_size_; }
    
private:
    // Thread-local storage for current thread pointer
    static thread_local Thread* current_thread_;
    
    // Thread state
    Context context_;
    void* stack_;
    size_t stack_size_;
    EntryFunc entry_;
    State state_ = State::READY;
    uint64_t id_;
    char* name_ = nullptr;
    
    // Thread ID counter
    static std::atomic<uint64_t> next_id_;
    
    // Cleanup handlers
    std::vector<CleanupHandler> cleanup_handlers_;
    bool should_exit_ = false;
    
    friend class Scheduler;  // For context access
};
