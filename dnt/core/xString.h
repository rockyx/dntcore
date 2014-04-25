#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XSTRING_H__
#define __DNT_CORE_XSTRING_H__

#include <dnt/core/xGlobal.h>

X_CORE_BEGIN_DECLS

typedef struct _xString xString;

struct _xString
{
	xChar *str;
	xSize len;
	xSize allocated_len;
};

xString* x_string_new(const xChar *init);
xString* x_string_new_len(const xChar *init, xSSize len);
xString* x_string_sized_new(xSize dfl_size);
xChar* x_string_free(xString *string, xBoolean free_segment);
//xBytes* x_string_free_to_bytes(xString *string);
xBoolean x_string_equal(const xString *v, const xString *v2);
xUInt x_string_hash(const xString *str);
xString* x_string_assign(xString *string, const xChar *rval);
xString* x_string_truncate(xString *string, xSize len);
xString* x_string_set_size(xString *string, xSize len);
xString* x_string_insert_len(xString *string, xSSize pos, const xChar *val, xSSize len);
xString* x_string_append(xString *string, const xChar *val);
xString* x_string_append_len(xString *string, const xChar *val, xSSize len);
xString* x_string_append_c(xString *string, xChar c);
//xString* x_string_append_unichar(xString *string, xUnichar wc);
xString* x_string_prepend(xString *string, const xChar *val);
xString* x_string_prepend_c(xString *string, xChar c);
//xString* x_string_prepend_unichar(xString *string, xUnichar wc);
xString* x_string_prepend_len(xString *string, const xChar *val, xSSize len);
xString* x_string_insert(xString *string, xSSize pos, const xChar *val);
xString* x_string_insert_c(xString *string, xSSize pos, xChar c);
//xString* x_string_insert_unichar(xString *string, xSSize pos, xUnichar wc);
xString* x_string_overwrite(xString *string, xSize pos, const xChar *val);
xString* x_string_overwrite_len(xString *string, xSize pos, const xChar *val, xSSize len);
xString* x_string_erase(xString *string, xSSize pos, xSSize len);
xString* x_string_ascii_down(xString *string);
xString* x_string_ascii_up(xString *string);
void x_string_vprintf(xString *string, const xChar *format, va_list args);
void x_string_printf(xString *string, const xChar *format, ...);
void x_string_append_vprintf(xString *string, const xChar *format, va_list args);
void x_string_append_printf(xString *string, const xChar *format, ...);
//xString* x_string_append_uri_escaped(xString *string, const xChar *unescaped, const xChar *reserved_chars_allowed, xBoolean allow_utf8);
static xString* x_string_append_c_inline(xString *string, xChar c)
{
	if (string->len + 1 < string->allocated_len)
	{
		string->str[string->len++] = c;
		string->str[string->len] = 0;
	}
	else
	{
		x_string_insert_c(string, -1, c);
	}
	return string;
}

X_CORE_END_DECLS

#endif // __DNT_CORE_XSTRING_H__
