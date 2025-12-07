#include "../src/runtime/runtime.hpp"
#include <cstdio>
#include <memory>

// A simple resource that needs cleanup
class Resource {
    const char* name_;
    bool* freed_;

public:
    Resource(const char* name, bool* freed) : name_(name), freed_(freed) {
        printf("  [%s] Resource acquired\n", name_);
        *freed_ = false;
    }

    ~Resource() {
        printf("  [%s] Resource released\n", name_);
        *freed_ = true;
    }

    void use() {
        printf("  [%s] Resource in use\n", name_);
    }
};

// A thread that uses RAII for cleanup
void resource_user() {
    printf("Thread %lu: Starting resource user\n", Thread::current()->id());
    
    // These will be automatically cleaned up when the thread exits
    bool res1_freed = false;
    bool res2_freed = false;
    
    {
        Resource res1("Res1", &res1_freed);
        Resource res2("Res2", &res2_freed);
        
        // Use resources
        res1.use();
        res2.use();
        
        printf("Thread %lu: Yielding...\n", Thread::current()->id());
        yield();
        
        // Resources are still valid after yield
        res1.use();
        res2.use();
        
        printf("Thread %lu: About to exit\n", Thread::current()->id());
    } // Resources go out of scope here and are destroyed
    
    // Verify resources were cleaned up
    if (res1_freed && res2_freed) {
        printf("Thread %lu: Resources properly cleaned up!\n", Thread::current()->id());
    } else {
        printf("Thread %lu: RESOURCE LEAK DETECTED!\n", Thread::current()->id());
    }
}

// A thread that requests exit from another thread
void exit_request_worker() {
    printf("Thread %lu: Worker starting...\n", Thread::current()->id());
    
    int count = 0;
    while (true) {
        printf("Thread %lu: Working %d...\n", Thread::current()->id(), ++count);
        
        // Check for exit request
        if (Thread::current()->should_exit()) {
            printf("Thread %lu: Received exit request, cleaning up...\n", Thread::current()->id());
            break;
        }
        
        yield();
    }
    
    printf("Thread %lu: Exiting cleanly\n", Thread::current()->id());
}

// A thread that requests another thread to exit
void exit_requester(Thread* target) {
    printf("Thread %lu: Will request thread %lu to exit\n", 
           Thread::current()->id(), target->id());
    
    // Simulate some work
    for (int i = 0; i < 3; ++i) {
        printf("Thread %lu: Working %d/3\n", Thread::current()->id(), i + 1);
        yield();
    }
    
    // Request the target thread to exit
    printf("Thread %lu: Requesting thread %lu to exit\n", 
           Thread::current()->id(), target->id());
    target->request_exit();
    
    // Give it some time to exit
    for (int i = 0; i < 2; ++i) {
        printf("Thread %lu: Waiting for thread %lu to exit...\n", 
               Thread::current()->id(), target->id());
        yield();
    }
    
    printf("Thread %lu: Done\n", Thread::current()->id());
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    printf("=== Thread Cleanup Example ===\n\n");
    
    // Test 1: RAII cleanup
    printf("Test 1: RAII Resource Cleanup\n");
    spawn(resource_user);
    
    // Let the resource_user thread run
    for (int i = 0; i < 5; ++i) {
        yield();
    }
    
    // Test 2: Request exit
    printf("\nTest 2: Request Thread Exit\n");
    Thread* worker = new Thread(exit_request_worker);
    worker->set_name("Worker");
    
    Thread* requester = new Thread([worker]() { exit_requester(worker); });
    requester->set_name("Requester");
    
    // Schedule both threads
    schedule(worker);
    schedule(requester);
    
    // Run the scheduler
    runtime_run();
    
    // Clean up
    delete worker;
    delete requester;
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
