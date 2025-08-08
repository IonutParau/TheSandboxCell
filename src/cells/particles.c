#include "grid.h"
#include "ticking.h"
#include "../threads/threads.h"
#include <stdlib.h>

typedef struct tsc_particleQueue {
	bool containsStuff;
	tsc_particleConfig config;
	struct tsc_particleQueue *next;
} tsc_particleQueue;

mtx_t tsc_particlesLock;
volatile tsc_particleQueue tsc_particleQueueRoot;

void tsc_requestParticle(tsc_particleConfig config) {
#ifndef TSC_TURBO
    if(!storeExtraGraphicInfo) return;

	mtx_lock(&tsc_particlesLock);

	if(tsc_particleQueueRoot.containsStuff) {
		tsc_particleQueue *next = malloc(sizeof(tsc_particleQueue));
		*next = tsc_particleQueueRoot;
		tsc_particleQueueRoot = (tsc_particleQueue) {
			.containsStuff = true,
			.config = config,
			.next = next,
		};
	} else {
		tsc_particleQueueRoot = (tsc_particleQueue) {
			.containsStuff = true,
			.config = config,
			.next = NULL,
		};
	}

	mtx_unlock(&tsc_particlesLock);
#endif
}

bool tsc_getRequestedParticle(tsc_particleConfig *config) {
	if(!tsc_particleQueueRoot.containsStuff) return false;
	mtx_lock(&tsc_particlesLock);

	if(!tsc_particleQueueRoot.containsStuff) {
		mtx_unlock(&tsc_particlesLock);
		return false;
	}

	*config = tsc_particleQueueRoot.config;

	if(tsc_particleQueueRoot.next == NULL) {
		// last one
		tsc_particleQueueRoot.containsStuff = false;
	} else {
		// shit!!!!!
		tsc_particleQueue *next = tsc_particleQueueRoot.next;
		tsc_particleQueueRoot = *next;
		free(next);
	}

	mtx_unlock(&tsc_particlesLock);
	return true;
}

void tsc_initParticleQueue() {
	mtx_init(&tsc_particlesLock, mtx_plain);
	tsc_particleQueueRoot.containsStuff = false;
	tsc_particleQueueRoot.next = NULL;
}
