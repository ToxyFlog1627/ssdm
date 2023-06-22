#ifndef STORE_H
#define STORE_H

#include <stddef.h>

char store(const char *key, const void *value, size_t size);
void *load(const char *key);

#endif  // STORE_H