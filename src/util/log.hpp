#pragma once

#include <cstdio>

#define LOG(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG_INFO(fmt, ...) LOG("INFO: " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG("DEBUG: " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG("ERROR: " fmt, ##__VA_ARGS__)
