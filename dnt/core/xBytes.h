#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XBYTES_H__
#define __DNT_CORE_XBYTES_H__

#include <dnt/core/xGlobal.h>
#include <dnt/core/xArray.h>

X_CORE_BEGIN_DECLS

xBytes* x_bytes_new(xConstPointer data, xSize size);
xBytes* x_bytes_new_take(xPointer data, xSize size);
xBytes* x_bytes_new_static(xConstPointer data, xSize size);
xBytes* x_bytes_new_with_free_func(xConstPointer data, xSize size, xDestroyNotify free_func, xPointer user_data);
xBytes* x_bytes_new_from_bytes(xBytes *bytes, xSize offset, xSize length);
xConstPointer x_bytes_get_data(xBytes *bytes, xSize *size);
xSize x_bytes_get_size(xBytes *bytes);
xBytes* x_bytes_ref(xBytes *bytes);
void x_bytes_unref(xBytes *bytes);
xPointer x_bytes_unref_to_data(xBytes *bytes, xSize *size);
xByteArray* x_bytes_unref_to_array(xBytes *bytes);
xUInt x_bytes_hash(xConstPointer bytes);
xBoolean x_bytes_equal(xConstPointer bytes1, xConstPointer bytes2);
xInt x_bytes_compare(xConstPointer bytes1, xConstPointer bytes2);

X_CORE_END_DECLS

#endif // __DNT_CORE_XBYTES_H__
