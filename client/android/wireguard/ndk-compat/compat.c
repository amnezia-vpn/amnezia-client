/* SPDX-License-Identifier: BSD
 *
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 *
 */

#define FILE_IS_EMPTY

#if defined(__ANDROID_API__) && __ANDROID_API__ < 18
#  undef FILE_IS_EMPTY
#  include <stdio.h>
#  include <stdlib.h>

ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp) {
  char *ptr, *eptr;

  if (*buf == NULL || *bufsiz == 0) {
    *bufsiz = BUFSIZ;
    if ((*buf = malloc(*bufsiz)) == NULL) return -1;
  }

  for (ptr = *buf, eptr = *buf + *bufsiz;;) {
    int c = fgetc(fp);
    if (c == -1) {
      if (feof(fp)) {
        ssize_t diff = (ssize_t)(ptr - *buf);
        if (diff != 0) {
          *ptr = '\0';
          return diff;
        }
      }
      return -1;
    }
    *ptr++ = c;
    if (c == delimiter) {
      *ptr = '\0';
      return ptr - *buf;
    }
    if (ptr + 2 >= eptr) {
      char* nbuf;
      size_t nbufsiz = *bufsiz * 2;
      ssize_t d = ptr - *buf;
      if ((nbuf = realloc(*buf, nbufsiz)) == NULL) return -1;
      *buf = nbuf;
      *bufsiz = nbufsiz;
      eptr = nbuf + nbufsiz;
      ptr = nbuf + d;
    }
  }
}

ssize_t getline(char** buf, size_t* bufsiz, FILE* fp) {
  return getdelim(buf, bufsiz, '\n', fp);
}
#endif

#if defined(__ANDROID_API__) && __ANDROID_API__ < 24
#  undef FILE_IS_EMPTY
#  include <string.h>

char* strchrnul(const char* s, int c) {
  char* x = strchr(s, c);
  if (!x) return (char*)s + strlen(s);
  return x;
}
#endif

#ifdef FILE_IS_EMPTY
#  undef FILE_IS_EMPTY
static char ____x __attribute__((unused));
#endif
