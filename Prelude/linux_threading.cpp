#include "linux_aux_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) || defined(__APPLE__)

#include <errno.h>

typedef struct linux_wrapper_threadwrap {
	pthread_t thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int suspend_count;
}linux_wrapper_threadwrap;

HANDLE CreateMutex(void * lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) {
	pthread_mutex_t * mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, 0);

	if (bInitialOwner) {
		pthread_mutex_lock(mutex);
	}
	return (HANDLE)mutex;
}

BOOL ReleaseMutex(HANDLE hMutex) {
	pthread_mutex_unlock((pthread_mutex_t*)hMutex);
	return TRUE;
}


DWORD SuspendThread(HANDLE t) {
	linux_wrapper_threadwrap * tw = (linux_wrapper_threadwrap*)t;
	pthread_mutex_lock(&tw->mutex);
	int count = tw->suspend_count;
	tw->suspend_count++;
	if (tw->suspend_count <= 0) {
		pthread_mutex_unlock(&tw->mutex);
		return (DWORD)count;
	}
	pthread_cond_wait(&tw->cond, &tw->mutex);
	pthread_mutex_unlock(&tw->mutex);
	return (DWORD)count;
}

DWORD ResumeThread(HANDLE t) {
	linux_wrapper_threadwrap * tw = (linux_wrapper_threadwrap*)t;
	pthread_mutex_lock(&tw->mutex);
	int count = tw->suspend_count;
	tw->suspend_count--;
	if (tw->suspend_count <= 0) {
		pthread_cond_signal(&tw->cond);
	}
	pthread_mutex_unlock(&tw->mutex);
	return (DWORD)count;

}

HANDLE CreateThread(void * lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
	linux_wrapper_threadwrap * t = new linux_wrapper_threadwrap;
	t->suspend_count = 0;

	pthread_mutex_init(&t->mutex, 0);
	pthread_cond_init(&t->cond, 0);
	
	pthread_create(&t->thread, 0, (void*(*)(void*))lpStartAddress, lpParameter);

	if (lpThreadId) {
		// pthread_t is a pointer on macOS; the game only treats the id as an
		// opaque number, so truncation is fine
		*lpThreadId = (DWORD)(uintptr_t)t->thread;
	}

	return t;
}

BOOL TerminateThread(HANDLE hThread, DWORD  dwExitCode) {
	linux_wrapper_threadwrap * t = (linux_wrapper_threadwrap*)hThread;
	pthread_cancel(t->thread);
	
	delete t;
	return 1;
}

typedef struct linux_event {
	int state;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} linux_event;

HANDLE CreateEvent(void * lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName) {
	if (bManualReset) {
		printf("unimplemented CreateEvent\n");
		exit(0);
	}
	linux_event * t = new linux_event;
	t->state = bInitialState;
	pthread_mutex_init(&t->mutex, 0);
	pthread_cond_init(&t->cond, 0);

	return t;
}


DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
	linux_event * t = (linux_event*)hHandle;
	
	pthread_mutex_lock(&t->mutex);
	struct timespec ts;
	ts.tv_nsec = ts.tv_sec = 0;
	ts.tv_sec = dwMilliseconds % 1000;
	ts.tv_nsec = dwMilliseconds * 1000000;
	
	if (t->state) {
		t->state = 0;
		pthread_mutex_unlock(&t->mutex);
		return 0;
	}

	int rt = pthread_cond_timedwait(&t->cond, &t->mutex, &ts);
	
	if (t->state) {
		t->state = 0;
		pthread_mutex_unlock(&t->mutex);
		return 0;
	}

	pthread_mutex_unlock(&t->mutex);
	if (rt == ETIMEDOUT) {
		return WAIT_TIMEOUT;
	}
	return WAIT_OBJECT_0;

}

BOOL SetEvent(HANDLE hEvent) {
	
	linux_event * t = (linux_event*)hEvent;

	pthread_mutex_lock(&t->mutex);
	t->state = 1;
	pthread_cond_signal(&t->cond);

	pthread_mutex_unlock(&t->mutex);
	return TRUE;
}

#endif