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

