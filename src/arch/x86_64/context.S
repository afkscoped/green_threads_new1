.text

// void context_switch(Context* old_ctx, const Context* new_ctx)
// %rdi: old_ctx
// %rsi: new_ctx
.global context_switch
.type context_switch, @function
context_switch:
    // Save the current context
    movq %r15, 0x00(%rdi)
    movq %r14, 0x08(%rdi)
    movq %r13, 0x10(%rdi)
    movq %r12, 0x18(%rdi)
    movq %rbx, 0x20(%rdi)
    movq %rbp, 0x28(%rdi)
    movq %rsp, 0x30(%rdi)
    
    // Save return address (RIP)
    movq (%rsp), %rax
    movq %rax, 0x38(%rdi)
    
    // Load the new context
    movq 0x00(%rsi), %r15
    movq 0x08(%rsi), %r14
    movq 0x10(%rsi), %r13
    movq 0x18(%rsi), %r12
    movq 0x20(%rsi), %rbx
    movq 0x28(%rsi), %rbp
    movq 0x30(%rsi), %rsp
    
    // Jump to the saved RIP
    movq 0x38(%rsi), %rax
    jmp *%rax

// void context_init(Context* ctx, void* stack, size_t stack_size, void (*entry)())
// %rdi: ctx
// %rsi: stack
// %rdx: stack_size
// %rcx: entry
.global context_init
.type context_init, @function
context_init:
    // Set up the stack pointer (16-byte aligned)
    leaq -16(%rsi, %rdx, 1), %r8  // stack + stack_size - 16
    andq $-16, %r8                // Align to 16 bytes
    
    // Set up the return address (entry point) on the stack
    movq %rcx, 8(%r8)
    
    // Set up the context
    movq %r8, 0x30(%rdi)  // RSP
    leaq thread_trampoline(%rip), %rax
    movq %rax, 0x38(%rdi)  // RIP
    
    ret

// void context_save(Context* ctx)
// %rdi: ctx
.global context_save
.type context_save, @function
context_save:
    // Save all callee-saved registers
    movq %r15, 0x00(%rdi)
    movq %r14, 0x08(%rdi)
    movq %r13, 0x10(%rdi)
    movq %r12, 0x18(%rdi)
    movq %rbx, 0x20(%rdi)
    movq %rbp, 0x28(%rdi)
    movq %rsp, 0x30(%rdi)
    
    // Save the return address (RIP)
    movq (%rsp), %rax
    movq %rax, 0x38(%rdi)
    
    // Return 0 to indicate success
    xorq %rax, %rax
    ret

// Trampoline function that calls the thread entry point
thread_trampoline:
    // Load the entry function pointer from the stack and call it
    movq 8(%rsp), %rax
    call *%rax
    // If the thread returns, we should never get here
    ud2
