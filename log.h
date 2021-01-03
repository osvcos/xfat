#ifndef LOG_H
#define LOG_H

#ifndef DEBUG
#define LOG(...)
#else
#define LOG(...) \
    printf(__VA_ARGS__)
#endif

#endif
