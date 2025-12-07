#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <chrono>
#include <thread>
#include <atomic>

using namespace std::chrono_literals;

// Shared counter for testing preemption
std::atomic<int> shared_counter{0};

// A worker that increments a counter in a loop
void worker(int id, int iterations) {
    printf("Worker %d: Starting %d iterations\n", id, iterations);
    
    for (int i = 0; i < iterations; ++i) {
        // Simulate some work
        for (volatile int j = 0; j < 1000000; ++j) {}
        
        // Increment shared counter
        shared_counter++;
        
        printf("Worker %d: Counter = %d\n", id, shared_counter.load());
    }
    
    printf("Worker %d: Done!\n", id);
}

// A function that demonstrates preemption with critical sections
void critical_section_worker(int id, int iterations) {
    printf("Critical Section Worker %d: Starting\n", id);
    
    for (int i = 0; i < iterations; ++i) {
        // Enter critical section (disable preemption)
        Scheduler::disable_preemption();
        
        printf("Worker %d: Entered critical section\n", id);
        
        // Simulate critical section work
        int old_val = shared_counter.load();
        std::this_thread::sleep_for(50ms); // Simulate work
        shared_counter.store(old_val + 1);
        
        printf("Worker %d: Counter updated to %d\n", id, shared_counter.load());
        
        // Exit critical section (re-enable preemption)
        Scheduler::enable_preemption();
        
        // Yield to show preemption works outside critical section
        yield();
    }
    
    printf("Worker %d: Done with critical sections\n", id);
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("=== Preemption Example ===\n\n");
    
    // Set a small timeslice for frequent preemption
    Scheduler::set_timeslice(5ms);
    
    // Test 1: Basic preemption
    printf("Test 1: Basic Preemption\n");
    for (int i = 1; i <= 3; ++i) {
        Thread* t = new Thread([i]() { worker(i, 5); });
        t->set_name(("Worker " + std::to_string(i)).c_str());
        schedule(t);
    }
    
    // Run the first test
    runtime_run();
    
    // Reset counter for next test
    shared_counter = 0;
    
    // Test 2: Critical sections with preemption
    printf("\nTest 2: Critical Sections\n");
    for (int i = 1; i <= 2; ++i) {
        Thread* t = new Thread([i]() { critical_section_worker(i, 3); });
        t->set_name(("CS Worker " + std::to_string(i)).c_str());
        schedule(t);
    }
    
    // Run the second test
    runtime_run();
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
