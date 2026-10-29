/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int format_variadic(char *buffer, size_t count, const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, count, format, arguments);
  va_end(arguments);
  return result;
}

int main(void) {
  char line[128];
  char small[5] = {0};
  char single[1] = {'x'};
  const char expected[] = "[hello][I] core=1 seq=4294967295 err=-2147483648 %";

  int length = snprintf(line, sizeof(line),
      "[%s][%c] core=%u seq=%lu err=%ld %%", "hello", 'I', 1u,
      (unsigned long)UINT32_MAX, (long)INT32_MIN);
  if (length != (int)strlen(expected) || strcmp(line, expected) != 0) return 1;
  if (snprintf(small, sizeof(small), "%s", "abcdef") != 6 ||
      strcmp(small, "abcd") != 0) return 2;
  if (snprintf(single, sizeof(single), "%u", 123u) != 3 || single[0] != '\0')
    return 3;
  if (snprintf(NULL, 0u, "%s:%u", "x", 7u) != 3) return 4;
  if (format_variadic(line, sizeof(line), "%s %lu %ld", "values",
                      (unsigned long)UINT32_MAX, (long)-1) != 20 ||
      strcmp(line, "values 4294967295 -1") != 0) return 5;
  return 0;
}
