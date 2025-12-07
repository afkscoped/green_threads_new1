#include "mutex.hpp"
#include "scheduler.hpp"
#include "thread.hpp"
#include <cstdio>

Mutex::Mutex() 
    : owner_(nullptr), 
      lock_count_(0) {
}

Mutex::~Mutex() {
    ASSERT(owner_ == nullptr, "Destroying a locked mutex");
}

void Mutex::lock() {
    Thread* current = Scheduler::current();
    
    // If the current thread already owns the lock, just increment the count
    if (owner_ == current) {
        lock_count_++;
        return;
    }
    
    // Try to take the lock
    while (true) {
        // Use compare_exchange_weak to atomically check and set the owner
        Thread* expected = nullptr;
        if (owner_.compare_exchange_weak(expected, current)) {
            // Successfully acquired the lock
            lock_count_ = 1;
            return;
        }
        
        // Lock is held by another thread, add to wait queue and block
        wait_queue_.push_back(current);
        current->set_state(Thread::State::BLOCKED);
        Scheduler::yield();
        
        // When we get here, we've been woken up and should try to acquire the lock again
    }
}

bool Mutex::try_lock() {
    Thread* current = Scheduler::current();
    
    // If the current thread already owns the lock, just increment the count
    if (owner_ == current) {
        lock_count_++;
        return true;
    }
    
    // Try to take the lock
    Thread* expected = nullptr;
    if (owner_.compare_exchange_strong(expected, current)) {
        // Successfully acquired the lock
        lock_count_ = 1;
        return true;
    }
    
    // Lock is held by another thread
    return false;
}

void Mutex::unlock() {
    Thread* current = Scheduler::current();
    ASSERT(owner_ == current, "Thread does not own the mutex");
    
    // Decrement the lock count
    if (--lock_count_ > 0) {
        return; // Still holding the lock
    }
    
    // Release the lock
    owner_ = nullptr;
    
    // Wake up the next waiting thread, if any
    if (!wait_queue_.empty()) {
        Thread* next = wait_queue_.front();
        wait_queue_.pop_front();
        
        // Add to scheduler's ready queue
        next->set_state(Thread::State::READY);
        Scheduler::schedule(next);
    }
}
