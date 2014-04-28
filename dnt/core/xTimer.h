#pragma once

#ifndef __DNT_CORE_XTIMER_H__
#define __DNT_CORE_XTIMER_H__

#include <dnt/core/xGlobal.h>

X_CORE_BEGIN_DECLS

typedef struct _xTimer xTimer;

struct _xTimer
{
	xInt64 sec; // seconds
	xInt64 usec; // microseconds
};

#define X_TIMER_INITIALIZER {0, 0}

void x_timer_get_current_time(xTimer *time);
void x_timer_init(xTimer *time);
void x_timer_init_microseconds(xTimer *time, xInt64 microseconds);
void x_timer_init_milliseconds(xTimer *time, xInt64 milliseconds);
void x_timer_init_seconds(xTimer *time, xInt64 seconds);
xInt64 x_timer_get_real_time();
void x_timer_usleep(xInt64 microseconds);
void x_timer_sleep(xTimer *time);
void x_timer_add(xTimer *time1, xTimer *time2);
void x_timer_add_microseconds(xTimer *time, xInt64 microseconds);
void x_timer_add_milliseconds(xTimer *time, xInt64 millliseconds);
int x_timer_compare(xTimer *time1, xTimer *time2);
xBoolean x_timer_is_zero(xTimer *time);
xInt64 x_timer_to_microseconds(xTimer *time);
xInt64 x_timer_to_milliseconds(xTimer *time);
xInt64 x_timer_to_seconds(xTimer *time);

X_CORE_END_DECLS

#endif // __DNT_CORE_TIMER_H__
