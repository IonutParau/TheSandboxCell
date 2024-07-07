#ifndef TSC_THREAD_WORKERS
#define TSC_THREAD_WORKERS

#include <stddef.h>

typedef void (worker_task_t)(void *data);

// hideapi
void workers_setup(int count);
void workers_setupBest();
// hideapi
void workers_addTask(worker_task_t *task, void *data);
void workers_waitForTasks(worker_task_t *task, void **dataArr, size_t len);
void workers_waitForTasksFlat(worker_task_t *task, void *dataArr, size_t dataSize, size_t len);
int workers_amount();

#endif
