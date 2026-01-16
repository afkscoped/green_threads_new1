#ifndef DUAL_RUN_H
#define DUAL_RUN_H

void dual_run_workload(void (*worker)(void *), void *arg, int num_green,
                       int num_pthread);

#endif
