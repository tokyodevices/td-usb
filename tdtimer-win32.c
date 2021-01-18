// tdtimer-win32.c

#include <SDKDDKVer.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "tdtimer.h"

#pragma comment (lib, "winmm.lib")

static void(*pHandler)(void);

static void CALLBACK MyTimerCallback(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	pHandler();
}

void TdTimer_Init(void)
{

}

int TdTimer_Start(void pCallback(void), int Interval)
{
	pHandler = pCallback;
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	timeSetEvent(Interval, 10, (LPTIMECALLBACK)MyTimerCallback, 0, TIME_PERIODIC);
	WaitForSingleObject(hEvent, INFINITE);
	return 0;
}
