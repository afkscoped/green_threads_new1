#pragma once

#include "../util/assert.hpp"
#include <cstdint>
#include <deque>
#include <atomic>

class Thread;
class ConditionVariable;

class Mutex {
public:
    Mutex();
    ~Mutex();
    
    // Non-copyable
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    
    // Lock the mutex
    void lock();
    
    // Try to lock the mutex without blocking
    bool try_lock();
    
    // Unlock the mutex
    void unlock();
    
    // Get the current owner thread ID (for debugging)
    Thread* owner() const { return owner_; }
    
    // Check if the mutex is locked
    bool is_locked() const { return owner_ != nullptr; }

private:
    // The current owner thread (nullptr if unlocked)
    std::atomic<Thread*> owner_;
    
    // Queue of threads waiting for this mutex
    std::deque<Thread*> wait_queue_;
    
    // For debugging
    uint32_t lock_count_;
    
    friend class ConditionVariable;
};
