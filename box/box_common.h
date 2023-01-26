#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//platform exports/imports
#if defined(__linux__)
#define BOX_API extern

#else
#define BOX_API

#endif

#include <assert.h>

//test variable sizes based on platform
#define STATIC_ASSERT(test_for_true) static_assert((test_for_true), "(" #test_for_true ") failed")
