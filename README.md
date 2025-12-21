# Green Threads in C

A lightweight, user-space threading library implementation from scratch in C for Linux/x86_64. This project demonstrates core operating system concepts including context switching (assembly), cooperative scheduling, synchronization primitives, and asynchronous I/O.

## Features

- **User-Space Context Switching**: Implemented using x86_64 assembly (`gthread_switch`).
- **Stride Scheduling**: Deterministic proportional-share scheduler ensuring fairness (no starvation).
- **Synchronization**: Mutexes (`gmutex_t`) and Condition Variables (`gcond_t`).
- **Cooperative Multitasking**: Explicit `gthread_yield()` to switch contexts.
- **Sleep & Timers**: Efficient sleep implementation (`gthread_sleep`) with priority-queue based timer management.
- **Async I/O**: `poll()`-based event loop integration for non-blocking I/O operations (`read`, `write`, `accept`).
- **Dynamic Stack Management**: Heap-allocated stacks with lazy zombie thread reclamation.

## Project Structure

```
├── include/
│   ├── gthread.h       # Public API (Create, Yield, Join, Sleep)
│   ├── scheduler.h     # Scheduler & Stride implementation
│   ├── sync.h          # Mutex & Condition Variables
│   └── io.h            # Async I/O Wrappers
├── src/
│   ├── gthread.c       # Core thread management
│   ├── context.S       # Assembly context switch
│   ├── scheduler.c     # Scheduler logic (Min-heap ready queue)
│   ├── sync.c          # Synchronization primitives
│   └── io.c            # I/O event loop integration
├── examples/
│   ├── basic_threads.c # Simple create/join
│   ├── stride_test.c   # Scheduler fairness demonstration
│   ├── stack_test.c    # Recursion & stack usage
│   ├── mutex_test.c    # Producer-Consumer problem
│   ├── sleep_test.c    # Sleep/Wakeup timing
│   ├── io_test.c       # Async IO Echo client/server
│   ├── http_server.c   # Single-threaded async HTTP server
│   ├── matrix_mul.c    # Parallel matrix multiplication
│   └── runner.c        # Interactive launcher for all demos
└── Makefile
```

## Building and Running

### Prerequisites
- GCC
- Make
- Linux x86_64 environment (or WSL)

### Build
```bash
make
```

### Run Demos
Use the interactive runner to explore all features:
```bash
./build/runner
```

Or run individual examples:
```bash
./build/http_server
# In another terminal: curl localhost:8080
```

## Key Concepts Demonstrated

1.  **Context Switching**: Saving/Restoring `rbx`, `rsp`, `rbp`, `r12-r15` registers.
2.  **Scheduling**: Assigning 'tickets' to threads and picking the one with the lowest 'pass' value.
3.  **Green Threads vs OS Threads**: Managed entirely in user space (many green threads -> 1 OS thread).
4.  **Event Loop**: Integrating CPU-bound tasks with I/O-bound tasks in a single OS thread.

## License
MIT
