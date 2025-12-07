#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

// A simple worker that does work with sleeps
void worker(int id, int count) {
    printf("Worker %d: Starting, will run %d times\n", id, count);
    
    for (int i = 1; i <= count; ++i) {
        printf("Worker %d: Working %d/%d\n", id, i, count);
        
        // Simulate some work
        for (volatile int j = 0; j < 1000000; ++j) {}
        
        // Sleep for a while
        if (i < count) {  // Don't sleep after last iteration
            printf("Worker %d: Sleeping for %d ms...\n", id, 100 * id);
            sleep_for(std::chrono::milliseconds(100 * id));
        }
    }
    
    printf("Worker %d: Done!\n", id);
}

// A function that demonstrates nested sleeps
void nested_sleeper(int depth, int max_depth) {
    if (depth > max_depth) {
        printf("Reached max depth %d!\n", max_depth);
        return;
    }
    
    printf("Depth %d: Sleeping for %d ms...\n", depth, 100 * depth);
    sleep_for(std::chrono::milliseconds(100 * depth));
    
    // Create a new thread at each level
    if (depth < max_depth) {
        Thread* t = new Thread([depth, max_depth]() {
            nested_sleeper(depth + 1, max_depth);
        });
        t->set_name(("Nested " + std::to_string(depth)).c_str());
        schedule(t);
    }
    
    printf("Depth %d: Resumed after sleep\n", depth);
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("=== Sleep Example ===\n\n");
    
    // Test 1: Basic sleep
    printf("Test 1: Basic Sleep\n");
    spawn([]() {
        printf("Thread will sleep for 500ms...\n");
        auto start = std::chrono::steady_clock::now();
        sleep_for(500ms);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        printf("Slept for %lld ms (requested 500ms)\n", static_cast<long long>(duration));
    });
    
    // Let the first test complete
    runtime_run();
    
    // Test 2: Multiple workers with different sleep times
    printf("\nTest 2: Multiple Workers\n");
    for (int i = 1; i <= 3; ++i) {
        Thread* t = new Thread([i]() { worker(i, 3); });
        t->set_name(("Worker " + std::to_string(i)).c_str());
        schedule(t);
    }
    
    // Let the workers complete
    runtime_run();
    
    // Test 3: Nested sleeps
    printf("\nTest 3: Nested Sleeps\n");
    Thread* nested = new Thread([]() { nested_sleeper(1, 3); });
    nested->set_name("Nested Main");
    schedule(nested);
    
    // Let the nested test complete
    runtime_run();
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
