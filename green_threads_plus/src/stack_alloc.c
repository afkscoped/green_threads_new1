#define _GNU_SOURCE
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>


void *stack_alloc(size_t size) {
#ifdef __linux__
  // Align to page size
  size_t page_size = sysconf(_SC_PAGESIZE);
  if (size == 0)
    size = 64 * 1024;
  size = (size + page_size - 1) & ~(page_size - 1);

  // Add guard page size
  size_t total_size = size + page_size;

  void *stack = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack == MAP_FAILED) {
    perror("mmap stack");
    return NULL;
  }

  // Protect the first page (guard page)
  if (mprotect(stack, page_size, PROT_NONE) == -1) {
    perror("mprotect guard");
    munmap(stack, total_size);
    return NULL;
  }

  // Return pointer to the usable stack area
  return (char *)stack + page_size;
#else
  return malloc(size);
#endif
}

void stack_free(void *stack, size_t size) {
#ifdef __linux__
  size_t page_size = sysconf(_SC_PAGESIZE);
  void *real_stack = (char *)stack - page_size;
  munmap(real_stack, size + page_size);
#else
  free(stack);
#endif
}
