# Green Threads from Scratch

A fully functional, educational userspace threading library implemented in C from the ground up, featuring advanced scheduling, synchronization, async I/O, and a real-time web dashboard.

## 🚀 Features

*   **Userspace Context Switching**: Custom assembly implementation for switching execution contexts (stacks & registers) without kernel involvement.
*   **M:1 Threading Model**: Maps multiple green threads to a single OS thread.
*   **Advanced Scheduler**:
    *   **Stride Scheduling**: Proportional CPU sharing using "tickets".
    *   **Priority Queue**: Efficient O(log n) task selection.
    *   **Idle Task**: Handles CPU idle states properly.
*   **Synchronization Primitives**:
    *   **Mutexes**: Protecting critical sections with blocking wait queues (no busy waiting).
    *   **Condition Variables**: Wait and Signal mechanisms properly integrated with the scheduler.
*   **Asynchronous I/O**:
    *   Non-blocking system call wrappers (`read`, `write`, `accept`, `sleep`).
    *   `poll()`-based event loop woven directly into the scheduler.
    *   No blocking syscalls allowed!
*   **Full Networking Stack**:
    *   Green-threaded HTTP Server.
    *   Handle hundreds of concurrent connections.
*   **Parallel Computing Demos**:
    *   Matrix Multiplication, Merge Sort, Prime Sieve.
*   **📊 Real-Time Web Dashboard (Port 9090)**:
    *   Visualizes thread states (RUNNING, READY, BLOCKED, SLEEPING).
    *   **Stride Panel**: Adjust tickets dynamically and watch CPU share change.
    *   **Stack Panel**: Stack usage visualization bars.
    *   **Sync Panel**: See threads waiting on mutexes in real-time.
    *   **I/O Panel**: Monitor file descriptors blocking threads.

---

## 🛠️ Build & Run

### Prerequisites
*   Linux or WSL (Windows Subsystem for Linux).
*   GCC Compiler.
*   Make.
*   Curl (for testing).

### One-Step Build & Run
The project includes an interactive runner that builds everything and offers a menu.

```bash
make run
```

### Manual Build
```bash
make clean
make
```

---

## 📖 Usage Guide & Demos

Run `./build/runner` (or `make run`) to see the menu. Here are the key demos:

### 1. **Basic Threads** (Option 1)
Simple proof-of-concept launching threads that yield to each other.

### 2. **Interactive Stride Scheduler** (Option 2) - *Dashboard Enabled*
*   **What it does**: Demonstrates proportional alignment of CPU time.
*   **Input**: Asks for number of threads (e.g., 5).
*   **Dashboard**: Open [http://localhost:9090](http://localhost:9090). Modify tickets to see frequency of execution change.
*   **Phase**: 13 & 14.

### 3. **Stack Overflow Test** (Option 3) - *Dashboard Enabled*
*   **What it does**: Recursive function consuming stack memory.
*   **Input**: Recursion depth (e.g., 500).
*   **Dashboard**: Watch the stack bars fill up and turn red in the "Stack Management" panel.
*   **Phase**: 14.

### 4. **Process Synchronization** (Option 4) - *Dashboard Enabled*
*   **What it does**: Threads contending for a single Mutex lock.
*   **Input**: Number of threads (e.g., 4).
*   **Dashboard**: Watch threads turn **BLOCKED (Red Border)** in the "Synchronization" panel while waiting for the lock.

### 5. **Sleep & Timers** (Option 5) - *Dashboard Enabled*
*   **What it does**: Threads sleeping for random durations.
*   **Dashboard**: See threads in "Sleep & Timers" list with wake timestamps.

### 6. **Async I/O Echo Server** (Option 6) - *Dashboard Enabled*
*   **What it does**: Starts an echo server on a high port.
*   **Dashboard**: Connect to the echo server (e.g., `nc localhost 8080`) and see the handling thread appear in "I/O Integration" blocked on `read()`.

### 10. **Standalone Advanced Dashboard** (Option 10)
Run just the dashboard server on port 9090. Useful if you want to run other tests manually.

---

## 🏗️ Development Phases Implementation History

This project was built in sequential phases:

*   **Phase 1: Core System**: Defined `gthread_t`, stack allocation, and main loop.
*   **Phase 2: Context Switching**: Added assembly (`trampoline.S`) for `switch` and `yield`.
*   **Phase 3: Scheduling**: Implemented Stride Scheduler (with Priority Queue).
*   **Phase 4: Stack Management**: Dynamic stack allocation and safety guards.
*   **Phase 5: Synchronization**: Built Mutexes (`gmutex`) and Condition Variables (`gcond`).
*   **Phase 6: Timers**: Added `gthread_sleep` and efficient checking.
*   **Phase 7: Async I/O**: Integrated `poll()` into the scheduler for non-blocking operations.
*   **Phase 8: HTTP Server**: Built a green-threaded web server.
*   **Phase 9: Compute Demos**: Parallel Matrix Mul, Merge Sort.
*   **Phase 10-12**: Improvements, Linting, Documentation.
*   **Phase 13-16: Advanced Dashboard & Interactivity**:
    *   Built `src/dashboard.c` (JSON API).
    *   Created modern Web UI (HTML/JS/CSS).
    *   Made demos interactive (`scanf`).
    *   Fixed critical Segfaults (Thread safety, Race conditions).

---

## ✅ Verification Steps

To manually verify the system behaves correctly:

1.  **Rebuild**:
    ```bash
    make clean && make
    ```
2.  **Test Mutexes**:
    *   Run `make run`, select **4**. Input **3** threads.
    *   Check `http://localhost:9090` -> "Synchronization".
    *   Verify threads cycle through BLOCKED state.
3.  **Test I/O**:
    *   Run `make run`, select **6**.
    *   In another terminal: `nc localhost 9000` (or whatever port `io_test` uses).
    *   Check `http://localhost:9090` -> "I/O Integration".
4.  **Test Scheduler**:
    *   Run `make run`, select **2**.
    *   Change tickets of a thread to 1000 vs 1.
    *   Observe "Pass" value increasing much faster for the high-ticket thread.

## 📂 Project Structure
*   `src/`: Core library code (`scheduler.c`, `gthread.c`, `io.c`, `dashboard.c`).
*   `include/`: Header files defining the API.
*   `examples/`: Demo programs (`stride_test`, `mutex_test`, etc.).
*   `examples/advanced_static/`: Frontend assets for the Dashboard.
