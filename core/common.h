#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//platform exports/imports
#if defined(__linux__)
#define CORE_API extern
#else
#define CORE_API
#endif

#include <assert.h>

//test variable sizes based on platform
#define STATIC_ASSERT(test_for_true) static_assert((test_for_true), "(" #test_for_true ") failed")
