#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <unistd.h>

void worker(int id) {
    for (int i = 0; i < 3; ++i) {
        printf("Thread %d: step %d\n", id, i + 1);
        // Explicitly yield to other threads
        yield();
        
        // Simulate some work
        for (volatile int j = 0; j < 1000000; ++j) {}
    }
    printf("Thread %d: finished\n", id);
}

void thread1() {
    worker(1);
}

void thread2() {
    worker(2);
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("Main thread started\n");
    
    // Create two green threads
    spawn(thread1);
    spawn(thread2);
    
    printf("Threads created, starting scheduler...\n");
    
    // Run the scheduler
    runtime_run();
    
    printf("All threads finished, back in main thread\n");
    
    return 0;
}
