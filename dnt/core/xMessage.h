#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XMESSAGE_H__
#define __DNT_CORE_XMESSAGE_H__

#include <dnt/core/xGlobal.h>

#ifdef X_DISABLE_CHECKS

#define x_return_if_fail(expr) X_STMT_START{ (void)0; } X_STMT_END
#define x_return_val_if_fail(expr, val) X_STMT_START { (void)0; } X_STMT_END
#define x_return_if_reached() X_STMT_START { return; } X_STMT_END
#define x_return_val_if_reached(val) X_STMT_START { return (val); } X_STMT_END

#else // X_DISABLE_CHECKS

#define x_return_if_fail(expr) X_STMT_START { \
	if X_LIKELY(expr) { } else \
	{ \
	return; \
} \
} X_STMT_END

#define x_return_val_if_fail(expr, val) X_STMT_START{ \
	if X_LIKELY(expr) { } else \
	{ \
	return (val); \
} \
}X_STMT_END

#define x_return_if_reached() X_STMT_START { \
	return; \
} X_STMT_END

#define x_return_val_if_reached() X_STMT_START { \
	return (val); \
} X_STMT_END

#endif

#endif // __DNT_CORE_XMESSAGE_H__
