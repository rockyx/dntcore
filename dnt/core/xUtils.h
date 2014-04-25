#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XUTILS_H__
#define __DNT_CORE_XUTILS_H__

#include <dnt/core/xGlobal.h>

#ifndef MAKEWORD
#define MAKEWORD(a, b)      ((xUInt16)(((xUInt8)(((xUInt64)(a)) & 0xff)) | ((xUInt16)((xUInt8)(((xUInt64)(b)) & 0xff))) << 8))
#endif

#ifndef MAEKLONG
#define MAKELONG(a, b)      ((LONG)(((xUInt16)(((xUInt64)(a)) & 0xffff)) | ((DWORD)((xUInt16)(((xUInt64)(b)) & 0xffff))) << 16))
#endif

#ifndef LOWORD
#define LOWORD(l)           ((xUInt16)(((xUInt64)(l)) & 0xffff))
#endif

#ifndef HIWORD
#define HIWORD(l)           ((xUInt16)((((xUInt64)(l)) >> 16) & 0xffff))
#endif

#ifndef LOBYTE
#define LOBYTE(w)           ((xUInt8)(((xUInt64)(w)) & 0xff))
#endif

#ifndef HIBYTE
#define HIBYTE(w)           ((xUInt8)((((xUInt64)(w)) >> 8) & 0xff))
#endif

#endif // __DNT_CORE_UTILS_H__
