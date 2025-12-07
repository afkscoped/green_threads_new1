#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "mutex.hpp"
#include "condition_variable.hpp"

class IOEvent {
public:
    enum class Type {
        READ = 0x01,
        WRITE = 0x02,
        ERROR = 0x04
    };
    
    using Callback = std::function<void(int fd, Type type)>;
    
    // Create an IOEvent instance (singleton)
    static IOEvent& instance();
    
    // Register a callback for I/O events
    void register_event(int fd, Type type, Callback callback);
    
    // Unregister an I/O event
    void unregister_event(int fd, Type type);
    
    // Run the event loop (blocks until stop() is called)
    void run();
    
    // Stop the event loop
    void stop();
    
    // Process events with a timeout (in milliseconds)
    // Returns the number of events processed
    int process_events(int timeout_ms = -1);
    
    // Make file descriptor non-blocking
    static bool set_nonblocking(int fd, bool nonblocking = true);
    
private:
    IOEvent();
    ~IOEvent();
    
    // Prevent copying
    IOEvent(const IOEvent&) = delete;
    IOEvent& operator=(const IOEvent&) = delete;
    
    // Event data structure
    struct EventData {
        Callback read_cb;
        Callback write_cb;
        Callback error_cb;
    };
    
    // File descriptor for epoll
    int epoll_fd_;
    
    // Event data storage
    std::unordered_map<int, EventData> events_;
    Mutex events_mutex_;
    
    // Control pipe for stopping the event loop
    int control_pipe_[2];
    bool running_;
    
    // Process a single event
    void process_event(const epoll_event& event);
};

// Helper class for RAII-style I/O event registration
class IOEventGuard {
public:
    IOEventGuard() = default;
    
    ~IOEventGuard() {
        unregister_all();
    }
    
    // Register an I/O event
    void register_event(int fd, IOEvent::Type type, IOEvent::Callback callback) {
        IOEvent::instance().register_event(fd, type, callback);
        registrations_.push_back({fd, type});
    }
    
    // Unregister all events
    void unregister_all() {
        for (const auto& reg : registrations_) {
            IOEvent::instance().unregister_event(reg.fd, reg.type);
        }
        registrations_.clear();
    }
    
    // Non-copyable
    IOEventGuard(const IOEventGuard&) = delete;
    IOEventGuard& operator=(const IOEventGuard&) = delete;
    
private:
    struct Registration {
        int fd;
        IOEvent::Type type;
    };
    
    std::vector<Registration> registrations_;
};
