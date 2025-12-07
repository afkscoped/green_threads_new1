#pragma once

#include "mutex.hpp"
#include <deque>

class Thread;

class ConditionVariable {
public:
    ConditionVariable();
    ~ConditionVariable();
    
    // Wait for the condition (releases the lock, then reacquires it before returning)
    void wait(Mutex& mutex);
    
    // Notify one waiting thread
    void notify_one();
    
    // Notify all waiting threads
    void notify_all();
    
private:
    // Queue of threads waiting on this condition
    std::deque<Thread*> wait_queue_;
};
