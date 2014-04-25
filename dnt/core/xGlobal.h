#pragma once

#ifndef __DNT_CORE_XGLOBAL_H__
#define __DNT_CORE_XGLOBAL_H__

#include <limits.h>
#include <float.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <dnt/core/xSystemDetection.h>

#ifdef DNTCORE_BUILD
#ifdef _MSC_VER
#define X_CORE_EXPORT __declspec(dllexport)
#else
#define X_CORE_EXPORT
#endif
#else
#ifdef _MSC_VER
#define X_CORE_EXPORT __declspec(dllimport)
#else
#define X_CORE_EXPORT
#endif
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#define X_GNUC_EXTENSION __extension__
#else
#define X_GNUC_EXTENSION
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _X_BOOLEAN_EXPR(expr) \
	X_GNUC_EXTENSION ({ \
	int _x_boolean_var_; \
	if (expr) \
	_x_boolean_var_ = 1; \
	else \
	_x_boolean_var_ = 0; \
	_x_boolean_var_; \
})

#define X_LIKELY(expr) (__builtin_expect (_G_BOOLEAN_EXPR(expr), 1))
#define X_UNLIKELY(expr) (__builtin_expect (_G_BOOLEAN_EXPR(expr), 0))

#else

#define X_LIKELY(expr) (expr)
#define X_UNLIKELY(expr) (expr)

#endif

#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#undef ABS
#define ABS(a) (((a) < 0) ? -(a) : (a))

#undef CLAMP
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#if !(defined (X_STMT_START) && defined (X_STMT_END))
#define X_STMT_START do
#define X_STMT_END while(0)
#endif

#ifdef __cplusplus
#define X_CORE_BEGIN_DECLS extern "C" {
#define X_CORE_END_DECLS }
#else
#define X_CORE_BEGIN_DECLS
#define X_CORE_END_DECLS
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

typedef int8_t xInt8;
typedef uint8_t xUInt8;
typedef int16_t xInt16;
typedef uint16_t xUInt16;
typedef int32_t xInt32;
typedef uint32_t xUInt32;
typedef int64_t xInt64;
typedef uint64_t xUInt64;
typedef char xChar;
typedef unsigned char xUChar;
typedef short xShort;
typedef unsigned short xUShort;
typedef long xLong;
typedef unsigned long xULong;
typedef int xInt;
typedef unsigned int xUInt;
typedef float xFloat;
typedef double xDouble;
typedef xInt xBoolean;
typedef void* xPointer;
typedef const void* xConstPointer;

#if defined(X_OS_WIN64)
typedef signed long long xSSize;
typedef unsigned long long xSSize;
#else
typedef signed int xSSize;
typedef unsigned int xSize;
#endif

typedef void(*xDestroyNotify)(xPointer data);
typedef xInt(*XCompareFunc)(xConstPointer a, xConstPointer b);
typedef void(xFunc)(xPointer data, xPointer user_data);


#endif // __DNT_CORE_XGLOBAL_H__
