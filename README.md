# GreenThreads: User-Space Threading in C

A lightweight, from-scratch implementation of user-space threads (Green Threads) in C for Linux/x86_64. This project demonstrates core OS concepts including assembly-level context switching, cooperative scheduling, synchronization types, async I/O with event loops, and interactive web visualization.

## Features

- **Micro-Kernel Architecture**: Minimal core (~200 LOC assembly/C) handling context switches.
- **Stride Scheduler**: Deterministic, token-based scheduling ensuring fairness (no starvation).
- **Async I/O System**: Non-blocking `read`/`write` integrated with `poll()` event loop.
- **Synchronization**: Hand-written Mutexes (`gmutex_t`) and Condition Variables (`gcond_t`).
- **Web Dashboard**: Real-time thread visualization via an embedded HTTP server.
- **Zero External Runtime**: Depends only on `libc` and standard syscalls.

## Quick Start

### 1. Requirements

- **OS**: Linux (x86_64) or Windows (WSL2/MinGW).
- **Tools**: `gcc`, `make`.

### 2. Clone

```bash
git clone https://github.com/afkscoped/green_threads.git
cd green_threads
```

### 3. Build

Compile all core libraries and examples:
```bash
make
```

### 4. Run & Test

Use the interactive runner to verify all systems:
```bash
./build/runner
```

This launches a menu:
```text
=== Green Threads Demo Runner ===
1. Basic Threads          (Create/Join)
2. Stride Scheduling      (Fairness demo)
3. Stack Management       (Recursion tests)
4. Synchronization        (Mutex/CondVars)
5. Sleep/Timers           (Efficient wait)
6. IO Integration         (Async Socket Echo)
7. HTTP Server            (Single-threaded Async Server)
8. Matrix Multiplication  (Parallel Compute)
9. Web Dashboard          (Real-time Visualizer)
0. Exit
```

---

## Implementation Phases

This project was built in distinct phases, each adding layer of complexity:

### Phase 1: Core Context Switching
- **Goal**: Switch between two functions without returning.
- **Tech**: x86_64 Assembly (`src/context.S`) saving `rbx`, `rsp`, `rbp`, `r12-r15`.
- **Demo**: `examples/basic_threads`

### Phase 2: Thread Management
- **Goal**: Dynamic thread creation/destruction.
- **Tech**: `malloc` for stacks, `struct gthread` for metadata (State: NEW, RUNNING, DONE).

### Phase 3: Cooperative Scheduling
- **Goal**: Support `yield()` and fairness.
- **Tech**: **Stride Scheduling** algorithm. Threads have "passes" and "stride"; lowest pass runs next.

### Phase 4: Synchronization
- **Goal**: Prevent race conditions.
- **Tech**: `gmutex_t` using atomic flags and spin/yield loops; `gcond_t` for signaling.
- **Demo**: `examples/mutex_test` (Producer-Consumer).

### Phase 5: Timers & Sleep
- **Goal**: Non-blocking sleep.
- **Tech**: Priority queue (Min-Heap) of sleeping threads. Scheduler checks heap top O(1) before running tasks.
- **Demo**: `examples/sleep_test`.

### Phase 6-7: Stack Safety & Optimization
- **Goal**: Handle stack overflow/growth.
- **Tech**: Guard pages (mprotect) and dynamic stack sizing (simulated).

### Phase 8-9: Async I/O & Networking
- **Goal**: Handle network IO without blocking the whole process.
- **Tech**: `poll()` integration. `gthread_read/write` hooks that yield if `EAGAIN` is returned and register FD with scheduler.
- **Demo**: `examples/http_server`.

### Phase 10: Matrix Multiplication (Parallelism)
- **Goal**: Demonstrate CPU-bound concurrency.
- **Tech**: Splitting large matrix calc into chunks, processed by worker threads.
- **Demo**: `examples/matrix_mul`.

### Phase 11: Web Dashboard (Visualization)
- **Goal**: Visualize the runtime state.
- **Tech**: Embedded single-threaded HTTP server serving a lightweight HTML/JS frontend. Polls internal `monitor` API for thread states.
- **Demo**: `examples/web_dashboard`.

---

## Web Dashboard Usage

1. Select **Option 9** in the runner or run `./build/web_dashboard`.
2. Open [http://localhost:8080](http://localhost:8080).
3. Use the **Spawn** buttons to generate load.
4. **Exit**: Press `Ctrl+C` in the terminal to stop the server.

---

## License
MIT
