#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <chrono>
#include <thread>

// Simple worker function that runs in a thread
void worker(int id, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        printf("Thread %d: Working %d/%d\n", id, i + 1, iterations);
        
        // Simulate some work
        for (volatile int j = 0; j < 1000000; ++j) {}
        
        // Yield to other threads
        yield();
    }
    printf("Thread %d: Finished!\n", id);
}

// Wrappers for worker with fixed IDs so we can use plain function pointers
void worker0() { worker(0, 3); }
void worker1() { worker(1, 3); }
void worker2() { worker(2, 3); }

// A simple counter thread
void counter_thread(const char* name, int count) {
    for (int i = 1; i <= count; ++i) {
        printf("%s: %d\n", name, i);
        yield();
    }
}

// Wrapper functions for counter threads to use with spawn
void counter_threadA() { counter_thread("Counter A", 5); }
void counter_threadB() { counter_thread("Counter B", 4); }

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("Starting multiple threads example...\n");
    
    // Create several worker threads
    spawn(worker0);
    spawn(worker1);
    spawn(worker2);
    
    // Create some counter threads using fixed wrappers
    spawn(counter_threadA);
    spawn(counter_threadB);
    
    printf("All threads created, starting scheduler...\n");
    
    // Run the scheduler
    runtime_run();
    
    printf("All threads finished!\n");
    return 0;
}
