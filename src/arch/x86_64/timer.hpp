#pragma once

#include <cstdint>
#include <functional>

class Timer {
public:
    using Callback = std::function<void()>;
    
    // Initialize the timer with a callback function
    static void initialize(Callback callback, uint32_t interval_ms);
    
    // Enable or disable the timer
    static void enable();
    static void disable();
    
    // Check if preemption is enabled
    static bool is_enabled() { return enabled_; }
    
    // Set the timer interval in milliseconds
    static void set_interval(uint32_t interval_ms);
    
private:
    static bool initialized_;
    static bool enabled_;
    static uint32_t interval_ms_;
    static Callback callback_;
    
    // Platform-specific implementation
    static void platform_initialize();
    static void platform_enable();
    static void platform_disable();
    static void platform_set_interval(uint32_t interval_ms);
    
    // Signal handler for timer interrupts
    static void handle_timer(int signum);
};
