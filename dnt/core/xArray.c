#include "xArray.h"
#include <assert.h>
#include "xAtomic.h"
#include "xMessage.h"
#include "xBytes.h"

#define MIN_ARRAY_SIZE 16

typedef struct _xRealArray xRealArray;

struct _xRealArray
{
	xUInt8 *data;
	xUInt len;
	xUInt alloc;
	xUInt elt_size;
	xUInt zero_terminated : 1;
	xUInt clear : 1;
	xInt ref_count;
	xDestroyNotify clear_func;
};

#define x_array_elt_len(array, i) ((array)->elt_size * (i))
#define x_array_elt_pos(array, i) ((array)->data + x_array_elt_len((array), (i)))
#define x_array_elt_zero(array, pos, len) (memset(x_array_elt_pos((array), pos), 0, x_array_elt_len((array), len)))
#define x_array_zero_terminate(array) X_STMT_START { \
	if ((array)->zero_terminated) \
	x_array_elt_zero((array), (array)->len, 1); \
} X_STMT_END

static xUInt x_nearest_pow(xInt num);
static void x_array_maybe_expand(xRealArray *array, xInt len);

xArray* x_array_new(xBoolean zero_terminated, xBoolean clear, xUInt elt_size)
{
	x_return_val_if_fail(elt_size > 0, NULL);

	return x_array_sized_new(zero_terminated, clear, elt_size, 0);
}

xArray* x_array_sized_new(xBoolean zero_terminated, xBoolean clear, xUInt elt_size, xUInt reserved_size)
{
	xRealArray *array;

	x_return_val_if_fail(elt_size > 0, NULL);

	array = malloc(sizeof(xRealArray));
	array->data = NULL;
	array->len = 0;
	array->alloc = 0;
	array->zero_terminated = (zero_terminated ? 1 : 0);
	array->clear = (clear ? 1 : 0);
	array->elt_size = elt_size;
	array->ref_count = 1;
	array->clear_func = NULL;

	if (array->zero_terminated || reserved_size != 0)
	{
		x_array_maybe_expand(array, reserved_size);
		x_array_zero_terminate(array);
	}

	return (xArray*)array;
}

void x_array_set_clear_func(xArray *array, xDestroyNotify clear_func)
{
	xRealArray *rarray = (xRealArray*)array;

	x_return_if_fail(array);

	rarray->clear_func = clear_func;
}

xArray* x_array_ref(xArray *array)
{
	xRealArray *rarray = (xRealArray*)array;

	x_return_val_if_fail(array, NULL);

	x_atomic_int_inc(&rarray->ref_count);

	return array;
}

typedef enum
{
	FREE_SEGMENT = 1 << 0,
	PRESERVE_WRAPPER = 1 << 1
} ArrayFreeFlags;

static xChar* array_free(xRealArray *, ArrayFreeFlags);

void x_array_unref(xArray *array)
{
	xRealArray *rarray = (xRealArray*)array;

	x_return_if_fail(array);

	if (x_atomic_int_dec_and_test(&rarray->ref_count))
		array_free(rarray, FREE_SEGMENT);
}

xUInt x_array_get_element_size(xArray *array)
{
	xRealArray *rarray = (xRealArray*)array;

	x_return_val_if_fail(array, 0);

	return rarray->elt_size;
}

xChar* x_array_free(xArray *farray,  xBoolean free_segement)
{
	xRealArray *array = (xRealArray*)farray;
	ArrayFreeFlags flags;

	x_return_val_if_fail(array, NULL);

	flags = (free_segement ? FREE_SEGMENT : 0);

	if (!x_atomic_int_dec_and_test(&array->ref_count))
		flags |= PRESERVE_WRAPPER;

	return array_free(array, flags);
}

static xChar* array_free(xRealArray *array, ArrayFreeFlags flags)
{
	xChar *segment;

	if (flags & FREE_SEGMENT)
	{
		if (array->clear_func != NULL)
		{
			xUInt i;
			for (i = 0; i < array->len; i++)
				array->clear_func(x_array_elt_pos(array, i));
		}

		free(array->data);
		segment = NULL;
	}
	else
	{
		segment = (xChar*)array->data;
	}

	if (flags & PRESERVE_WRAPPER)
	{
		array->data = NULL;
		array->len = 0;
		array->alloc = 0;
	}
	else
	{
		free(array);
	}

	return segment;
}

xArray* x_array_append_vals(xArray *farray, xConstPointer data, xUInt len)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);

	x_array_maybe_expand(array, len);

	memcpy(x_array_elt_pos(array, array->len), data, x_array_elt_len(array, len));

	array->len += len;

	x_array_zero_terminate(array);

	return farray;
}

xArray* x_array_prepend_vals(xArray *farray, xConstPointer data, xUInt len)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);

	x_array_maybe_expand(array, len);

	memmove(x_array_elt_pos(array, len), x_array_elt_pos(array, 0), x_array_elt_len(array, array->len));
	memcpy(x_array_elt_pos(array, 0), data, x_array_elt_len(array, len));

	array->len += len;

	x_array_zero_terminate(array);

	return farray;
}

xArray* x_array_insert_vals(xArray *farray, xUInt index, xConstPointer data, xUInt len)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);

	x_array_maybe_expand(array, len);

	memmove(x_array_elt_pos(array, len + index), x_array_elt_pos(array, index), x_array_elt_len(array, array->len - index));
	memcpy(x_array_elt_pos(array, index), data, x_array_elt_len(array, len));

	array->len += len;

	x_array_zero_terminate(array);

	return farray;
}

xArray* x_array_set_size(xArray *farray, xUInt length)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);

	if (length > array->len)
	{
		x_array_maybe_expand(array, length - array->len);

		if (array->clear)
			x_array_elt_zero(array, array->len, length - array->len);
	}
	else if (length < array->len)
	{
		x_array_remove_range(farray, length, array->len - length);
	}

	array->len = length;
	x_array_zero_terminate(array);

	return farray;
}

xArray* x_array_remove_index(xArray *farray, xUInt index)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);
	x_return_val_if_fail(index < array->len, NULL);

	if (array->clear_func != NULL)
		array->clear_func(x_array_elt_pos(array, index));

	if (index != array->len - 1)
		memmove(x_array_elt_pos(array, index), x_array_elt_pos(array, index + 1), x_array_elt_len(array, array->len - index - 1));

	array->len -= 1;

	x_array_elt_zero(array, array->len, 1);
	return farray;
}

xArray* x_array_remove_index_fast(xArray *farray, xUInt index)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);
	x_return_val_if_fail(index < array->len, NULL);

	if (array->clear_func != NULL)
		array->clear_func(x_array_elt_pos(array, index));

	if (index != array->len - 1)
	{
		memcpy(x_array_elt_pos(array, index), x_array_elt_pos(array, array->len - 1), x_array_elt_len(array, 1));
	}

	array->len -= 1;

	x_array_elt_zero(array, array->len, 1);

	return farray;
}

xArray* x_array_remove_range(xArray *farray, xUInt index, xUInt length)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_val_if_fail(array, NULL);
	x_return_val_if_fail(index < array->len, NULL);
	x_return_val_if_fail(index + length <= array->len, NULL);

	if (array->clear_func != NULL)
	{
		xUInt i;
		for (i = 0; i < length; i++)
			array->clear_func(x_array_elt_pos(array, index + i));
	}

	if (index + length != array->len)
	{
		memmove(x_array_elt_pos(array, index), x_array_elt_pos(array, index + length), (array->len - (index + length)) * array->elt_size);
	}

	array->len -= length;

	x_array_elt_zero(array, array->len, length);

	return farray;
}

void x_array_sort(xArray *farray, xCompareFunc compare_func)
{
	xRealArray *array = (xRealArray*)farray;

	x_return_if_fail(array);

	qsort(array->data, array->len, array->elt_size, (xCompareFunc)compare_func);
}

/*
void x_array_sort_with_data
*/

static xUInt x_nearest_pow(xInt num)
{
	xUInt n = 1;

	while (n < (xUInt)num && n > 0)
		n <<= 1;

	return n ? n : (xUInt)num;
}

static void x_array_maybe_expand(xRealArray *array, xInt len)
{
	xUInt want_alloc = x_array_elt_len(array, array->len + len + array->zero_terminated);

	if (want_alloc > array->alloc)
	{
		want_alloc = x_nearest_pow(want_alloc);
		want_alloc = MAX(want_alloc, MIN_ARRAY_SIZE);

		array->data = realloc(array->data, want_alloc);

		memset(array->data + array->alloc, 0, want_alloc - array->alloc);

		array->alloc = want_alloc;
	}
}

typedef struct _xRealPtrArray xRealPtrArray;

struct _xRealPtrArray
{
	xPointer *pdata;
	xUInt len;
	xUInt alloc;
	xInt ref_count;
	xDestroyNotify element_free_func;
};

static void x_ptr_array_maybe_expand(xRealPtrArray *array, xInt len);

xPtrArray* x_ptr_array_new(void)
{
	return x_ptr_array_sized_new(0);
}

xPtrArray* x_ptr_array_sized_new(xUInt reserved_size)
{
	xRealPtrArray *array;

	array = malloc(sizeof(xRealPtrArray));

	array->pdata = NULL;
	array->len = 0;
	array->alloc = 0;
	array->ref_count = 1;
	array->element_free_func = NULL;

	if (reserved_size != 0)
		x_ptr_array_maybe_expand(array, reserved_size);

	return (xPtrArray*)array;
}

xPtrArray* x_ptr_array_new_with_free_func(xDestroyNotify element_free_func)
{
	xPtrArray *array;

	array = x_ptr_array_new();
	x_ptr_array_set_free_func(array, element_free_func);

	return array;
}

xPtrArray* x_ptr_array_new_full(xUInt reserved_size, xDestroyNotify element_free_func)
{
	xPtrArray *array;

	array = x_ptr_array_sized_new(reserved_size);
	x_ptr_array_set_free_func(array, element_free_func);

	return array;
}

void x_ptr_array_set_free_func(xPtrArray *array, xDestroyNotify element_free_func)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;

	x_return_if_fail(array);

	rarray->element_free_func = element_free_func;
}

xPtrArray* x_ptr_array_ref(xPtrArray *array)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;

	x_return_val_if_fail(array, NULL);

	x_atomic_int_inc(&rarray->ref_count);

	return array;
}

static xPointer* ptr_array_free(xPtrArray *, ArrayFreeFlags);

void x_ptr_array_unref(xPtrArray *array)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;

	if (x_atomic_int_dec_and_test(&rarray->ref_count))
		ptr_array_free(array, FREE_SEGMENT);
}

xPointer* x_ptr_array_free(xPtrArray *array, xBoolean free_segment)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	ArrayFreeFlags flags;

	x_return_val_if_fail(rarray, NULL);

	flags = (free_segment ? FREE_SEGMENT : 0);

	/* if others are holding a reference, preserve the wrapper but
	 * do free/return the data
	 */
	if (!x_atomic_int_dec_and_test(&rarray->ref_count))
		flags |= PRESERVE_WRAPPER;

	return ptr_array_free(array, flags);
}

static xPointer* ptr_array_free(xPtrArray *array, ArrayFreeFlags flags)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	xPointer *segment;

	if (flags & FREE_SEGMENT)
	{
		if (rarray->element_free_func != NULL)
		{
			x_ptr_array_foreach(array, (xFunc*)rarray->element_free_func, NULL);
		}

		free(rarray->pdata);
		segment = NULL;
	}
	else
	{
		segment = rarray->pdata;
	}

	if (flags & PRESERVE_WRAPPER)
	{
		rarray->pdata = NULL;
		rarray->len = 0;
		rarray->alloc = 0;
	}
	else
	{
		free(rarray);
	}

	return segment;
}

static void x_ptr_array_maybe_expand(xRealPtrArray *array, xInt len)
{
	if ((array->len + len) > array->alloc)
	{
		xUInt old_alloc = array->alloc;
		array->alloc = x_nearest_pow(array->len + len);
		array->pdata = realloc(array->pdata, sizeof(xPointer)* array->alloc);

		for (; old_alloc < array->alloc; old_alloc++)
			array->pdata[old_alloc] = NULL;
	}
}

void x_ptr_array_set_size(xPtrArray *array, xSize length)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;

	x_return_if_fail(rarray);

	if (length > rarray->len)
	{
		int i;
		x_ptr_array_maybe_expand(rarray, (length - rarray->len));
		/* This is not
		 *		memset(array->pdata + array->len, 0, sizeof(xPointer) * (length - array->len));
		 * to make it really portable. Remember (void*)NULL needn't be
		 * bitwise zero. It of course is silly not to use memset (..,0,..).
		 */
		for (i = rarray->len; (xUInt)i < length; i++)
			rarray->pdata[i] = NULL;
	}
	else if (length < rarray->len)
	{
		x_ptr_array_remove_range(array, length, rarray->len - length);
	}

	rarray->len = length;
}

xPointer x_ptr_array_remove_index(xPtrArray *array, xUInt index)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	xPointer result;

	x_return_val_if_fail(rarray, NULL);
	x_return_val_if_fail(index < rarray->len, NULL);

	result = rarray->pdata[index];

	if (rarray->element_free_func != NULL)
		rarray->element_free_func(rarray->pdata[index]);

	if (index != rarray->len - 1)
		memmove(rarray->pdata + index, rarray->pdata + index + 1, sizeof(xPointer)* (rarray->len - index - 1));

	rarray->len -= 1;

	rarray->pdata[rarray->len] = NULL;

	return result;
}

xPointer x_ptr_array_remove_index_fast(xPtrArray *array, xUInt index)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	xPointer result;

	x_return_val_if_fail(rarray, NULL);

	x_return_val_if_fail(index < rarray->len, NULL);

	result = rarray->pdata[index];

	if (rarray->element_free_func != NULL)
		rarray->element_free_func(rarray->pdata[index]);

	if (index != rarray->len - 1)
		rarray->pdata[index] = rarray->pdata[rarray->len - 1];

	rarray->len -= 1;

	rarray->pdata[rarray->len] = NULL;

	return result;
}

xPtrArray * x_ptr_array_remove_range(xPtrArray *array, xUInt index, xUInt length)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	xUInt n;

	x_return_val_if_fail(rarray != NULL, NULL);
	x_return_val_if_fail(index < rarray->len, NULL);
	x_return_val_if_fail(index + length <= rarray->len, NULL);

	if (rarray->element_free_func != NULL)
	{
		for (n = index; n < index + length; n++)
			rarray->element_free_func(rarray->pdata[n]);
	}

	if (index + length != rarray->len)
	{
		memmove(&rarray->pdata[index], &rarray->pdata[index + length], (rarray->len - (index + length)) * sizeof(xPointer));
	}

	rarray->len -= length;
	{
		xUInt i;
		for (i = 0; i < length; i++)
		{
			rarray->pdata[rarray->len + i] = NULL;
		}

		return array;
	}
}

xBoolean x_ptr_array_remove(xPtrArray *array, xPointer data)
{
	xUInt i;
	x_return_val_if_fail(array, FALSE);

	for (i = 0; i < array->len; i += 1)
	{
		if (array->pdata[i] == data)
		{
			x_ptr_array_remove_index(array, i);
			return TRUE;
		}
	}

	return FALSE;
}

xBoolean x_ptr_array_remove_fast(xPtrArray *array, xPointer data)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	xUInt i;

	x_return_val_if_fail(rarray, FALSE);

	for (i = 0; i < rarray->len; i += 1)
	{
		if (rarray->pdata[i] == data)
		{
			x_ptr_array_remove_index_fast(array, i);
			return TRUE;
		}
	}

	return FALSE;
}

void x_ptr_array_add(xPtrArray *array, xPointer data)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;
	x_return_if_fail(rarray);
	x_ptr_array_maybe_expand(rarray, 1);
	rarray->pdata[rarray->len++] = data;
}

void x_ptr_array_insert(xPtrArray *array, xInt index, xPointer data)
{
	xRealPtrArray *rarray = (xRealPtrArray*)array;

	x_return_if_fail(rarray);
	x_return_if_fail(index >= -1);
	x_return_if_fail(index <= (xInt)rarray->len);

	x_ptr_array_maybe_expand(rarray, 1);

	if (index < 0)
		index = rarray->len;

	if ((xUInt)index < rarray->len)
	{
		memmove(&(rarray->pdata[index + 1]), &(rarray->pdata[index]), (rarray->len - index) * sizeof(xPointer));
	}

	rarray->len++;
	rarray->pdata[index] = data;
}

void x_ptr_array_sort(xPtrArray *array, xCompareFunc compare_func)
{
	x_return_if_fail(array != NULL);

	qsort(array->pdata, array->len, sizeof (xPointer), compare_func);
}

// x_ptr_array_sort_with_data

void x_ptr_array_foreach(xPtrArray *array, xFunc func, xPointer user_data)
{
	xUInt i;

	x_return_if_fail(array);

	for (i = 0; i < array->len; i++)
		(*func)(array->pdata[i], user_data);
}

xByteArray* x_byte_array_new(void)
{
	return (xByteArray*)x_array_sized_new(FALSE, FALSE, 1, 0);
}

xByteArray* x_byte_array_new_take(xUInt8* data, xSize len)
{
	xByteArray *array;
	xRealArray *real;

	array = x_byte_array_new();
	real = (xRealArray*)array;

	assert(real->data == NULL);
	assert(real->len == 0);

	real->data = data;
	real->len = len;

	return array;
}

xByteArray* x_byte_array_sized_new(xUInt reserved_size)
{
	return (xByteArray*)x_array_sized_new(FALSE, FALSE, 1, reserved_size);
}

xUInt8* x_byte_array_free(xByteArray *array, xBoolean free_segment)
{
	return (xUInt8*)x_array_free((xArray*)array, free_segment);
}

xBytes* x_byte_array_free_to_bytes(xByteArray *array)
{
	xSize length;

	x_return_val_if_fail(array != NULL, NULL);

	length = array->len;
	return x_bytes_new_take(x_byte_array_free(array, FALSE), length);
}

xByteArray* x_byte_array_ref(xByteArray *array)
{
	return (xByteArray*)x_array_ref((xArray*)array);
}

void x_byte_array_unref(xByteArray *array)
{
	x_array_unref((xArray*)array);
}

xByteArray* x_byte_array_append(xByteArray *array, const xUInt8 *data, xUInt len)
{
	x_array_append_vals((xArray*)array, (xUInt8*)data, len);

	return array;
}

xByteArray* x_byte_array_prepend(xByteArray *array, const xUInt8 *data, xUInt len)
{
	x_array_prepend_vals((xArray*)array, (xUInt8*)data, len);

	return array;
}

xByteArray* x_byte_array_set_size(xByteArray *array, xUInt length)
{
	x_array_set_size((xArray*)array, length);
	
	return array;
}

xByteArray *x_byte_array_remove_index(xByteArray *array, xUInt index)
{
	x_array_remove_index((xArray*)array, index);

	return array;
}

xByteArray *x_byte_array_remove_index_fast(xByteArray *array, xUInt index)
{
	x_array_remove_index_fast((xArray*)array, index);

	return array;
}

xByteArray* x_byte_array_remove_range(xByteArray *array, xUInt index, xUInt length)
{
	x_return_val_if_fail(array, NULL);
	x_return_val_if_fail(index < array->len, NULL);
	x_return_val_if_fail(index + length <= array->len, NULL);

	return (xByteArray*)x_array_remove_range((xArray*)array, index, length);
}

void x_byte_array_sort(xByteArray *array, xCompareFunc compare_func)
{
	x_array_sort((xArray*)array, compare_func);
}

// void x_byte_array_sort_with_data