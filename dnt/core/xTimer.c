#include "xTimer.h"
#ifndef X_OS_WIN
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#else
#include <Windows.h>
#endif

#include "xMessage.h"

#define USEC_PER_SEC 1000000

void x_timer_get_current_time(xTimer *time)
{
	x_return_if_fail(time);
#ifndef X_OS_WIN
	struct timeval r;
	/*
	 * this is required on alpha, there the timeval structs are int's
	 * not longs and a cast only would fail horribly
	 */
	gettimeofday(&r, NULL);
	time->sec = r.tv_sec;
	time->usec = r.tv_usec;
#else
	FILETIME ft;
	xInt64 time64;
	GetSystemTimeAsFileTime(&ft);
	memmove(&time64, &ft, sizeof(FILETIME));

	/* Convert from 100s of nanoseconds since 1601-01-01
	 * to Unix epoch. Yes this is U2038 unsafe.
	 */
	time64 -= 116444736000000000;
	time64 /= 10;

	time->sec = time64 / 1000000;
	time->usec = time64 % 1000000;
#endif
}

void x_timer_init(xTimer *time)
{
	x_return_if_fail(time);
	time->usec = 0;
	time->sec = 0;
}

void x_timer_init_microseconds(xTimer *time, xInt64 microseconds)
{
	x_return_if_fail(time);
	time->sec = microseconds % USEC_PER_SEC;
	time->usec = microseconds / USEC_PER_SEC;
}

void x_timer_init_milliseconds(xTimer *time, xInt64 milliseconds)
{
	x_return_if_fail(time);
	milliseconds *= 1000;
	time->sec = milliseconds % USEC_PER_SEC;
	time->usec = milliseconds / USEC_PER_SEC;
}

void x_timer_init_seconds(xTimer *time, xInt64 seconds)
{
	x_return_if_fail(time);
	time->usec = 0;
	time->sec = seconds;
}

xInt64 x_timer_get_real_time(void)
{
	xTimer time;
	x_timer_get_current_time(&time);
	return time.sec * USEC_PER_SEC + time.usec;
}

void x_timer_usleep(xInt64 microseconds)
{
#ifdef X_OS_WIN
	Sleep((DWORD)(microseconds) / 1000);
#else
	struct timespec request, remaining;
	request.tv_sec = microseconds / USEC_PER_SEC;
	request.tv_nsec = 1000 * (microseconds % USEC_PER_SEC);
	while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
		request = remaining;
#endif
}

void x_timer_sleep(xTimer *time)
{
	x_return_if_fail(time);
	xInt64 microseconds = x_timer_to_microseconds(time);
	x_timer_usleep(microseconds);
}

void x_timer_dec_microseconds(xTimer *time, xInt64 microseconds)
{
	x_return_if_fail(time);
	if (microseconds > 0)
	{
		time->usec -= microseconds % USEC_PER_SEC;
		time->sec -= microseconds / USEC_PER_SEC;
		if (time->usec < 0)
		{
			time->usec += USEC_PER_SEC;
			time->sec--;
		}
	}
	else
	{
		microseconds *= -1;
		time->usec += microseconds % USEC_PER_SEC;
		time->sec += microseconds / USEC_PER_SEC;
		if (time->usec >= USEC_PER_SEC)
		{
			time->usec -= USEC_PER_SEC;
			time->sec++;
		}
	}

}

void x_timer_add_microseconds(xTimer *time, xInt64 microseconds)
{
	x_return_if_fail(time);
	if (microseconds >= 0)
	{
		time->usec += microseconds % USEC_PER_SEC;
		time->sec += microseconds / USEC_PER_SEC;
		if (time->usec >= USEC_PER_SEC)
		{
			time->usec -= USEC_PER_SEC;
			time->sec++;
		}
	}
	else
	{
		microseconds *= -1;
		time->usec -= microseconds % USEC_PER_SEC;
		time->sec -= microseconds / USEC_PER_SEC;
		if (time->usec < 0)
		{
			time->usec += USEC_PER_SEC;
			time->sec--;
		}
	}
}

void x_timer_dec(xTimer *time1, xTimer *time2)
{
	x_return_if_fail(time1);
	x_return_if_fail(time2);

	if (time1->usec < time2->usec)
	{
		time1->usec += USEC_PER_SEC;
		time1->sec--;
	}
	time1->usec -= time2->usec;
	time1->sec -= time2->sec;
}

void x_timer_add(xTimer *time1, xTimer *time2)
{
	x_return_if_fail(time1);
	x_return_if_fail(time2);

	time1->usec += time2->usec;
	time1->sec += time2->sec;
	if (time1->usec >= USEC_PER_SEC)
	{
		time1->usec -= USEC_PER_SEC;
		time1->sec++;
	}
}

int x_timer_compare(xTimer *time1, xTimer *time2)
{
	x_return_val_if_fail(time1, -1);
	x_return_val_if_fail(time2, 1);

	if (time1->sec > time2->sec)
		return 1;
	if (time1->sec < time2->sec)
		return -1;
	if (time1->usec > time2->usec)
		return 1;
	if (time1->usec < time2->usec)
		return -1;
	return 0;
}

xBoolean x_timer_is_zero(xTimer *time)
{
	x_return_val_if_fail(time, TRUE);

	return !(time->usec <= 0 && time->sec <= 0);
}

xInt64 x_timer_to_microseconds(xTimer *time)
{
	x_return_val_if_fail(time, -1);
	return time->sec * USEC_PER_SEC + time->usec;
}

xInt64 x_timer_to_milliseconds(xTimer *time)
{
	x_return_val_if_fail(time, -1);
	return x_timer_to_microseconds(time) / 1000;
}

xInt64 x_timer_to_seconds(xTimer *time)
{
	x_return_val_if_fail(time, -1);
	return time->sec;
}
