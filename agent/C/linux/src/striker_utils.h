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
 *---------
 * Buffers.
 *---------
 */

// The buffer struct
typedef struct{
  void *buffer; // The buffer itself.
  size_t size; // The size of the buffer.
  size_t used; // The number of bytes in use.
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

/**
 *-------------
 * FIFO queues.
 *-------------
 */

typedef struct {
  size_t size; // The max size of the queue.
  size_t count; // The number of items in the queue.
  size_t pos; // Index of the value to read next.
  void **items; // Pointer to all the items in the queue.
} queue;

// Create a queue for a maximum of `size` elements.
queue *queue_init(size_t size);

// Add the item `q` to the end of a queue. Returns the offset of the item, -1 on error.
ssize_t queue_put(queue *q, void *e);

// Get the next item in a queue. Returns NULL on error.
void *queue_get(queue *q);

// Remove an item at the given offset.
void *queue_remove(queue *q, size_t pos);

// Returns 1 if there is no space in the queue for a new item.
unsigned short queue_full(queue *q);

// Returns 1 if the queue is empty.
unsigned short queue_empty(queue *q);

// Returns true if the end of the queue has been reached.
unsigned short queue_exhausted(queue *q);

// Jump to an offset in the qeueu. Returns 1 on success.
unsigned short queue_seek(queue *q, size_t pos);

// Free the queue and all the pointers added to it. If `items` is non-zero, free() will be called on all the queue items.
void queue_free(queue *q, unsigned short items);

/**
 *----------------------
 * File system utilites.
 *----------------------
 */

// Find the absolute offset of `target` that is `len` bytes long inside file `rfo`. Returns -1 if not found.
ssize_t find_offset(FILE *rfo, const void *target, size_t len);

// Copy `len` bytes from file `src` into `dest`
ssize_t filecpy(FILE *dest, FILE *src, size_t len);

// Keep reading from a file file until a NULL byte or EOF. Return the number of bytes read.
size_t read_until_null(FILE *rfo);

#endif // STRIKER_UTILS_H
