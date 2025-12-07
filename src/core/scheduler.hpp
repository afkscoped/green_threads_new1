#pragma once

#include "thread.hpp"
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <chrono>
#include <queue>
#include <functional>
#include <atomic>
#include <csignal>
#include <mutex>

class Scheduler {
public:
    // Scheduler statistics
    struct Stats {
        size_t total_threads_created = 0;
        size_t total_context_switches = 0;
        std::chrono::steady_clock::time_point start_time;
        
        // Per-thread statistics
        struct ThreadStats {
            size_t times_scheduled = 0;
            std::chrono::steady_clock::time_point last_run;
            std::chrono::nanoseconds total_runtime{0};
        };
        
        std::unordered_map<uint64_t, ThreadStats> thread_stats;
    };

    // Initialize the scheduler
    static void initialize();
    
    // Schedule a new thread
    static void schedule(Thread* thread, const char* name = nullptr);
    
    // Yield the current thread
    static void yield();
    
    // Get the current thread
    static Thread* current();
    
    // Put the current thread to sleep for the specified duration
    static void sleep_for(std::chrono::milliseconds duration);
    
    // Check and wake up any sleeping threads whose sleep has expired
    static void check_sleeping_threads();
    
    // Enable or disable preemption
    static void enable_preemption();
    static void disable_preemption();
    
    // Check if preemption is enabled
    static bool is_preemption_enabled() { return preemption_enabled_; }
    
    // Handle a preemption interrupt (called from signal handler)
    static void handle_preemption();
    
    // Set the timeslice duration for preemption
    static void set_timeslice(std::chrono::milliseconds timeslice);
    
    // Start the scheduler
    static void run();
    
    // Get the current time in the scheduler's clock
    static std::chrono::steady_clock::time_point now() {
        return std::chrono::steady_clock::now();
    }
    
    // Get scheduler statistics
    static const Stats& stats() { return stats_; }
    
    // Print thread list
    static void print_threads();
    
private:
    // Get the next runnable thread
    static Thread* get_next_thread();
    
    // Update thread statistics
    static void update_thread_stats(Thread* thread);
    
    // Thread state
    static thread_local Thread* current_;
    static Thread* main_thread_;
    static std::deque<Thread*> ready_queue_;
    
    // Thread management
    static std::vector<Thread*> all_threads_;
    static std::mutex threads_mutex_;
    
    // Statistics
    static Stats stats_;
    
    // Preemption control
    static std::atomic<bool> preemption_enabled_;
    static std::atomic<bool> in_scheduler_;
    static std::chrono::milliseconds timeslice_;
    static thread_local int preemption_disabled_count_;
    
    // Sleep entry for sleeping threads
    struct SleepEntry {
        std::chrono::steady_clock::time_point wakeup_time;
        Thread* thread;
        bool operator>(const SleepEntry& other) const { 
            return wakeup_time > other.wakeup_time; 
        }
    };
    static std::priority_queue<SleepEntry, std::vector<SleepEntry>, std::greater<>> sleeping_threads_;
};
