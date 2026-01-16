# Green Threads Library

A lightweight, high-performance user-space threading library in C. This project demonstrates the implementation of M:1 cooperative concurrency (Green Threads) from scratch, featuring a custom scheduler, synchronization primitives, async I/O, and a real-time web dashboard.

## Features

- **Core Threading**: Custom `gthread_t` structures with heap-allocated stacks (64KB default).
- **Fast Context Switching**: Assembly-level (`context.S`) switching for minimal overhead.
- **Stride Scheduler**: Fair, deterministic scheduling using a Min-Heap priority queue. Threads are assigned "tickets" to control their CPU share.
- **Synchronization**:
  - `gmutex`: Mutex locks for critical sections.
  - `gcond`: Condition variables for signaling.
- **Async I/O System**: Non-blocking `read`/`write`/`accept` wrappers integrated with a system `poll` loop, enabling high-concurrency networking.
- **Sleep & Timers**: Efficient O(1) sleep queue handling for `gthread_sleep`.
- **Integrated Web Dashboard**: A real-time monitoring dashboard (port 8080) to visualize thread states, stacks, and scheduling metrics.
- **Interactive Demos**: Five fully interactive demos showcasing scheduling, memory, sync, and I/O.

---

## Getting Started

### Prerequisites
- GCC Compiler
- Make
- Linux/WSL (Required for strict context switching ABI)

### Build
To build the library and all examples:
```bash
make clean && make
```

### Run
The easiest way to explore is the **Runner**:
```bash
./build/runner
```
This interactive menu allows you to launch all demos and the web dashboard.

---

## Interactive Demos

All demos now feature **Startup Configuration**, allowing you to set parameters before execution.

### 1. Stride Scheduling Test (Runner Option 2)
Tests the fairness of the scheduler.
- **Input**: Number of threads, and "Tickets" for each thread.
- **Behavior**: Threads with more tickets get more CPU time.
- **Output**: Prints progress every 10% of workload.

### 2. Stack Management (Runner Option 3)
Tests deep recursion handling.
- **Input**: Recursion depth (e.g., 40 for approx 40KB stack).
- **Behavior**: Spawns a thread that eats stack space to verify safety limits.

### 3. Synchronization (Runner Option 4)
Tests Mutex and Condition Variables (Producer-Consumer).
- **Input**: Number of Producers and Consumers.
- **Behavior**: Threads contend for a shared buffer, locking/unlocking and signaling safely.

### 4. Sleep/Timers (Runner Option 5)
Tests the sleep queue accuracy.
- **Input**: Sleep duration (ms) for varying threads.
- **Behavior**: Threads sleep efficiently without blocking the scheduler and wake up on time.

### 5. I/O Scalability (Runner Option 6)
Stress tests the Async I/O reactor.
- **Input**: Number of concurrent clients (e.g., 100).
- **Behavior**: Spawns N clients that connect to a Green Thread server, exchange data, and disconnect.

---

## Web Dashboard

The project includes a visualization tool.
1. Run **Option 9** from the runner (or `./build/web_dashboard`).
2. Open `http://localhost:8080` in your browser.
3. You will see a table of live threads with metrics:
   - **ID**: Thread ID.
   - **State**: RUNNING, READY, BLOCKED, DONE.
   - **Tickets**: Priority weight.
   - **Stack Used**: Current stack memory usage.
   - **Wait Time**: Next wake-up time (if sleeping).

---

## Project Structure

```text
.
├── src/
│   ├── gthread.c       # Core thread creation/init
│   ├── scheduler.c     # Stride scheduler & Min-heap
│   ├── context.S       # Assembly context switch
│   ├── sync.c          # Mutex & Condition vars
│   ├── io.c            # Async I/O wrappers & Poll
│   └── dashboard.c     # Web server for metrics
├── include/            # Public headers
├── examples/           # Demo applications
│   ├── runner.c        # Main menu
│   ├── stride_test.c   # Scheduling fairnes demo
│   └── ...
└── Makefile            # Build system
```

## Architecture Notes

- **M:1 Model**: All green threads run on a single OS thread. This eliminates race conditions on global CPU data but means blocking system calls (like `scanf`) block *all* threads. We solve this by using non-blocking I/O wrappers.
- **Stride Scheduling**: We use a `pass` value that increments by `stride` (inverse of tickets). The thread with the lowest `pass` is always picked next.
- **Stack Safety**: Stacks are `malloc`'d. Infinite recursion will overflow the allocated heap block (64KB), potentially crashing the program (Guard pages are planned).

---

*Authored by Antigravity Agent*
