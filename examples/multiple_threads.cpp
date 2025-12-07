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

// A simple counter thread
void counter_thread(const char* name, int count) {
    for (int i = 1; i <= count; ++i) {
        printf("%s: %d\n", name, i);
        yield();
    }
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("Starting multiple threads example...\n");
    
    // Create several worker threads
    for (int i = 0; i < 3; ++i) {
        spawn([i]() { 
            worker(i, 3); 
        });
    }
    
    // Create some counter threads
    spawn([]() { counter_thread("Counter A", 5); });
    spawn([]() { counter_thread("Counter B", 4); });
    
    printf("All threads created, starting scheduler...\n");
    
    // Run the scheduler
    runtime_run();
    
    printf("All threads finished!\n");
    return 0;
}
