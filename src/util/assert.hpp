#pragma once

#include <cstdio>
#include <cstdlib>

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)
