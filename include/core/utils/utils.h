#pragma once

#include <stdarg.h>
#include <stddef.h>

/* Trim whitespace (modifies string in-place) */
void str_trim(char *s);

/* Return pointer to first non-whitespace (doesn't modify) */
char *str_trim_left(char *s);

/* Trim trailing whitespace (modifies in-place) */
void str_trim_right(char *s);

/* Case-insensitive string comparison */
int str_eq_ci(const char *a, const char *b);

/* Append formatted text to growable buffer */
int str_appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...);
