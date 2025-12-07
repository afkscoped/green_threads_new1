# Userspace Green Thread Runtime

A lightweight, cooperative userspace threading library implemented in C++17 for Linux x86_64.

## Building

```bash
mkdir -p build && cd build
cmake ..
make
```

## Running the Example

```bash
./bin/example_basic
```

## Phase 1: Minimal Green Thread

This phase implements a basic green thread that can be created and switched to from the main thread. The implementation includes:

- Basic thread creation with stack allocation
- Context switching using assembly
- Simple scheduler for switching between main thread and user thread

### Implementation Details

- **Thread**: Manages stack and execution context
- **Context**: Handles CPU state and switching
- **Scheduler**: Manages thread scheduling (currently just two threads)

## Next Steps

- Phase 2: Implement cooperative yield()
- Phase 3: Support multiple green threads
- Phase 4: Thread cleanup and state management
