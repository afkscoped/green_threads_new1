.global gthread_switch
.global gthread_trampoline

/*
 * void gthread_switch(gthread_ctx_t *new_ctx, gthread_ctx_t *old_ctx);
 * RDI = new_ctx
 * RSI = old_ctx
 *
 * gthread_ctx_t layout:
 * 0:  rbx
 * 8:  rbp
 * 16: r12
 * 24: r13
 * 32: r14
 * 40: r15
 * 48: rip
 * 56: rsp
 */

gthread_switch:
    /* Save current context (old_ctx) if RSI is not NULL */
    test %rsi, %rsi
    jz load_new

    mov %rbx, 0(%rsi)
    mov %rbp, 8(%rsi)
    mov %r12, 16(%rsi)
    mov %r13, 24(%rsi)
    mov %r14, 32(%rsi)
    mov %r15, 40(%rsi)
    
    /* Save RIP and RSP */
    /* The return address is on the stack right now (pushed by call) */
    mov (%rsp), %rax
    mov %rax, 48(%rsi)
    
    /* Save RSP (after return address popped, so simulate it) */
    /* Actually better: save current RSP + 8 */
    lea 8(%rsp), %rax
    mov %rax, 56(%rsi)

load_new:
    /* Load new context (new_ctx) from RDI */
    mov 0(%rdi), %rbx
    mov 8(%rdi), %rbp
    mov 16(%rdi), %r12
    mov 24(%rdi), %r13
    mov 32(%rdi), %r14
    mov 40(%rdi), %r15
    
    /* Restore RSP and RIP */
    mov 56(%rdi), %rsp
    mov 48(%rdi), %rax
    
    /* Jump to saved RIP */
    jmp *%rax

/*
 * Helper start routine for new threads.
 * We expect:
 * R12 = Function pointer
 * R13 = Argument
 */
gthread_trampoline:
    /* Function prologue not strictly needed since we are base of stack */
    mov %r13, %rdi  /* Move arg to First Argument Register (System V ABI) */
    call *%r12      /* Call thread function */
    
    /* If function returns, call exit */
    call gthread_exit
    
    /* Should never return here */
    hlt
