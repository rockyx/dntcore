#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XSTRINGLIST_H__
#define __DNT_CORE_XSTRINGLIST_H__

#include <dnt/core/xArray.h>
#include <dnt/core/xString.h>

X_CORE_BEGIN_DECLS

typedef struct _xStringList xStringList;

struct _xStringList
{
	xPtrArray *list;
	xSize count;
};

#define x_string_list_index(str_list, index) (((xString*)x_ptr_array_index((str_list)->list, (index)))->str)

xStringList * x_string_list_new(void);
xStringList * x_string_list_sized_new(xSize reserved_size);
void x_string_list_free(xStringList *list);
xStringList * x_string_list_append(xStringList *list, const xChar *value);
xStringList * x_string_list_append_len(xStringList *list, const xChar *value, xSSize len);
xStringList * x_string_list_append_str(xStringList *list, xString *value);

X_CORE_END_DECLS

#endif /* __DNT_CORE_XSTRINGLIST_H__ */
