// tdtimer-windows.c

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "..\tdtimer.h"

#pragma comment (lib, "winmm.lib")

uint64_t TdTimer_GetMilliseconds(void)
{
	return GetTickCount64();
}

static void(*pUserHandler)(void *);
static void *pUserParam;

static void CALLBACK MyTimerCallback(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	pUserHandler(pUserParam);
}

int TdTimer_Start(void pCallback(void *), void *pParam, int Interval)
{
	pUserHandler = pCallback;
	pUserParam = pParam;

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) return -1;
	MMRESULT timer = timeSetEvent(Interval, 10, MyTimerCallback, 0, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
	if (timer == 0)
	{
		CloseHandle(hEvent);
		return -1;
	}
	DWORD result = WaitForSingleObject(hEvent, INFINITE);
	timeKillEvent(timer);
	CloseHandle(hEvent);
	return result == WAIT_OBJECT_0 ? 0 : -1;
}
