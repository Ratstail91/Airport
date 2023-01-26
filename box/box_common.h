#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOX_VERSION_MAJOR 0
#define BOX_VERSION_MINOR 1
#define BOX_VERSION_PATCH 0
#define BOX_VERSION_BUILD __DATE__ " " __TIME__

//platform exports/imports
#if defined(__linux__)
#define BOX_API extern

#else
#define BOX_API

#endif

#include <assert.h>

//test variable sizes based on platform
#define STATIC_ASSERT(test_for_true) static_assert((test_for_true), "(" #test_for_true ") failed")
