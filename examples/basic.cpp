#include "../src/runtime/runtime.hpp"
#include <cstdio>

void thread_func() {
    printf("Hello from thread!\n");
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("Main thread started\n");
    
    // Create a new green thread
    spawn(thread_func);
    
    // Run the scheduler
    runtime_run();
    
    printf("Back in main thread\n");
    
    return 0;
}
