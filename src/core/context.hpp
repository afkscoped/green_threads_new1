#pragma once

#include <cstdint>
#include <cstddef>

// Platform-specific context structure
struct Context {
    void* rip;      // Instruction pointer
    void* rsp;      // Stack pointer
    void* rbp;      // Base pointer
    void* rbx;      // Callee-saved registers
    void* r12;
    void* r13;
    void* r14;
    void* r15;
};

// Initialize a context to run the given function with the given stack
void context_init(Context* ctx, void* stack, size_t stack_size, void (*entry)());

// Context switching
extern "C" void context_switch(Context* old_ctx, const Context* new_ctx);
