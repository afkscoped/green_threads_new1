#include "../src/core/mutex.hpp"
#include "../src/core/condition_variable.hpp"
#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <vector>
#include <chrono>

using namespace std::chrono_literals;

// Thread-safe queue using mutex and condition variable
template<typename T, size_t Capacity>
class ThreadSafeQueue {
public:
    void push(T value) {
        Mutex::Lock lock(mutex_);
        
        // Wait until there's space in the queue
        while (queue_.size() >= Capacity) {
            not_full_.wait(mutex_);
        }
        
        // Add the item
        queue_.push_back(value);
        
        // Notify consumers that an item is available
        not_empty_.notify_one();
    }
    
    T pop() {
        Mutex::Lock lock(mutex_);
        
        // Wait until there's an item in the queue
        while (queue_.empty()) {
            not_empty_.wait(mutex_);
        }
        
        // Remove and return the item
        T value = queue_.front();
        queue_.erase(queue_.begin());
        
        // Notify producers that space is available
        not_full_.notify_one();
        
        return value;
    }
    
    bool try_pop(T& value) {
        Mutex::Lock lock(mutex_);
        
        if (queue_.empty()) {
            return false;
        }
        
        value = queue_.front();
        queue_.erase(queue_.begin());
        
        not_full_.notify_one();
        return true;
    }
    
    size_t size() const {
        Mutex::Lock lock(mutex_);
        return queue_.size();
    }

private:
    std::vector<T> queue_;
    mutable Mutex mutex_;
    ConditionVariable not_empty_;
    ConditionVariable not_full_;
};

// Global queue for producer-consumer example
ThreadSafeQueue<int, 5> g_queue;

// Producer function
void producer(int id, int items_to_produce) {
    for (int i = 0; i < items_to_produce; ++i) {
        // Produce an item
        int item = id * 1000 + i;
        
        // Simulate some work
        for (volatile int j = 0; j < 1000000; ++j) {}
        
        // Add item to queue
        printf("Producer %d: Producing item %d\n", id, item);
        g_queue.push(item);
        
        // Occasionally yield to show preemption
        if (i % 2 == 0) {
            yield();
        }
    }
    
    printf("Producer %d: Done!\n", id);
}

// Consumer function
void consumer(int id, int items_to_consume) {
    int consumed = 0;
    
    while (consumed < items_to_consume) {
        // Get an item from the queue
        int item = g_queue.pop();
        
        // Process the item
        printf("Consumer %d: Consumed item %d\n", id, item);
        
        // Simulate some work
        for (volatile int j = 0; j < 1500000; ++j) {}
        
        consumed++;
        
        // Occasionally yield to show preemption
        if (consumed % 3 == 0) {
            yield();
        }
    }
    
    printf("Consumer %d: Done!\n", id);
}

// Test for mutex and condition variable
void test_mutex_cv() {
    printf("=== Testing Mutex and Condition Variable ===\n");
    
    Mutex mutex;
    ConditionVariable cv;
    bool ready = false;
    
    // Worker thread that waits for a condition
    Thread* worker = new Thread([&]() {
        printf("Worker: Waiting for condition...\n");
        
        {
            Mutex::Lock lock(mutex);
            while (!ready) {
                cv.wait(mutex);
            }
        }
        
        printf("Worker: Condition met, continuing...\n");
    });
    
    worker->set_name("Worker");
    
    // Main thread signals the condition after a delay
    Thread* main_thread = new Thread([&]() {
        printf("Main: Working for 1 second...\n");
        
        // Simulate work
        for (volatile int i = 0; i < 500000000; ++i) {}
        
        {
            Mutex::Lock lock(mutex);
            printf("Main: Signaling condition...\n");
            ready = true;
            cv.notify_one();
        }
        
        printf("Main: Done\n");
    });
    
    main_thread->set_name("Main");
    
    // Schedule both threads
    Scheduler::schedule(worker);
    Scheduler::schedule(main_thread);
    
    // Run until all threads complete
    runtime_run();
    
    delete worker;
    delete main_thread;
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("=== Synchronization Primitives Example ===\n\n");
    
    // Test 1: Mutex and Condition Variable
    test_mutex_cv();
    
    // Test 2: Producer-Consumer with Bounded Buffer
    printf("\n=== Testing Producer-Consumer ===\n");
    
    // Create producers
    const int num_producers = 2;
    const int items_per_producer = 5;
    
    for (int i = 0; i < num_producers; ++i) {
        Thread* t = new Thread([i]() {
            producer(i + 1, items_per_producer);
        });
        t->set_name(("Producer " + std::to_string(i + 1)).c_str());
        Scheduler::schedule(t);
    }
    
    // Create consumers
    const int num_consumers = 3;
    const int items_per_consumer = (num_producers * items_per_producer) / num_consumers;
    
    for (int i = 0; i < num_consumers; ++i) {
        Thread* t = new Thread([i]() {
            consumer(i + 1, items_per_consumer);
        });
        t->set_name(("Consumer " + std::to_string(i + 1)).c_str());
        Scheduler::schedule(t);
    }
    
    // Run the scheduler
    runtime_run();
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
