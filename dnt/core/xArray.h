#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_ARRAY_H__
#define __DNT_CORE_ARRAY_H__

#include <dnt/core/xGlobal.h>

X_CORE_BEGIN_DECLS

typedef struct _xBytes xBytes;
typedef struct _xArray xArray;
typedef struct _xPtrArray xPtrArray;
typedef struct _xByteArray xByteArray;

struct _xArray
{
	xChar *data;
	xUInt len;
};

struct _xPtrArray
{
	xPointer *pdata;
	xUInt len;
};

struct _xByteArray
{
	xUInt8 *data;
	xUInt len;
};

#define x_array_append_val(a, v) x_array_append_vals(a, &(v), 1)
#define x_array_prepend_val(a, v) x_array_prepend_vals(a, &(v), 1)
#define x_array_insert_val(a, i, v) x_array_insert_vals(a, i, &(v), 1)
#define x_array_index(a, t, i) (((t*)(void*)(a)->data)[(i)])

xArray* x_array_new(xBoolean zero_terminated, xBoolean clear, xUInt element_size);
xArray* x_array_sized_new(xBoolean zero_terminated, xBoolean clear, xUInt element_size, xUInt reserved_size);
xChar* x_array_free(xArray *array, xBoolean free_segment);
xArray* x_array_ref(xArray *array);
void x_array_unref(xArray *array);
xUInt x_array_get_element_size(xArray *array);
xArray* x_array_append_vals(xArray *array, xConstPointer data, xUInt len);
xArray* x_array_prepend_vals(xArray *array, xConstPointer data, xUInt len);
xArray* x_array_insert_vals(xArray *array, xUInt index, xConstPointer data, xUInt len);
xArray* x_array_set_size(xArray *array, xUInt length);
xArray* x_array_remove_index(xArray *array, xUInt index);
xArray* x_array_remove_index_fast(xArray *array, xUInt index);
xArray* x_array_remove_range(xArray *array, xUInt index, xUInt length);
void x_array_sort(xArray *array, xCompareFunc compare_func);
/*void x_array_sort_with_data(xArray *array, xCompareDataFunc compare_func, xPointer user_data); */
void x_array_set_clear_func(xArray *array, xDestroyNotify clear_func);

#define x_ptr_array_index(array, index) ((array)->pdata)[index]
xPtrArray* x_ptr_array_new(void);
xPtrArray* x_ptr_array_new_with_free_func(xDestroyNotify element_free_func);
xPtrArray* x_ptr_array_sized_new(xUInt reserved_size);
xPtrArray* x_ptr_array_new_full(xUInt reserved_size, xDestroyNotify element_free_func);
xPointer* x_ptr_array_free(xPtrArray *array, xBoolean free_seg);
xPtrArray* x_ptr_array_ref(xPtrArray *array);
void x_ptr_array_unref(xPtrArray *array);
void x_ptr_array_set_free_func(xPtrArray *array, xDestroyNotify element_free_func);
void x_ptr_array_set_size(xPtrArray *array, xSize length);
xPointer x_ptr_array_remove_index(xPtrArray *array, xUInt index);
xPointer x_ptr_array_remove_index_fast(xPtrArray *array, xUInt index);
xBoolean x_ptr_array_remove(xPtrArray *array, xPointer data);
xBoolean x_ptr_array_remove_fast(xPtrArray *array, xPointer data);
xPtrArray* x_ptr_array_remove_range(xPtrArray *array, xUInt index, xUInt length);
void x_ptr_array_add(xPtrArray *array, xPointer data);
void x_ptr_array_insert(xPtrArray *array, xInt index, xPointer data);
void x_ptr_array_sort(xPtrArray *array, xCompareFunc compare_func);
// void x_ptr_array_sort_with_data(xPtrArray *array, xCompareFunc compare_func, xPointer user_data);
void x_ptr_array_foreach(xPtrArray *array, xFunc func, xPointer user_data);

xByteArray* x_byte_array_new(void);
xByteArray* x_byte_array_new_take(xUInt8 *data, xSize len);
xByteArray* x_byte_array_sized_new(xUInt reserved_size);
xUInt8* x_byte_array_free(xByteArray *array, xBoolean free_segment);
xBytes* x_byte_array_free_to_bytes (xByteArray *array);
xByteArray* x_byte_array_ref(xByteArray *array);
void x_byte_array_unref(xByteArray *array);
xByteArray* x_byte_array_append(xByteArray *array, const xUInt8 *data, xUInt len);
xByteArray* x_byte_array_prepend(xByteArray *array, const xUInt8 *data, xUInt len);
xByteArray* x_byte_array_set_size(xByteArray *array, xUInt length);
xByteArray* x_byte_array_remove_index(xByteArray *array, xUInt index);
xByteArray* x_byte_array_remove_index_fast(xByteArray *array, xUInt index);
xByteArray* x_byte_array_remove_range(xByteArray *array, xUInt index, xUInt length);
void x_byte_array_sort(xByteArray *array, xCompareFunc compare_func);
// void x_byte_array_sort_with_data(xByteArray *array, xCompareDataFunc compare_func, xPointer user_data);

X_CORE_END_DECLS

#endif // __DNT_CORE_ARRAY_H__
