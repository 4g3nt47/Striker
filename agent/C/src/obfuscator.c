/**
 *--------------------------------------------------------------------------------------
 *      Obfuscator - A library for obfuscating strings in a program.
 * This is designed to prevent sensitive strings from showing up when dumping strings
 * from a binary, but the encoding scheme is not that secure.
 *      For a protected binary to be able to use it's protected strings, it needs to have
 * access to the obfs_decode() function and the key used. The recommended way to do this
 * is to simply copy the code of the obfs_decode() function to your source files, which
 * is self contained to avoid bloating up your program.
 *                                                                    Author: Umar Abdul
 *--------------------------------------------------------------------------------------
 */

#ifdef __WIN32__
  #include <windows.h>
#endif
#include "obfuscator.h"

// The marker for strings to obfuscate.
const char *OBFS_MARKER = "[OBFS_ENC]";

char *obfs_encode(unsigned char key, char str[]){
  
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

char *obfs_decode(unsigned char key, char str[]){

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

long obfs_find_offset(FILE *rfo, const void *target, size_t len){

  void *buff = malloc(len);
  int c;
  size_t n = fread(buff, 1, len, rfo);
  if (!n)
    return -1;
  while (1){
    if (!memcmp(buff, target, len)){
      free(buff);
      return ftell(rfo) - len;
    }
    for (int i = 1; i < len; i++) // Shift all bytes back one place, discarding the first byte.
      *((unsigned char *)buff + (i - 1)) = *((unsigned char *)buff + i);
    // Load one new byte to the end of the buffer.
    c = fgetc(rfo);
    if (c == EOF)
      break;
    *((unsigned char *)buff + (len - 1)) = c;
  }
  free(buff);
  return -1;
}

size_t obfs_filecpy(FILE *dest, FILE *src, size_t len){

  size_t n = 0;
  int c;
  for (; n < len; n++){
    if ((c = fgetc(src)) == EOF)
      break;
    fputc(c, dest);
  }
  return n;
}

size_t obfs_read_until_null(FILE *rfo){

  size_t n = 0;
  int c;
  while (!feof(rfo)){
    c = fgetc(rfo);
    if (c == EOF || c == 0)
      break;
    n++;
  }
  return n;
}

void obfs_run(FILE *dest, FILE *src, unsigned char key, short int verbose){

  if (verbose)
    printf("[*] Finding strings marked for obfuscation...\n");
  unsigned short marker_len = strlen(OBFS_MARKER);
  long *offsets = malloc((OBFS_MAX_OFFSETS_COUNT * 2) * sizeof(long));
  unsigned int offsets_count = 0;
  long target_len;
  int c;
  for (int index = 0; index < OBFS_MAX_OFFSETS_COUNT && (!feof(src)); index++){
    long offset = obfs_find_offset(src, OBFS_MARKER, marker_len);
    if (offset == -1)
      break;
    // `src` if now pointed to the beginning of a target string (with the marker skipped).
    target_len = obfs_read_until_null(src);
    *(offsets + (index * 2)) = offset;
    *(offsets + (index * 2 + 1)) = target_len + marker_len;
    offsets_count++;
  }
  if (verbose)
    printf("[+] %d targets identified!\n", offsets_count);
  if (offsets_count == 0)
    goto end;
  if (verbose)
    printf("[*] Obfuscating strings using key: 0x%02x...\n", key);
  rewind(src);
  for (int i = 0; i < offsets_count; i++){
    long offset = *(offsets + (i * 2));
    long target_len = *(offsets + (i * 2 + 1));
    // Copy all bytes preceding the obfuscated string.
    // This will point `src` to the beginning of the marker, and `dest` to the place our obfuscated string should be.
    obfs_filecpy(dest, src, offset - ftell(src));
    fseek(src, marker_len, SEEK_CUR); // Skip over the marker.
    // Encode the string.
    long org_str_len = target_len - marker_len;
    char *buff = malloc(target_len); // This also includes the length of the marker, which we will fill with nulls.
    memset(buff, 0, target_len);
    fread(buff, 1, org_str_len, src);
    obfs_encode(key, buff);
    fwrite(buff, 1, target_len, dest); // Write the encoded string with marker removed.
    free(buff);
    if (verbose)
      printf("[+] Offset 0x%08lx : %ld bytes...\n", offset, org_str_len);
  }
  // Copy the remaining bytes.
  if (verbose)
    printf("[*] Copying remaining bytes...\n");
  while ((c = fgetc(src)) != EOF)
    fputc(c, dest);
  printf("[+] %d strings obfuscated :)\n", offsets_count);

  // We are done :)
  end:
    free(offsets);
}
