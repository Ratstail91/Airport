#include "dbg_profiler.h"

#include "toy_memory.h"

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//https://stackoverflow.com/questions/2347770/how-do-you-clear-the-console-screen-in-c
void Dbg_clearConsole()
{
	HANDLE					 hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD					  count;
	DWORD					  cellCount;
	COORD					  homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR)' ',
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Move the cursor home */
	SetConsoleCursorPosition(hStdOut, homeCoords);
}

#else // !_WIN32
#include <unistd.h>
#include <term.h>

void Dbg_clearConsole()
{
	if (!cur_term)
	{
		int result;
		setupterm(NULL, STDOUT_FILENO, &result);
		if (result <= 0) return;
	}

	putp(tigetstr("clear"));
}
#endif

void Dbg_initTimer(Dbg_Timer* timer) {
	timer->name = NULL;
	timer->start = 0;
	timer->log = TOY_ALLOCATE(char, 2048);
	memset(timer->log, 0, 2048);
	timer->logPos = 0;
}

void Dbg_startTimer(Dbg_Timer* timer, const char* name) {
	timer->name = name;
	timer->start = clock();
}

void Dbg_stopTimer(Dbg_Timer* timer) {
	double duration = (clock() - timer->start) / (double) CLOCKS_PER_SEC * 1000;
	timer->logPos += sprintf(&(timer->log[timer->logPos]), "%.3fms %s\n", duration, timer->name);
}

void Dbg_printTimerLog(Dbg_Timer* timer) {
	printf("%.*s", timer->logPos, timer->log);
	timer->logPos = 0;
}

void Dbg_freeTimer(Dbg_Timer* timer) {
	TOY_FREE_ARRAY(char, timer->log, 2048);
}

void Dbg_initFPSCounter(Dbg_FPSCounter* counter) {
	counter->count = 0;
	counter->start = clock();
	counter->log = TOY_ALLOCATE(char, 256);
	memset(counter->log, 0, 256);
}

void Dbg_tickFPSCounter(Dbg_FPSCounter* counter) {
	counter->count++;

	if (clock() - counter->start > CLOCKS_PER_SEC) {
		sprintf(counter->log, "%d FPS", counter->count);
		counter->start = clock();
		counter->count = 0;
	}
}

void Dbg_printFPSCounter(Dbg_FPSCounter* counter) {
	printf("%s\n", counter->log);
}

void Dbg_freeFPSCounter(Dbg_FPSCounter* counter) {
	TOY_FREE_ARRAY(char, counter->log, 256);
}