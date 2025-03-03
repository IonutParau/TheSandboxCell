#include "workers.h"
#ifdef TSC_USE_OPENMP
#include "omp.h"

void workers_setup(int count) {
    // Do nothing: OpenMP does it automagically
}

void workers_setupBest() {
    // OpenMP will do this regardless
}

void workers_addTask(worker_task_t *task, void *data) {
    // Useless, who cares.
}

void workers_waitForTasks(worker_task_t *task, void **dataArr, size_t len) {
    #pragma omp parallel for
    for(size_t i = 0; i < len; i++) {
        task(dataArr[i]);
    }
}

void workers_waitForTasksFlat(worker_task_t *task, void *dataArr, size_t dataSize, size_t len) {
    #pragma omp parallel for
    for(size_t i = 0; i < len; i++) {
        void *data = dataArr + i * dataSize;
        task(data);
    }
}

int workers_amount() {
    // Not by me. See https://stackoverflow.com/questions/4706494/openmp-get-total-number-of-running-threads for details
    int num_threads = 0;
    #pragma omp parallel reduction(+:num_threads)
    num_threads += 1;
    return num_threads;
}
#else
#include "tinycthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef linux
#include <unistd.h>
#endif

typedef struct worker_waitgroup_t {
    volatile atomic_ulong count;
} worker_waitgroup_t;

static worker_waitgroup_t workers_createWaitGroup() {
    worker_waitgroup_t worker;
    atomic_init(&worker.count, 0);
    return worker;
}

static void workers_addToWaitGroup(volatile worker_waitgroup_t *wg, size_t amount) {
    atomic_fetch_add_explicit(&wg->count, amount, memory_order_relaxed);
}

static void workers_removeFromWaitGroup(volatile worker_waitgroup_t *wg) {
    atomic_fetch_sub_explicit(&wg->count, 1, memory_order_relaxed);
}

static void workers_waitForGroup(volatile worker_waitgroup_t *wg) {
    while(true) {
        size_t amount = atomic_load_explicit(&wg->count, memory_order_relaxed);
        if(amount == 0) break;
        thrd_yield();
    }
}

typedef struct worker_task_info_t {
    worker_task_t *task;
    void *data;
    volatile worker_waitgroup_t *wg;
} worker_task_info_t;

typedef struct worker_channel_t {
    mtx_t lock;
    cnd_t taskCompleted;
    cnd_t taskGiven;
    volatile size_t len;
    volatile size_t cap;
    worker_task_info_t *volatile info;
} worker_channel_t;

static worker_channel_t workers_channel;
static size_t workers_count;
static thrd_t *workers_threads;

static void workers_channel_addUnsafe(worker_task_t *task, void *data, volatile worker_waitgroup_t *wg) {
    worker_task_info_t info = {task, data, wg};
    size_t idx = workers_channel.len;
    workers_channel.len++;
    if(workers_channel.len == workers_channel.cap) {
        workers_channel.cap *= 2;
        workers_channel.info = realloc(workers_channel.info, sizeof(worker_task_info_t) * workers_channel.cap);
    }
    workers_channel.info[idx] = info;
}

static void workers_channel_add(worker_task_t *task, void *data, volatile worker_waitgroup_t *wg) {
    mtx_lock(&workers_channel.lock);
    workers_channel_addUnsafe(task, data, wg);
    mtx_unlock(&workers_channel.lock);
}

static worker_task_info_t workers_getTask() {
    worker_task_info_t notask = {NULL, NULL, NULL};
    if(workers_channel.len == 0) return notask;
    return workers_channel.info[--workers_channel.len];
}

static int workers_worker(void *pid) {
    size_t id = (size_t)pid;
    mtx_lock(&workers_channel.lock);
    while(1) {
        if(id >= workers_count) {
            mtx_unlock(&workers_channel.lock);
            thrd_exit(0);
        }
        worker_task_info_t task = workers_getTask();
        if(task.task == NULL) {
            cnd_wait(&workers_channel.taskGiven, &workers_channel.lock);
        } else {
            mtx_unlock(&workers_channel.lock);
            task.task(task.data);
            if(task.wg != NULL) workers_removeFromWaitGroup(task.wg);
            // Say task completed
            cnd_broadcast(&workers_channel.taskCompleted);
            mtx_lock(&workers_channel.lock);
        }
    }
    return 0;
}

void workers_setAmount(int newCount) {
    size_t amount = workers_idealAmount();
    while(newCount < 0) {
        newCount += amount;
    }
    // 0 threads is valid btw!
    if(newCount > amount) {
        newCount = amount;
    }
    size_t old = workers_count;
    workers_count = newCount;
    if(workers_count == 0) {
        // Nothing left
        free(workers_threads);
        workers_threads = NULL;
        cnd_broadcast(&workers_channel.taskGiven);
    } else if(workers_count < old) {
        // Shrink
        workers_threads = realloc(workers_threads, sizeof(thrd_t) * workers_count);
        cnd_broadcast(&workers_channel.taskGiven);
    } else if(workers_count > old) {
        // Grow
        workers_threads = realloc(workers_threads, sizeof(thrd_t) * workers_count);
        for(size_t i = old; i < workers_count; i++) {
            thrd_create(workers_threads + i, workers_worker, (void *)(size_t)i);
        }
    }
}

bool workers_isDisabled() {
    return workers_count == 0;
}

void workers_setup(int count) {
    mtx_init(&workers_channel.lock, mtx_recursive);
    cnd_init(&workers_channel.taskCompleted);
    cnd_init(&workers_channel.taskGiven);
    workers_channel.len = 0;
    workers_channel.cap = 20;
    workers_channel.info = malloc(sizeof(worker_task_info_t) * workers_channel.cap);
    workers_count = count;
    workers_threads = malloc(sizeof(thrd_t) * count);
    for(int i = 0; i < count; i++) {
        thrd_create(workers_threads + i, &workers_worker, (void *)(size_t)i);
    }
}

size_t workers_idealAmount() {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#endif
#ifdef linux
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

void workers_setupBest() {
    workers_setup(workers_idealAmount());
}

void workers_addTask(worker_task_t *task, void *data) {
    workers_channel_add(task, data, NULL);
    cnd_signal(&workers_channel.taskGiven);
}

void workers_waitForTasks(worker_task_t *task, void **dataArr, size_t len) {
    if(workers_isDisabled()) {
        for(int i = 0; i < len; i++) task(dataArr[i]);
        return;
    }
    worker_waitgroup_t wg = workers_createWaitGroup();
    workers_addToWaitGroup(&wg, len);
    mtx_lock(&workers_channel.lock);
    for(int i = 0; i < len; i++) {
        workers_channel_addUnsafe(task, dataArr[i], &wg);
    }
    mtx_unlock(&workers_channel.lock);
    cnd_broadcast(&workers_channel.taskGiven);
    workers_waitForGroup(&wg);
}

void workers_waitForTasksFlat(worker_task_t *task, void *dataArr, size_t dataSize, size_t len) {
    if(workers_isDisabled()) {
        for(int i = 0; i < len; i++) {
            task(dataArr);
            dataArr += dataSize;
        }
        return;
    }
    worker_waitgroup_t wg = workers_createWaitGroup();
    workers_addToWaitGroup(&wg, len);
    mtx_lock(&workers_channel.lock);
    for(int i = 0; i < len; i++) {
        workers_channel_addUnsafe(task, dataArr, &wg);
        dataArr += dataSize;
    }
    mtx_unlock(&workers_channel.lock);
    cnd_broadcast(&workers_channel.taskGiven);
    workers_waitForGroup(&wg);
}

int workers_amount() {
    return workers_count;
}
#endif
