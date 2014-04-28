#include "xStringList.h"
#include "xMessage.h"

static void free_string(xString *str)
{
	x_string_free(str, TRUE);
}

xStringList * x_string_list_new(void)
{
	xStringList *ret = (xStringList*)malloc(sizeof(xStringList));
	ret->list = x_ptr_array_new();
	ret->count = ret->list->len;
	x_ptr_array_set_free_func(ret->list, (xDestroyNotify)free_string);
	return ret;
}

xStringList * x_string_list_sized_new(xSize reserved_size)
{
	xStringList *ret = (xStringList*)malloc(sizeof(xStringList));
	ret->list = x_ptr_array_sized_new(reserved_size);
	ret->count = ret->list->len;
	x_ptr_array_set_free_func(ret->list, (xDestroyNotify)free_string);
	return ret;
}

void x_string_list_free(xStringList *list)
{
	x_return_if_fail(list);

	x_return_if_fail(list->list);

	x_ptr_array_free(list->list, TRUE);

	free(list);
}

xStringList * x_string_list_append(xStringList *list, const xChar *value)
{
	x_return_val_if_fail(list, NULL);
	x_return_val_if_fail(list->list, list);
	x_return_val_if_fail(value, list);
	
	xString *str = x_string_new(value);
	x_ptr_array_add(list->list, str);
	list->count = list->list->len;

	return list;
}

xStringList * x_string_list_append_len(xStringList *list, const xChar *value, xSSize len)
{
	x_return_val_if_fail(list, NULL);
	x_return_val_if_fail(list->list, list);
	x_return_val_if_fail(value, list);

	xString *str = x_string_new("");
	x_string_append_len(str, value, len);
	x_ptr_array_add(list->list, str);
	list->count = list->list->len;

	return list;
}

xStringList * x_string_list_append_str(xStringList *list, xString *str)
{
	x_return_val_if_fail(list, NULL);
	x_return_val_if_fail(list->list, list);
	x_return_val_if_fail(str, list);

	xString *str_new = x_string_new("");
	str_new = x_string_assign(str_new, str->str);
	x_ptr_array_add(list->list, str_new);
	list->count = list->list->len;

	return list;
}
