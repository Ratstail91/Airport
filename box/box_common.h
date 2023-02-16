#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOX_VERSION_MAJOR 0
#define BOX_VERSION_MINOR 1
#define BOX_VERSION_PATCH 0
#define BOX_VERSION_BUILD __DATE__ " " __TIME__

//platform/compiler-specific instructions
#if defined(__linux__) || defined(__MINGW32__) || defined(__GNUC__)

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <unistd.h>
#define BOX_API extern

#elif defined(_MSC_VER)

#include <SDL.h>
#include <SDL_image.h>

#include <windows.h>
#ifndef BOX_EXPORT
#define BOX_API __declspec(dllimport)
#else
#define BOX_API __declspec(dllexport)
#endif

#define sleep Sleep

#else

#define BOX_API extern

#endif

#include <assert.h>

//test variable sizes based on platform
#define STATIC_ASSERT(test_for_true) static_assert((test_for_true), "(" #test_for_true ") failed")
