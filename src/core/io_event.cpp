#include "io_event.hpp"
#include "scheduler.hpp"
#include "../util/assert.hpp"
#include "../util/log.hpp"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/eventfd.h>

// Convert our event type to EPOLL events
static uint32_t to_epoll_events(IOEvent::Type type) {
    uint32_t events = 0;
    if (static_cast<int>(type) & static_cast<int>(IOEvent::Type::READ)) {
        events |= EPOLLIN | EPOLLRDHUP;
    }
    if (static_cast<int>(type) & static_cast<int>(IOEvent::Type::WRITE)) {
        events |= EPOLLOUT;
    }
    if (static_cast<int>(type) & static_cast<int>(IOEvent::Type::ERROR)) {
        events |= EPOLLERR | EPOLLHUP;
    }
    return events;
}

// Convert EPOLL events to our event type
static IOEvent::Type from_epoll_events(uint32_t events) {
    int result = 0;
    if (events & (EPOLLIN | EPOLLRDHUP)) {
        result |= static_cast<int>(IOEvent::Type::READ);
    }
    if (events & EPOLLOUT) {
        result |= static_cast<int>(IOEvent::Type::WRITE);
    }
    if (events & (EPOLLERR | EPOLLHUP)) {
        result |= static_cast<int>(IOEvent::Type::ERROR);
    }
    return static_cast<IOEvent::Type>(result);
}

IOEvent::IOEvent() : running_(false) {
    // Create epoll instance
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    
    // Create control pipe for stopping the event loop
    if (pipe2(control_pipe_, O_NONBLOCK | O_CLOEXEC) == -1) {
        close(epoll_fd_);
        throw std::runtime_error("Failed to create control pipe");
    }
    
    // Add control pipe to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = control_pipe_[0];
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, control_pipe_[0], &ev) == -1) {
        close(control_pipe_[0]);
        close(control_pipe_[1]);
        close(epoll_fd_);
        throw std::runtime_error("Failed to add control pipe to epoll");
    }
    
    LOG_DEBUG("IOEvent system initialized");
}

IOEvent::~IOEvent() {
    stop();
    
    // Close control pipe
    if (control_pipe_[0] != -1) {
        close(control_pipe_[0]);
        close(control_pipe_[1]);
    }
    
    // Close epoll fd
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    
    LOG_DEBUG("IOEvent system shutdown");
}

IOEvent& IOEvent::instance() {
    static IOEvent instance;
    return instance;
}

void IOEvent::register_event(int fd, Type type, Callback callback) {
    if (fd < 0) {
        throw std::invalid_argument("Invalid file descriptor");
    }
    
    std::lock_guard<Mutex> lock(events_mutex_);
    
    // Check if we already have this fd
    auto it = events_.find(fd);
    if (it == events_.end()) {
        // New fd, add to epoll
        struct epoll_event ev;
        ev.events = to_epoll_events(type);
        ev.data.fd = fd;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            throw std::runtime_error("Failed to add fd to epoll");
        }
        
        // Initialize event data
        EventData data;
        if (type == Type::READ) data.read_cb = std::move(callback);
        else if (type == Type::WRITE) data.write_cb = std::move(callback);
        else if (type == Type::ERROR) data.error_cb = std::move(callback);
        
        events_.emplace(fd, std::move(data));
    } else {
        // Existing fd, update epoll
        EventData& data = it->second;
        uint32_t events = 0;
        
        // Update callbacks
        if (type == Type::READ) data.read_cb = std::move(callback);
        else if (type == Type::WRITE) data.write_cb = std::move(callback);
        else if (type == Type::ERROR) data.error_cb = std::move(callback);
        
        // Calculate new event mask
        if (data.read_cb) events |= to_epoll_events(Type::READ);
        if (data.write_cb) events |= to_epoll_events(Type::WRITE);
        if (data.error_cb) events |= to_epoll_events(Type::ERROR);
        
        // Update epoll
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
            throw std::runtime_error("Failed to modify fd in epoll");
        }
    }
    
    LOG_DEBUG("Registered event: fd=%d, type=%d", fd, static_cast<int>(type));
}

void IOEvent::unregister_event(int fd, Type type) {
    std::lock_guard<Mutex> lock(events_mutex_);
    
    auto it = events_.find(fd);
    if (it == events_.end()) {
        return; // Not found
    }
    
    EventData& data = it->second;
    
    // Clear the appropriate callback
    if (type == Type::READ) data.read_cb = nullptr;
    else if (type == Type::WRITE) data.write_cb = nullptr;
    else if (type == Type::ERROR) data.error_cb = nullptr;
    
    // If no more callbacks, remove from epoll and events_
    if (!data.read_cb && !data.write_cb && !data.error_cb) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        events_.erase(it);
        LOG_DEBUG("Unregistered all events for fd=%d", fd);
    } else {
        // Update epoll with new event mask
        uint32_t events = 0;
        if (data.read_cb) events |= to_epoll_events(Type::READ);
        if (data.write_cb) events |= to_epoll_events(Type::WRITE);
        if (data.error_cb) events |= to_epoll_events(Type::ERROR);
        
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
            throw std::runtime_error("Failed to modify fd in epoll");
        }
        
        LOG_DEBUG("Unregistered event: fd=%d, type=%d", fd, static_cast<int>(type));
    }
}

void IOEvent::run() {
    if (running_) {
        return; // Already running
    }
    
    running_ = true;
    
    while (running_) {
        process_events(1000); // 1 second timeout
    }
}

void IOEvent::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Write to control pipe to wake up epoll_wait
    uint64_t value = 1;
    write(control_pipe_[1], &value, sizeof(value));
}

int IOEvent::process_events(int timeout_ms) {
    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];
    
    int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout_ms);
    if (num_events == -1) {
        if (errno == EINTR) {
            return 0; // Interrupted by signal
        }
        throw std::runtime_error("epoll_wait failed");
    }
    
    for (int i = 0; i < num_events; ++i) {
        if (events[i].data.fd == control_pipe_[0]) {
            // Control event, read the data to clear it
            uint64_t value;
            read(control_pipe_[0], &value, sizeof(value));
            continue;
        }
        
        process_event(events[i]);
    }
    
    return num_events;
}

void IOEvent::process_event(const epoll_event& event) {
    int fd = event.data.fd;
    uint32_t events = event.events;
    
    // Find the callbacks for this fd
    std::lock_guard<Mutex> lock(events_mutex_);
    auto it = events_.find(fd);
    if (it == events_.end()) {
        return; // No callbacks registered
    }
    
    EventData& data = it->second;
    
    // Process the events
    if ((events & EPOLLIN) && data.read_cb) {
        data.read_cb(fd, Type::READ);
    }
    
    if ((events & EPOLLOUT) && data.write_cb) {
        data.write_cb(fd, Type::WRITE);
    }
    
    if (((events & EPOLLERR) || (events & EPOLLHUP)) && data.error_cb) {
        data.error_cb(fd, Type::ERROR);
    }
}

bool IOEvent::set_nonblocking(int fd, bool nonblocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    return fcntl(fd, F_SETFL, flags) != -1;
}
