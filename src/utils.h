#define min(x, y) (x) < (y) ? (x) : (y)
#define max(x, y) (x) > (y) ? (x) : (y)
#define clamp(v, l, u) min(max(v, l), u)
#ifdef DEBUG
int debugging = 1;
#else
int debugging = 0;
#endif
#define IF_DEBUGGING(expression) \
    if (debugging) expression;
