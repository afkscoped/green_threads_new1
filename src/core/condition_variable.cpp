#include "condition_variable.hpp"
#include "scheduler.hpp"
#include "thread.hpp"
#include <cstdio>

ConditionVariable::ConditionVariable() {}

ConditionVariable::~ConditionVariable() {
    ASSERT(wait_queue_.empty(), "Destroying condition variable with waiting threads");
}

void ConditionVariable::wait(Mutex& mutex) {
    Thread* current = Scheduler::current();
    
    // Add to wait queue
    wait_queue_.push_back(current);
    
    // Release the mutex (saving the lock count)
    unsigned int lock_count = mutex.lock_count_;
    mutex.unlock();
    
    // Block the current thread
    current->set_state(Thread::State::BLOCKED);
    Scheduler::yield();
    
    // When we get here, we've been woken up and should reacquire the mutex
    while (!mutex.try_lock()) {
        Scheduler::yield();
    }
    
    // Restore the lock count
    mutex.lock_count_ = lock_count;
}

void ConditionVariable::notify_one() {
    if (!wait_queue_.empty()) {
        Thread* thread = wait_queue_.front();
        wait_queue_.pop_front();
        
        // Add to scheduler's ready queue
        thread->set_state(Thread::State::READY);
        Scheduler::schedule(thread);
    }
}

void ConditionVariable::notify_all() {
    // Move all waiting threads to the scheduler's ready queue
    while (!wait_queue_.empty()) {
        Thread* thread = wait_queue_.front();
        wait_queue_.pop_front();
        
        thread->set_state(Thread::State::READY);
        Scheduler::schedule(thread);
    }
}
