# Advanced GreenThreads Dashboard

This dashboard provides runtime control and deep introspection of the GreenThreads system.

## Running the Dashboard

The advanced dashboard runs as a separate binary on port **9090**.

```bash
./build/advanced_dashboard
```

You can run it simultaneously with the standard dashboard (port 8080):

```bash
./build/web_dashboard &
./build/advanced_dashboard &
```

## Features & Metrics

### 1. Stride Scheduler Control
- **Tickets**: Represents the priority of a thread. Higher tickets = larger CPU share.
- **Pass**: The virtual time accumulated by the thread. The scheduler picks the thread with the lowest Pass value.
- **Stride**: inversely proportional to Tickets (`Constant / Tickets`).
- **Control**: Enter a new ticket value (1-1000) and click "Set" to dynamically adjust priority.

### 2. Stack Usage Monitor
- Visualizes the stack memory usage for each green thread.
- Green: Low usage.
- Orange: >70% usage.
- Red: >90% usage (Near overflow danger).
- Note: Stack usage is approximate based on the current Stack Pointer (RSP).

### 3. I/O & Global Metrics
- **Waiting I/O**: Lists threads currently blocked on `poll()` waiting for file descriptors.
- **Global Stats**: Counts of Runnable, Sleeping, and waiting threads.

## Architecture
This dashboard communicates with the runtime via `include/runtime_stats.h`, utilizing a global thread list and direct state introspection.
