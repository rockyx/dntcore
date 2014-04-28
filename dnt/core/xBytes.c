#include "xBytes.h"
#include "xMessage.h"
#include "xAtomic.h"

struct _xBytes
{
	xConstPointer data; /* may be NULL if (size == 0) */
	xSize size; /* may be 0 */
	xInt ref_count;
	xDestroyNotify free_func;
	xPointer user_data;
};

static xPointer memdup(xConstPointer mem, xUInt byte_size)
{
	xPointer new_mem;

	if (mem)
	{
		new_mem = malloc(byte_size);
		memcpy(new_mem, mem, byte_size);
	}
	else
	{
		new_mem = NULL;
	}

	return new_mem;
}

xBytes* x_bytes_new(xConstPointer data, xSize size)
{
	x_return_val_if_fail(data != NULL || size == 0, NULL);

	return x_bytes_new_take(memdup(data, size), size);
}

xBytes* x_bytes_new_take(xPointer data, xSize size)
{
	return x_bytes_new_with_free_func(data, size, free, data);
}

xBytes* x_bytes_new_static(xConstPointer data, xSize size)
{
	return x_bytes_new_with_free_func(data, size, NULL, NULL);
}

xBytes* x_bytes_new_with_free_func(xConstPointer data, xSize size, xDestroyNotify free_func, xPointer user_data)
{
	xBytes *bytes;

	x_return_val_if_fail(data != NULL || size == 0, NULL);

	bytes = malloc(sizeof(xBytes));
	bytes->data = data;
	bytes->size = size;
	bytes->free_func = free_func;
	bytes->user_data = user_data;
	bytes->ref_count = 1;

	return bytes;
}

xBytes* x_bytes_new_from_bytes(xBytes *bytes, xSize offset, xSize length)
{
	/* Note that length may be 0. */
	x_return_val_if_fail(bytes != NULL, NULL);
	x_return_val_if_fail(offset <= bytes->size, NULL);
	x_return_val_if_fail(offset + length <= bytes->size, NULL);

	return x_bytes_new_with_free_func((xChar*)bytes->data + offset, length, x_bytes_unref, x_bytes_ref(bytes));
}

xConstPointer x_bytes_get_data(xBytes *bytes, xSize *size)
{
	x_return_val_if_fail(bytes != NULL, NULL);
	if (size)
		*size = bytes->size;
	return bytes->data;
}

xSize x_bytes_get_size(xBytes *bytes)
{
	x_return_val_if_fail(bytes != NULL, 0);
	return bytes->size;
}

xBytes* x_bytes_ref(xBytes *bytes)
{
	x_return_val_if_fail(bytes != NULL, NULL);

	x_atomic_int_inc(&bytes->ref_count);

	return bytes;
}

void x_bytes_unref(xBytes *bytes)
{
	if (bytes == NULL)
		return;

	if (x_atomic_int_dec_and_test(&bytes->ref_count))
	{
		if (bytes->free_func != NULL)
			bytes->free_func(bytes->user_data);
		free(bytes);
	}
}

xBoolean x_bytes_equal(xConstPointer bytes1, xConstPointer bytes2)
{
	const xBytes *b1 = bytes1;
	const xBytes *b2 = bytes2;

	x_return_val_if_fail(bytes1 != NULL, FALSE);
	x_return_val_if_fail(bytes2 != NULL, FALSE);

	return b1->size == b2->size && memcmp(b1->data, b2->data, b1->size) == 0;
}

xUInt x_bytes_hash(xConstPointer bytes)
{
	const xBytes *a = bytes;
	const signed char *p, *e;
	xUInt32 h = 5381;

	x_return_val_if_fail(bytes != NULL, 0);
	for (p = (signed char*)a->data, e = (signed char*)a->data + a->size; p != e; p++)
		h = (h << 5) + h + *p;

	return h;
}

xInt x_bytes_compare(xConstPointer bytes1, xConstPointer bytes2)
{
	const xBytes *b1 = bytes1;
	const xBytes *b2 = bytes2;
	xInt ret;

	x_return_val_if_fail(bytes1 != NULL, 0);
	x_return_val_if_fail(bytes2 != NULL, 0);

	ret = memcmp(b1->data, b2->data, MIN(b1->size, b2->size));
	if (ret == 0 && b1->size != b2->size)
		ret = b1->size < b2->size ? -1 : 1;
	return ret;
}

static xPointer try_steal_and_unref(xBytes *bytes, xDestroyNotify free_func, xSize *size)
{
	xPointer result;

	if (bytes->free_func != free_func || bytes->data == NULL)
		return NULL;

	/* Are we the only reference? */
	if (x_atomic_int_get(&bytes->ref_count) == 1)
	{
		*size = bytes->size;
		result = (xPointer)bytes->data;
		free(bytes);
		return result;
	}

	return NULL;
}

xPointer x_bytes_unref_to_data(xBytes *bytes, xSize *size)
{
	xPointer result;

	x_return_val_if_fail(bytes != NULL, NULL);
	x_return_val_if_fail(size != NULL, NULL);

	/*
	 * Optimal path: if this is was the last reference, then we can return
	 * the data from this xBytes without copying.
	 */
	result = try_steal_and_unref(bytes, free, size);
	if (result == NULL)
	{
		/*
		 * Copy: Non malloc (or compatible) allocater, or static memory,
		 * so we have to copy, and then unref.
		 */
		result = memdup(bytes->data, bytes->size);
		*size = bytes->size;
		x_bytes_unref(bytes);
	}

	return result;
}

xByteArray* x_bytes_unref_to_array(xBytes *bytes)
{
	xPointer data;
	xSize size;

	x_return_val_if_fail(bytes != NULL, NULL);

	data = x_bytes_unref_to_data(bytes, &size);
	return x_byte_array_new_take(data, size);
}