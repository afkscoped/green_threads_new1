#pragma once

#include <functional>
#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "context.hpp"
#include "scheduler.hpp"

class Task {
public:
    using Func = std::function<void()>;
    
    explicit Task(Func func) : func_(std::move(func)) {}
    
    void operator()() const {
        if (func_) {
            func_();
        }
    }
    
    bool valid() const { return static_cast<bool>(func_); }
    
private:
    Func func_;
};

class WorkStealingQueue {
public:
    void push(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push_back(std::move(task));
        cv_.notify_one();
    }
    
    bool try_pop(Task& task) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (tasks_.empty()) {
            return false;
        }
        
        task = std::move(tasks_.front());
        tasks_.pop_front();
        return true;
    }
    
    bool try_steal(Task& task) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (tasks_.empty()) {
            return false;
        }
        
        task = std::move(tasks_.back());
        tasks_.pop_back();
        return true;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }
    
    void wait_for_task() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tasks_.empty()) {
            cv_.wait(lock, [this] { return !tasks_.empty(); });
        }
    }
    
private:
    std::deque<Task> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

class TaskScheduler {
public:
    static const size_t DEFAULT_THREAD_COUNT = 4;
    
    static TaskScheduler& instance() {
        static TaskScheduler instance;
        return instance;
    }
    
    ~TaskScheduler() {
        stop();
    }
    
    void start(size_t thread_count = DEFAULT_THREAD_COUNT) {
        if (running_.exchange(true)) {
            return; // Already running
        }
        
        // Create worker threads
        for (size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this, i] { worker_loop(i); });
        }
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }
        
        // Notify all workers to stop
        for (auto& queue : queues_) {
            queue.push(Task{[] {}}); // Empty task as stop signal
        }
        
        // Wait for workers to finish
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
        queues_.clear();
    }
    
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args)...);
            }
        );
        
        std::future<ReturnType> result = task->get_future();
        
        // Select a random queue to submit the task
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, queues_.size() - 1);
        
        size_t queue_idx = dist(gen);
        queues_[queue_idx].push(Task([task = std::move(task)] { (*task)(); }));
        
        return result;
    }
    
private:
    TaskScheduler() = default;
    
    void worker_loop(size_t worker_id) {
        // Set thread name for debugging
        std::string thread_name = "Worker-" + std::to_string(worker_id);
        pthread_setname_np(pthread_self(), thread_name.c_str());
        
        WorkStealingQueue& local_queue = queues_[worker_id];
        
        while (running_) {
            Task task;
            
            // Try to get a task from the local queue
            if (local_queue.try_pop(task)) {
                task();
                continue;
            }
            
            // Try to steal a task from another queue
            for (size_t i = 0; i < queues_.size(); ++i) {
                if (i == worker_id) continue;
                
                if (queues_[i].try_steal(task)) {
                    task();
                    break;
                }
            }
            
            // No tasks available, wait for a new task
            if (running_) {
                local_queue.wait_for_task();
            }
        }
    }
    
    std::vector<std::thread> workers_;
    std::vector<WorkStealingQueue> queues_;
    std::atomic<bool> running_{false};
};

// Helper function to submit tasks
template <typename F, typename... Args>
auto async(F&& f, Args&&... args) {
    return TaskScheduler::instance().submit(
        std::forward<F>(f), std::forward<Args>(args)...
    );
}

// Parallel for loop
template <typename Range, typename Func>
void parallel_for(Range&& range, Func&& func) {
    using value_type = decltype(*std::begin(range));
    
    auto begin = std::begin(range);
    auto end = std::end(range);
    size_t size = std::distance(begin, end);
    
    if (size == 0) {
        return;
    }
    
    // For small ranges, just execute sequentially
    if (size <= 1) {
        for (auto it = begin; it != end; ++it) {
            func(*it);
        }
        return;
    }
    
    // For larger ranges, split the work
    const size_t min_chunk_size = 1000;
    const size_t num_chunks = std::min(
        size / min_chunk_size + 1,
        static_cast<size_t>(TaskScheduler::DEFAULT_THREAD_COUNT * 2)
    );
    
    const size_t chunk_size = (size + num_chunks - 1) / num_chunks;
    
    std::vector<std::future<void>> futures;
    futures.reserve(num_chunks);
    
    for (size_t i = 0; i < num_chunks; ++i) {
        auto chunk_begin = begin + (i * chunk_size);
        auto chunk_end = (i == num_chunks - 1) ? end : chunk_begin + chunk_size;
        
        futures.push_back(async([=, &func] {
            for (auto it = chunk_begin; it != chunk_end; ++it) {
                func(*it);
            }
        }));
    }
    
    // Wait for all chunks to complete
    for (auto& future : futures) {
        future.wait();
    }
}
