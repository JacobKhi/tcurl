#pragma once

/* cJSON is packaged with different include layouts across distros/toolchains.
   Support both common forms without requiring source changes per platform. */
#if defined(__has_include)
#if __has_include(<cjson/cJSON.h>)
#include <cjson/cJSON.h>
#elif __has_include(<cJSON.h>)
#include <cJSON.h>
#else
#error "cJSON header not found. Install libcjson/cjson development package."
#endif
#else
#include <cjson/cJSON.h>
#endif

