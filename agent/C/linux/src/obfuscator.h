/**
 *-------------------------------
 * Header file for C Obfuscator.
 *             Author: Umar Abdul
 *-------------------------------
 */

#ifndef OBFUSCATOR_H
#define OBFUSCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of strings marked for obfuscation to look for.
#define OBFS_MAX_OFFSETS_COUNT 10

/**
 * Handles strign obfuscation. `key` is encoding key, and `str` is the target string.
 * Returns a pointer to `str`
 */
char *obfs_encode(unsigned char key, char str[]);

/**
 * Same as obfs_encode(), but for decoding.
 * This is not used by the program itself, but for the obfuscated programs to use to dynamically decode
 * their obfuscated strings. The function should be copied to protected programs.
 */
char *obfs_decode(unsigned char key, char str[]);

// Find the absolute offset of `target` that is `len` bytes long inside file `rfo`
ssize_t obfs_find_offset(FILE *rfo, const void *target, size_t len);

// Copy `len` bytes from file `src` into `dest`
size_t obfs_filecpy(FILE *dest, FILE *src, size_t len);

// Keep reading from a file file until a NULL byte or EOF. Return the number of bytes read.
size_t obfs_read_until_null(FILE *rfo);

/**
 * Obfuscate strings in file `src` using `key` and write to `dest`
 */
void obfs_run(FILE *dest, FILE *src, unsigned char key, short int verbose);

#endif // OBFUSCATOR_H

