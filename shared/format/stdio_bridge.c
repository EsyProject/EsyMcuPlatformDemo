/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "printf.h"
#undef printf
#undef sprintf
#undef snprintf
#undef vsnprintf
#undef vprintf

/* The final firmware links these strong buffer-formatting symbols before libc.
 * No console output function is supplied: unreferenced provider console
 * sections are discarded by --gc-sections. */
int vsnprintf(char *buffer, size_t count, const char *format, va_list arguments) {
  return vsnprintf_(buffer, count, format, arguments);
}

int snprintf(char *buffer, size_t count, const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf_(buffer, count, format, arguments);
  va_end(arguments);
  return result;
}
