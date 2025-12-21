#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Parameters
#define SIZE 200
#define NUM_THREADS 4

// Matrices
int A[SIZE][SIZE];
int B[SIZE][SIZE];
int C[SIZE][SIZE];        // Result
int C_serial[SIZE][SIZE]; // For verification

typedef struct {
  int start_row;
  int end_row;
  int thread_id;
} thread_arg_t;

void matrix_worker(void *arg) {
  thread_arg_t *data = (thread_arg_t *)arg;
  int start = data->start_row;
  int end = data->end_row;

  printf("Thread %d starting rows %d to %d\n", data->thread_id, start, end);

  for (int i = start; i < end; i++) {
    for (int j = 0; j < SIZE; j++) {
      int sum = 0;
      for (int k = 0; k < SIZE; k++) {
        sum += A[i][k] * B[k][j];
      }
      C[i][j] = sum;

      // Cooperatively yield every row (or fewer) to let others run
      // This is crucial for green threads on a single core to prevent
      // starvation if strict priority wasn't fair, but with stride it helps
      // fairness latency.
      if (j % 50 == 0)
        gthread_yield();
    }
    // Yield after every row completed
    gthread_yield();
  }

  printf("Thread %d finished\n", data->thread_id);
  free(data);
}

void init_matrices(void) {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      A[i][j] = rand() % 10;
      B[i][j] = rand() % 10;
    }
  }
}

void serial_multiply(void) {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      int sum = 0;
      for (int k = 0; k < SIZE; k++) {
        sum += A[i][k] * B[k][j];
      }
      C_serial[i][j] = sum;
    }
  }
}

int verify(void) {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      if (C[i][j] != C_serial[i][j]) {
        printf("Mismatch at [%d][%d]: %d != %d\n", i, j, C[i][j],
               C_serial[i][j]);
        return 0;
      }
    }
  }
  return 1;
}

int main(void) {
  gthread_init();
  srand(time(NULL));

  printf("Initializing %dx%d matrices...\n", SIZE, SIZE);
  init_matrices();

  printf("Spawning %d threads...\n", NUM_THREADS);
  gthread_t *threads[NUM_THREADS];
  int rows_per_thread = SIZE / NUM_THREADS;

  for (int i = 0; i < NUM_THREADS; i++) {
    thread_arg_t *arg = malloc(sizeof(thread_arg_t));
    arg->start_row = i * rows_per_thread;
    arg->end_row = (i == NUM_THREADS - 1) ? SIZE : (i + 1) * rows_per_thread;
    arg->thread_id = i;
    gthread_create(&threads[i], matrix_worker, arg);
  }

  // Join
  for (int i = 0; i < NUM_THREADS; i++) {
    gthread_join(threads[i], NULL);
  }

  printf("Parallel computation done. Verifying...\n");
  serial_multiply();

  if (verify()) {
    printf("Verification SUCCESS!\n");
  } else {
    printf("Verification FAILED!\n");
  }

  printf("\nResult Preview (10x10):\n");
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      printf("%4d ", C[i][j]);
    }
    printf("\n");
  }

  return 0;
}
