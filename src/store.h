#ifndef STORE_H
#define STORE_H

#include <stddef.h>

void *load(const char *key);
char store(const char *key, const void *value, size_t size);

#endif  // STORE_H