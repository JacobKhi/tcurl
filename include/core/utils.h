#pragma once

#include <stdarg.h>
#include <stddef.h>

/**
 * Trims leading and trailing whitespace from a string in-place.
 * Modifies the input string directly by moving content and adding null terminators.
 * 
 * @param s The string to trim (must be mutable). Does nothing if NULL.
 */
void str_trim(char *s);

/**
 * Trims only leading whitespace from a string.
 * Returns a pointer into the same string, skipping leading whitespace.
 * Does not modify the input string.
 * 
 * @param s The string to process.
 * @return Pointer to first non-whitespace character, or pointer to null terminator if all whitespace.
 */
char *str_trim_left(char *s);

/**
 * Trims only trailing whitespace from a string in-place.
 * Modifies the input string by adding null terminators to remove trailing whitespace.
 * 
 * @param s The string to trim (must be mutable). Does nothing if NULL.
 */
void str_trim_right(char *s);

/**
 * Performs case-insensitive string equality comparison.
 * Compares two strings ignoring case differences (ASCII only).
 * 
 * @param a First string to compare.
 * @param b Second string to compare.
 * @return 1 if strings are equal (ignoring case), 0 otherwise. Returns 0 if either string is NULL.
 */
int str_eq_ci(const char *a, const char *b);

/**
 * Appends formatted text to a dynamically-growing buffer.
 * Automatically grows the buffer capacity as needed using realloc.
 * Uses printf-style formatting for the appended text.
 * 
 * Thread safety: Not thread-safe. Caller must handle synchronization if needed.
 * 
 * @param buf Pointer to buffer pointer (will be updated if realloc occurs). Must not be NULL.
 * @param len Pointer to current string length in buffer. Must not be NULL.
 * @param cap Pointer to current buffer capacity in bytes. Must not be NULL.
 * @param fmt Printf-style format string.
 * @param ... Variable arguments for format string.
 * @return 1 on success, 0 on failure (OOM or formatting error).
 * 
 * Example:
 *   char *buf = NULL;
 *   size_t len = 0, cap = 0;
 *   str_appendf(&buf, &len, &cap, "Hello %s", "world");
 *   // buf now contains "Hello world", len=11, cap>=12
 */
int str_appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...);
