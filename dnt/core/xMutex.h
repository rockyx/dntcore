#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XMUTEX_H__
#define __DNT_CORE_XMUTEX_H__

#include <dnt/core/xGlobal.h>

X_CORE_BEGIN_DECLS

typedef struct _xMutex xMutex;

xMutex * x_mutex_new(void);
void x_mutex_free(xMutex *mutex);
void x_mutex_lock(xMutex *mutex);
void x_mutex_unlock(xMutex *mutex);

X_CORE_END_DECLS

#endif // __DNT_CORE_MUTEX_H__
