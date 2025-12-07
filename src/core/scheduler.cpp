#include "scheduler.hpp"
#include "thread.hpp"
#include "../util/assert.hpp"
#include "../arch/x86_64/timer.hpp"
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <csignal>

// Static member initialization
thread_local Thread* Scheduler::current_ = nullptr;
Thread* Scheduler::main_thread_ = nullptr;
std::deque<Thread*> Scheduler::ready_queue_;
std::priority_queue<Scheduler::SleepEntry, std::vector<Scheduler::SleepEntry>, std::greater<>> Scheduler::sleeping_threads_;
std::vector<Thread*> Scheduler::all_threads_;
std::mutex Scheduler::threads_mutex_;
Scheduler::Stats Scheduler::stats_;
std::atomic<bool> Scheduler::preemption_enabled_{false};
std::atomic<bool> Scheduler::in_scheduler_{false};
std::chrono::milliseconds Scheduler::timeslice_{10};
thread_local int Scheduler::preemption_disabled_count_{0};

/* static */ void Scheduler::initialize() {
    // Set up the main thread
    main_thread_ = new Thread(nullptr);
    main_thread_->set_state(Thread::State::RUNNING);
    current_ = main_thread_;
    Thread::current_thread_ = main_thread_;
    
    // Add main thread to all threads list
    all_threads_.push_back(main_thread_);
}

/* static */ void Scheduler::schedule(Thread* thread, const char* name) {
    ASSERT(thread != nullptr, "Cannot schedule a null thread");
    
    // Set thread name if provided
    if (name) {
        thread->set_name(name);
    }
    
    // Set thread state to READY and add to ready queue
    thread->set_state(Thread::State::READY);
    ready_queue_.push_back(thread);
    
    // Add to all threads list for cleanup
    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        all_threads_.push_back(thread);
    }
    
    // Update statistics
    stats_.total_threads_created++;
    stats_.thread_stats[thread->id()] = {};
}

/* static */ void Scheduler::check_sleeping_threads() {
    auto current_time = now();
    
    while (!sleeping_threads_.empty()) {
        const auto& entry = sleeping_threads_.top();
        if (entry.wakeup_time > current_time) {
            break; // No more expired sleeps
        }
        
        // Wake up the thread
        Thread* thread = entry.thread;
        if (thread->state() != Thread::State::FINISHED) {
            thread->set_state(Thread::State::READY);
            ready_queue_.push_back(thread);
        }
        
        sleeping_threads_.pop();
    }
}

/* static */ Thread* Scheduler::get_next_thread() {
    // Check for sleeping threads that should wake up
    check_sleeping_threads();
    
    // Clean up finished threads
    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        auto it = std::remove_if(all_threads_.begin(), all_threads_.end(),
            [](Thread* t) {
                if (t->state() == Thread::State::FINISHED) {
                    delete t;
                    return true;
                }
                return false;
            });
        all_threads_.erase(it, all_threads_.end());
    }
    
    // Look for the next runnable thread
    while (!ready_queue_.empty()) {
        Thread* thread = ready_queue_.front();
        ready_queue_.pop_front();
        
        // Skip finished threads
        if (thread->state() == Thread::State::FINISHED) {
            continue;
        }
        
        // Check if thread was requested to exit
        if (thread->should_exit()) {
            thread->set_state(Thread::State::FINISHED);
            continue;
        }
        
        if (thread->state() == Thread::State::READY) {
            return thread;
        }
    }
    
    // If no threads are ready, return the main thread
    return main_thread_;
}

/* static */ void Scheduler::sleep_for(std::chrono::milliseconds duration) {
    Thread* current = current_;
    if (current == main_thread_ || current->state() == Thread::State::FINISHED) {
        return; // Main thread or finished threads can't sleep
    }
    
    // Calculate wakeup time
    auto wakeup_time = now() + duration;
    
    // Add to sleeping threads
    sleeping_threads_.push({wakeup_time, current});
    current->set_state(Thread::State::FINISHED); // Will be set to READY when woken up
    
    // Switch to next thread
    yield();
}

/* static */ void Scheduler::yield() {
    Thread* current = current_;
    
    // Update statistics for the current thread
    update_thread_stats(current);
    
    // If current thread isn't finished and isn't sleeping, put it back in the ready queue
    if (current != main_thread_ && 
        current->state() == Thread::State::RUNNING) {
        current->set_state(Thread::State::READY);
        ready_queue_.push_back(current);
    }
    
    // Get the next thread to run
    Thread* next = get_next_thread();
    
    // If no threads are ready, return to main thread
    if (next == nullptr) {
        next = main_thread_;
    }
    
    // Disable preemption during context switch
    disable_preemption();
    
    // Update states and perform context switch if needed
    if (current != next) {
        Thread* prev = current_;
        current_ = next;
        next->set_state(Thread::State::RUNNING);
        
        // Update statistics
        stats_.thread_stats[next->id()].times_scheduled++;
        stats_.thread_stats[next->id()].last_run = std::chrono::steady_clock::now();
        stats_.total_context_switches++;
        
        // Perform the context switch
        context_switch(&prev->context_, &next->context_);
        
        // Re-enable preemption after context switch
        enable_preemption();
    }
}

/* static */ Thread* Scheduler::current() {
    return current_;
}

/* static */ void Scheduler::update_thread_stats(Thread* thread) {
    if (!thread || thread == main_thread_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto& stats = stats_.thread_stats[thread->id()];
    
    if (stats.last_run.time_since_epoch().count() > 0) {
        stats.total_runtime += std::chrono::duration_cast<std::chrono::nanoseconds>(
            now - stats.last_run);
    }
    stats.last_run = now;
}

/* static */ void Scheduler::print_threads() {
    std::lock_guard<std::mutex> lock(threads_mutex_);
    
    printf("\n=== Thread Dump (%zu threads) ===\n", all_threads_.size());
    printf("ID\tState\t\tName\n");
    printf("--------------------------------\n");
    
    for (const auto& thread : all_threads_) {
        const char* state_str = "UNKNOWN";
        switch (thread->state()) {
            case Thread::State::READY: state_str = "READY  "; break;
            case Thread::State::RUNNING: state_str = "RUNNING"; break;
            case Thread::State::FINISHED: state_str = "FINISHED"; break;
        }
        
        printf("%lu\t%s\t%s\n", 
               thread->id(), 
               state_str,
               thread->name());
    }
    printf("\n");
}

/* static */ void Scheduler::enable_preemption() {
    if (preemption_disabled_count_ > 0) {
        preemption_disabled_count_--;
        if (preemption_disabled_count_ == 0) {
            // Re-enable preemption if counter reaches zero
            Timer::enable();
        }
    }
}

/* static */ void Scheduler::disable_preemption() {
    if (preemption_disabled_count_ == 0) {
        // Only disable if not already disabled
        Timer::disable();
    }
    preemption_disabled_count_++;
}

/* static */ void Scheduler::set_timeslice(std::chrono::milliseconds timeslice) {
    timeslice_ = timeslice;
    Timer::set_interval(timeslice_.count());
}

/* static */ void Scheduler::handle_preemption() {
    // Don't preempt if preemption is disabled or we're already in the scheduler
    if (preemption_disabled_count_ > 0 || in_scheduler_ || !preemption_enabled_) {
        return;
    }
    
    // Mark that we're in the scheduler
    in_scheduler_ = true;
    
    // Save the current context
    Thread* current = current_;
    if (current && current != main_thread_ && current->state() == Thread::State::RUNNING) {
        // Save the current thread's context
        context_save(&current->context_);
        
        // If we're still in the same thread, yield
        if (current_ == current) {
            current->set_state(Thread::State::READY);
            ready_queue_.push_back(current);
            
            // Get the next thread to run
            Thread* next = get_next_thread();
            if (next && next != current_) {
                Thread* prev = current_;
                current_ = next;
                next->set_state(Thread::State::RUNNING);
                
                // Disable preemption during context switch
                disable_preemption();
                
                // Switch context
                context_switch(&prev->context_, &next->context_);
                
                // Re-enable preemption after context switch
                enable_preemption();
            }
        }
    }
    
    in_scheduler_ = false;
}

/* static */ void Scheduler::run() {
    // Initialize statistics
    stats_.start_time = now();
    
    // Initialize and enable the timer for preemption
    Timer::initialize([]() {
        Scheduler::handle_preemption();
    }, timeslice_.count());
    
    // Enable preemption
    preemption_enabled_ = true;
    Timer::enable();
    
    // Main scheduling loop
    while (true) {
        // Check for sleeping threads that should wake up
        check_sleeping_threads();
        
        // Get the next thread to run
        Thread* next = get_next_thread();
        
        // If no threads are ready, we're done
        if (next == main_thread_ && ready_queue_.empty()) {
            break;
        }
        
        // Switch to the next thread
        if (next != nullptr && next != current_) {
            Thread* prev = current_;
            current_ = next;
            next->set_state(Thread::State::RUNNING);
            
            // Update statistics
            update_thread_stats(prev);
            stats_.thread_stats[next->id()].times_scheduled++;
            stats_.thread_stats[next->id()].last_run = std::chrono::steady_clock::now();
            stats_.total_context_switches++;
            
            // Perform the context switch
            context_switch(&prev->context_, &next->context_);
        } else {
            // No threads ready, yield to main
            yield();
        }
    }
    
    // Clean up any remaining threads
    std::lock_guard<std::mutex> lock(threads_mutex_);
    for (auto it = all_threads_.begin(); it != all_threads_.end(); ) {
        if (*it != main_thread_) {
            delete *it;
            it = all_threads_.erase(it);
        } else {
            ++it;
        }
    }
}
