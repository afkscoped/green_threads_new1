#pragma once

#include <cstdio>

#define LOG(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
