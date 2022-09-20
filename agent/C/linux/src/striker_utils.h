/**
 *-------------------------------------------------
 * Header file for the shared utilities of Striker.
 *                               Author: Umar Abdul
 *-------------------------------------------------
 */

#ifndef STRIKER_UTILS_H
#define STRIKER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * A struct for working with buffers.
 * `buffer` is the buffer (very helpful :)
 * `size` is total number of bytes allocated to it.
 * `used` is used to track number of bytes in use.
 */
typedef struct{
  void *buffer;
  size_t size;
  size_t used;
} buffer;

/**
 * Create a new buffer of `size` bytes, and initialize all it's bytes to null.
 * If `size` is zero, the call to malloc() will use 1 to still allocate some memory but buffer.size
 * and buffer.used will be kept at 0.
 */
buffer *create_buffer(size_t size);

// Resize a buffer. It DOES NOT change the `used` property unless the `new_size` is less than it.
void resize_buffer(buffer *buff, size_t new_size);

/**
 * Append a max of `len` bytes from `src` to `dest` buffer.
 * Note: resize_buffer() will be used to resize the dest buffer if not enough space is available.
 * Returns the number of bytes appended.
 */
size_t append_buffer(buffer *dest, const void *src, size_t len);
/**
 * Empty `dest` buffer and copy the contents of null-terminated `src` as it's new value.
 * It DOES NOT null-terminate the `dest` buffer.
 */
size_t buffer_strcpy(buffer *dest, const char *src);

// Convert a buffer to a null-terminated string. Uses the `used` var to determine length.
char *buffer_to_string(buffer *buff);

// Free a buffer. Called when a buffer is no longer needed.
void free_buffer(buffer *buff);

#endif // STRIKER_UTILS_H
