#pragma once

#include "cpp_magic.h"

#include <unistd.h>

#define PACKED __attribute__((packed))

#define ARRAY_LEN(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define SAFE_CLOSE(fd) \
    do { \
        if (fd < 0) { \
            close(fd); \
            fd = -1; \
        } \
    } while (0)

#define SAFE_FREE(ptr) \
    do { \
        if (ptr != NULL) { \
            free(ptr); \
            ptr = NULL; \
        } \
    } while (0)
