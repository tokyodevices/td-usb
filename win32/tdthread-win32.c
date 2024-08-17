// tdthread-win32.c

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\td-usb.h"
#include "..\tdthread.h"

static void (*UserCallback)(void *);
static void *pUserParam;

static DWORD WINAPI TdThread_Proc( _In_ LPVOID lpParameter )
{
	UserCallback(pUserParam);
	return 0;
}

int TdThread_Start(void pCallback(void *), void *pParam)
{
	DWORD threadId;

	UserCallback = pCallback;
	pUserParam = pParam;

	HANDLE hThread = CreateThread(
		NULL,
		0,
		TdThread_Proc,
		NULL,
		0,
		&threadId
	);

	printf("CreateThread threadId=#%d\n", threadId);
	
	return 0;
}