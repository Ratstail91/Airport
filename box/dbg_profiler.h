#pragma once

#include <time.h>

void Dbg_clearConsole();

typedef struct Dbg_Timer {
	const char* name;
	clock_t start;
	char log[2048];
	int logPos;
} Dbg_Timer;

void Dbg_initTimer(Dbg_Timer*);
void Dbg_startTimer(Dbg_Timer*, const char* name);
void Dbg_stopTimer(Dbg_Timer*);
void Dbg_printTimerLog(Dbg_Timer*);
void Dbg_freeTimer(Dbg_Timer*);

typedef struct Dbg_FPSCounter {
	int count;
	clock_t start;
	char log[256];
} Dbg_FPSCounter;

void Dbg_initFPSCounter(Dbg_FPSCounter*);
void Dbg_tickFPSCounter(Dbg_FPSCounter*);
void Dbg_printFPSCounter(Dbg_FPSCounter*);
void Dbg_freeFPSCounter(Dbg_FPSCounter*);
