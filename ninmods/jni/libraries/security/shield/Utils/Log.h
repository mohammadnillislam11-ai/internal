#include <cstdio>
#include <unistd.h>

#include <android/log.h>
#include <string>

#ifdef DEBUG_BUILD
#define LOGI(...) ((void)0) // Disable this on production
#else
#define LOGI(...)
#endif
