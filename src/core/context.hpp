#pragma once

#include <cstdint>
#include <cstddef>

// Platform-specific context structure (must match context.S layout)
struct Context {
    void* r15;
    void* r14;
    void* r13;
    void* r12;
    void* rbx;      // Callee-saved registers
    void* rbp;      // Base pointer
    void* rsp;      // Stack pointer
    void* rip;      // Instruction pointer
};

// Initialize a context to run the given function with the given stack
extern "C" void context_init(Context* ctx, void* stack, size_t stack_size, void (*entry)());

// Context operations
extern "C" void context_switch(Context* old_ctx, const Context* new_ctx);
extern "C" void context_save(Context* ctx);
