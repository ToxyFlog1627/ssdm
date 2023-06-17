#ifndef UTILS_H
#define UTILS_H

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(v, l, u) min(max(v, l), u)

#endif  // UTILS_H