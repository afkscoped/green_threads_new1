#pragma once

// Simple C-style API for the green thread runtime
// Implemented in runtime.cpp

// Initialize the green thread runtime and scheduler
void runtime_init();

// Spawn a new green thread that runs the given entry function
void spawn(void (*entry)());

// Yield the current green thread
void yield();

// Start the scheduler loop; returns when all threads are done
void runtime_run();
