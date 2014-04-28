#include "xString.h"
#include "xMessage.h"


#define MY_MAXSIZE ((xSize)-1)

static xSize nearest_power(xSize base, xSize num)
{
	if (num > MY_MAXSIZE / 2)
	{
		return MY_MAXSIZE;
	}
	else
	{
		xSize n = base;

		while (n < num)
			n <<= 1;

		return n;
	}
}

static void x_string_maybe_expand(xString *string, xSize len)
{
	if (string->len + len >= string->allocated_len)
	{
		string->allocated_len = nearest_power(1, string->len + len + 1);
		string->str = realloc(string->str, string->allocated_len);
	}
}

xString* x_string_sized_new(xSize dfl_size)
{
	xString *string = malloc(sizeof(xString));

	string->allocated_len = 0;
	string->len = 0;
	string->str = NULL;

	x_string_maybe_expand(string, MAX(dfl_size, 2));
	string->str[0] = 0;

	return string;
}

xString* x_string_new(const xChar* init)
{
	xString *string;

	if (init == NULL || *init == '\0')
		string = x_string_sized_new(2);
	else
	{
		xInt len;

		len = strlen(init);
		string = x_string_sized_new(len + 2);

		x_string_append_len(string, init, len);
	}
	return string;
}

xString* x_string_new_len(const xChar *init, xSSize len)
{
	xString *string;

	if (len < 0)
		return x_string_new(init);
	else
	{
		string = x_string_sized_new(len);

		if (init)
			x_string_append_len(string, init, len);

		return string;
	}
}

xChar* x_string_free(xString *string, xBoolean free_segment)
{
	xChar *segment;

	x_return_val_if_fail(string != NULL, NULL);

	if (free_segment)
	{
		free(string->str);
		segment = NULL;
	}
	else
	{
		segment = string->str;
	}

	free(string);

	return segment;
}

//xBytes* x_string_free_to_bytes(xString *string)

xBoolean x_string_equal(const xString *v, const xString *v2)
{
	xChar *p, *q;
	xString *string1 = (xString*)v;
	xString *string2 = (xString*)v2;
	xSize i = string1->len;

	if (i != string2->len)
		return FALSE;

	p = string1->str;
	q = string2->str;

	while (i)
	{
		if (*p != *q)
			return FALSE;
		p++;
		q++;
		i--;
	}

	return TRUE;
}

xUInt x_string_hash(const xString *str)
{
	const xChar *p = str->str;
	xSize n = str->len;
	xUInt h = 0;

	/* 31 bit hash function */
	while (n--)
	{
		h = (h << 5) - h + *p;
		p++;
	}

	return h;
}

xString* x_string_assign(xString *string, const xChar *rval)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(rval != NULL, string);

	/* Make sure assigning to itself doens't corrupt the string. */
	if (string->str != rval)
	{
		/* Assigning from substring should be OK, since
		 * x_string_truncate() does not reallocate.
		 */
		x_string_truncate(string, 0);
		x_string_append(string, rval);
	}

	return string;
}

xString* x_string_truncate(xString *string, xSize len)
{
	x_return_val_if_fail(string != NULL, NULL);

	string->len = MIN(len, string->len);
	string->str[string->len] = 0;

	return string;
}

xString* x_string_set_size(xString *string, xSize len)
{
	x_return_val_if_fail(string != NULL, NULL);
	
	if (len >= string->allocated_len)
		x_string_maybe_expand(string, len - string->len);

	string->len = len;
	string->str[len] = 0;

	return string;
}

xString* x_string_insert_len(xString *string, xSSize pos, const xChar *val, xSSize len)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(len == 0 || val != NULL, string);

	if (len == 0)
		return string;

	if (len < 0)
		len = strlen(val);

	if (pos < 0)
		pos = string->len;
	else
		x_return_val_if_fail((xSize)pos <= string->len, string);

	/* Check whether val represents a substring of string.
	 * This test probably violates chapter and verse of the C standards,
	 * since ">=" and "<=" are only valid when val really is a substring.
	 * In practice, it will work on modern archs.
	 */
	if (val >= string->str && val <= string->str + string->len)
	{
		xSSize offset = val - string->str;
		xSSize precount = 0;

		x_string_maybe_expand(string, len);
		val = string->str + offset;

		/* At this point, val is valid again. */

		/* Open up space where we are going to insert. */
		if ((xSize)pos < string->len)
			memmove(string->str + pos + len, string->str + pos, string->len - pos);

		/* Move the source part before the gap, if any. */
		if (offset < pos)
		{
			precount = MIN(len, pos - offset);
			memcpy(string->str + pos, val, precount);
		}

		/* Move the source part after the gap, if any. */
		if (len > precount)
			memcpy(string->str + pos + precount, val + /* Already moved: */ precount + /* Space opened up: */ len, len - precount);
	}
	else
	{
		x_string_maybe_expand(string, len);

		/* If we aren't appending at the end, move a hunk
		 * of the old string to the end, opening up space
		 */
		if ((xSize)pos < string->len)
			memmove(string->str + pos + len, string->str + pos, string->len - pos);

		/* Insert the new string */
		if (len == 1)
			string->str[pos] = *val;
		else
			memcpy(string->str + pos, val, len);
	}

	string->len += len;
	string->str[string->len] = 0;

	return string;
}

#define SUB_DELIM_CHARS "!$&'()*+,;="

static xBoolean is_vaild(char c, const char *reserved_chars_allowed)
{
	if (isalnum(c) ||
		c == '-' ||
		c == '.' ||
		c == '_' ||
		c == '~')
		return TRUE;

	if (reserved_chars_allowed &&
		strchr(reserved_chars_allowed, c) != NULL)
		return TRUE;

	return FALSE;
}

//static xBoolean xUnichar_ok(xUnichar c)

//xString* x_string_append_uri_escaped(xString *string, const xChar *unescaped, const xChar *reserved_chars_allowed, xBoolean allow_utf8)

xString* x_string_append(xString *string, const xChar *val)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(val != NULL, string);

	return x_string_insert_len(string, -1, val, -1);
}

xString* x_string_append_len(xString *string, const xChar *val, xSSize len)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(len == 0 || val != NULL, string);

	return x_string_insert_len(string, -1, val, len);
}

#undef x_string_append_c
xString* x_string_append_c(xString *string, xChar c)
{
	x_return_val_if_fail(string != NULL, NULL);

	return x_string_insert_c(string, -1, c);
}

//xString* x_string_append_unichar(xString *string, xUnichar wc)

xString* x_string_prepend(xString *string, const xChar *val)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(val != NULL, string);

	return x_string_insert_len(string, 0, val, -1);
}

xString* x_string_prepend_len(xString *string, const xChar *val, xSSize len)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(val != NULL, string);

	return x_string_insert_len(string, 0, val, len);
}

xString* x_string_prepend_c(xString *string, xChar c)
{
	x_return_val_if_fail(string != NULL, NULL);

	return x_string_insert_c(string, 0, c);
}

//String* x_string_prepend_unichar(xString *string, xUnichar wc)

xString* x_string_insert(xString *string, xSSize pos, const xChar *val)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(val != NULL, string);

	if (pos >= 0)
		x_return_val_if_fail((xSize)pos <= string->len, string);

	return x_string_insert_len(string, pos, val, -1);
}

xString* x_string_insert_c(xString *string, xSSize pos, xChar c)
{
	x_return_val_if_fail(string != NULL, NULL);

	x_string_maybe_expand(string, 1);

	if (pos < 0)
		pos = string->len;
	else
		x_return_val_if_fail((xSize)pos <= string->len, string);

	/* If not just an append, move the old stuff */
	if ((xSize)pos < string->len)
		memmove(string->str + pos + 1, string->str + pos, string->len - pos);

	string->str[pos] = c;

	string->len += 1;

	string->str[string->len] = 0;

	return string;
}

//xString* x_string_insert_unichar(xString *string, xSSize pos, xUnichar wc)

xString* x_string_overwrite(xString *string, xSize pos, const xChar *val)
{
	x_return_val_if_fail(val != NULL, string);
	return x_string_overwrite_len(string, pos, val, strlen(val));
}

xString* x_string_overwrite_len(xString *string, xSize pos, const xChar *val, xSSize len)
{
	xSize end;

	x_return_val_if_fail(string != NULL, NULL);

	if (!len)
		return string;

	x_return_val_if_fail(val != NULL, string);
	x_return_val_if_fail(pos <= string->len, string);

	if (len < 0)
		len = strlen(val);

	end = pos + len;

	if (end > string->len)
		x_string_maybe_expand(string, end - string->len);

	memcpy(string->str + pos, val, len);

	if (end > string->len)
	{
		string->str[end] = '\0';
		string->len = end;
	}

	return string;
}

xString* x_string_erase(xString *string, xSSize pos, xSSize len)
{
	x_return_val_if_fail(string != NULL, NULL);
	x_return_val_if_fail(pos >= 0, string);
	x_return_val_if_fail((xSize)pos <= string->len, string);

	if (len < 0)
		len = string->len - pos;
	else
	{
		x_return_val_if_fail((xSize)(pos + len) <= string->len, string);

		if ((xSize)(pos + len) < string->len)
			memmove(string->str + pos, string->str + pos + len, string->len - (pos + len));
	}

	string->len -= len;

	string->str[string->len] = 0;

	return string;
}

xString* x_string_ascii_down(xString *string)
{
	xChar *s;
	xInt n;

	x_return_val_if_fail(string != NULL, NULL);

	n = string->len;
	s = string->str;

	while (n)
	{
		*s = tolower(*s);
		s++;
		n--;
	}

	return string;
}

xString* x_string_ascii_up(xString *string)
{
	xChar *s;
	xInt n;

	x_return_val_if_fail(string != NULL, NULL);

	n = string->len;
	s = string->str;

	while (n)
	{
		*s = toupper(*s);
		s++;
		n--;
	}

	return string;
}

void x_string_append_vprintf(xString *string, const xChar *format, va_list args)
{
	xChar buf[1024];
	xInt len;

	x_return_if_fail(string != NULL);
	x_return_if_fail(format != NULL);

	len = vsprintf(buf, format, args);

	if (len >= 0)
	{
		x_string_maybe_expand(string, len);
		memcpy(string->str + string->len, buf, len + 1);
		string->len += len;
	}
}

void x_string_vprintf(xString *string, const xChar *format, va_list args)
{
	x_string_truncate(string, 0);
	x_string_append_vprintf(string, format, args);
}

void x_string_printf(xString *string, const xChar *format, ...)
{
	va_list args;

	x_string_truncate(string, 0);

	va_start(args, format);
	x_string_append_vprintf(string, format, args);
	va_end(args);
}

void x_string_append_printf(xString *string, const xChar *format, ...)
{
	va_list args;

	va_start(args, format);
	x_string_append_vprintf(string, format, args);
	va_end(args);
}

