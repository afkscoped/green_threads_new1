#include "timer.hpp"
#include "../../core/context.hpp"
#include "../../core/scheduler.hpp"
#include <signal.h>
#include <sys/time.h>
#include <cstring>
#include <stdexcept>

// Static member initialization
bool Timer::initialized_ = false;
bool Timer::enabled_ = false;
uint32_t Timer::interval_ms_ = 10; // Default 10ms timeslice
Timer::Callback Timer::callback_;

/* static */ void Timer::initialize(Callback callback, uint32_t interval_ms) {
    if (initialized_) {
        throw std::runtime_error("Timer already initialized");
    }
    
    callback_ = std::move(callback);
    interval_ms_ = interval_ms;
    
    // Set up signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = [](int, siginfo_t*, void*) {
        if (enabled_ && callback_) {
            callback_();
        }
    };
    
    if (sigaction(SIGALRM, &sa, nullptr) == -1) {
        throw std::runtime_error("Failed to set up signal handler");
    }
    
    // Set up the timer
    struct itimerval timer;
    timer.it_interval.tv_sec = interval_ms_ / 1000;
    timer.it_interval.tv_usec = (interval_ms_ % 1000) * 1000;
    timer.it_value = timer.it_interval;
    
    if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
        throw std::runtime_error("Failed to set up interval timer");
    }
    
    initialized_ = true;
}

/* static */ void Timer::enable() {
    if (!initialized_) {
        throw std::runtime_error("Timer not initialized");
    }
    enabled_ = true;
}

/* static */ void Timer::disable() {
    enabled_ = false;
}

/* static */ void Timer::set_interval(uint32_t interval_ms) {
    interval_ms_ = interval_ms;
    
    if (initialized_) {
        struct itimerval timer;
        timer.it_interval.tv_sec = interval_ms_ / 1000;
        timer.it_interval.tv_usec = (interval_ms_ % 1000) * 1000;
        timer.it_value = timer.it_interval;
        
        if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
            throw std::runtime_error("Failed to update timer interval");
        }
    }
}
