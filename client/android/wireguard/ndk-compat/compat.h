/* SPDX-License-Identifier: BSD
 *
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 *
 */

#if defined(__ANDROID_API__) && __ANDROID_API__ < 18
#  include <stdio.h>
ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp);
ssize_t getline(char** buf, size_t* bufsiz, FILE* fp);
#endif

#if defined(__ANDROID_API__) && __ANDROID_API__ < 24
char* strchrnul(const char* s, int c);
#endif
