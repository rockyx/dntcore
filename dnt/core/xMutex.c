#include "xMutex.h"
#include "xMessage.h"

#ifdef X_OS_WIN
#include <Windows.h>

struct _xMutex
{
	HANDLE handle;
	DWORD ret;
};

xMutex* x_mutex_new(void)
{
	xMutex *ret = malloc(sizeof(xMutex));
	ret->handle = CreateMutex(NULL, FALSE, NULL);
	return ret;
}

void x_mutex_free(xMutex *mutex)
{
	x_return_if_fail(mutex);
	x_mutex_unlock(mutex);
	CloseHandle(mutex->handle);
	free(mutex);
}

void x_mutex_lock(xMutex *mutex)
{
	x_return_if_fail(mutex);

	if (mutex->handle != INVALID_HANDLE_VALUE)
		mutex->ret = WaitForSingleObject(mutex->handle, INFINITE);
}

void x_mutex_unlock(xMutex *mutex)
{
	x_return_if_fail(mutex);

	/* Release owenship of the mutex object */
	if (!ReleaseMutex(mutex->handle))
	{
		/* Handle error */
	}
}

#else
#include <pthread.h>

struct _xMutex
{
	pthread_mutex_t handle;
};

xMutex* x_mutex_new(void)
{
	xMutex *ret = malloc(sizeof(xMutex));
	pthread_mutex_init(&(ret->handle), NULL);
	return ret;
}

void x_mutex_free(xMutex *mutex)
{
	x_return_if_fail(mutex);
	
	x_mutex_unlock(mutex);

	pthread_mutex_destroy(&(mutex->handle));

	free(mutex);
}

void x_mutex_lock(xMutex *mutex)
{
	x_return_if_fail(mutex);

	pthread_mutex_lock(&(mutex->handle));
}

void x_mutex_unlock(xMutex *mutex)
{
	x_return_if_fail(mutex);

	pthread_mutex_unlock(&(mutex->handle));
}

#endif
