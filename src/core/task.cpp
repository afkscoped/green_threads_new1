#include "task.hpp"
#include "scheduler.hpp"
#include "../util/log.hpp"
#include <random>

// Initialize static members
std::vector<WorkStealingQueue> TaskScheduler::queues_;

TaskScheduler& TaskScheduler::instance() {
    static TaskScheduler instance;
    return instance;
}

TaskScheduler::TaskScheduler() {
    // Initialize with hardware concurrency - 1 worker threads
    // (leaving one core for the main thread and I/O)
    size_t num_workers = std::thread::hardware_concurrency();
    if (num_workers == 0) {
        num_workers = DEFAULT_THREAD_COUNT;
    } else if (num_workers > 1) {
        num_workers--; // Leave one core for I/O
    }
    
    LOG_INFO("Initializing task scheduler with %zu workers", num_workers);
    
    // Create work queues (one per worker + one for the main thread)
    queues_.resize(num_workers + 1);
    
    // Start worker threads
    start(num_workers);
}

void TaskScheduler::start(size_t thread_count) {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    LOG_DEBUG("Starting task scheduler with %zu threads", thread_count);
    
    // Create worker threads
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this, i] { worker_loop(i); });
    }
}

void TaskScheduler::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    LOG_DEBUG("Stopping task scheduler");
    
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

void TaskScheduler::worker_loop(size_t worker_id) {
    // Set thread name for debugging
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "worker-%zu", worker_id);
    
#ifdef __linux__
    pthread_setname_np(pthread_self(), thread_name);
#elif defined(__APPLE__)
    pthread_setname_np(thread_name);
#endif
    
    LOG_DEBUG("Worker %zu started", worker_id);
    
    WorkStealingQueue& local_queue = queues_[worker_id];
    
    while (running_) {
        Task task;
        
        // Try to get a task from the local queue
        if (local_queue.try_pop(task)) {
            try {
                task();
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in task: %s", e.what());
            } catch (...) {
                LOG_ERROR("Unknown exception in task");
            }
            continue;
        }
        
        // Try to steal a task from another queue
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        
        // Create a random permutation of queue indices to avoid contention
        std::vector<size_t> indices(queues_.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), gen);
        
        for (size_t i : indices) {
            if (i == worker_id) continue;
            
            if (queues_[i].try_steal(task)) {
                try {
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in stolen task: %s", e.what());
                } catch (...) {
                    LOG_ERROR("Unknown exception in stolen task");
                }
                break;
            }
        }
        
        // No tasks available, yield to other threads
        if (running_) {
            std::this_thread::yield();
        }
    }
    
    LOG_DEBUG("Worker %zu stopping", worker_id);
}

// Initialize the task scheduler on program startup
namespace {
    struct TaskSchedulerInitializer {
        TaskSchedulerInitializer() {
            TaskScheduler::instance(); // Force initialization
        }
    };
    
    TaskSchedulerInitializer task_scheduler_initializer;
} // namespace
