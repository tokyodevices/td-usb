/* Check timer failures and cleanup without waiting or touching USB hardware. */
#include <assert.h>
#include <windows.h>
#include <stdio.h>

static int fail_event, fail_timer, creates, starts, waits, stops, closes, ticks;
static DWORD wait_result;
static int parameter;
static HANDLE event_handle;

static HANDLE WINAPI test_CreateEvent(LPSECURITY_ATTRIBUTES attributes,
    BOOL manual_reset, BOOL initial_state, LPCTSTR name)
{
    assert(!attributes && !manual_reset && !initial_state && !name);
    creates++;
    return fail_event ? NULL : event_handle;
}
static MMRESULT WINAPI test_timeSetEvent(UINT delay, UINT resolution,
    LPTIMECALLBACK callback, DWORD_PTR user, UINT flags)
{
    assert(delay == 50 && resolution == 10 && user == 0);
    assert(flags == (TIME_PERIODIC | TIME_KILL_SYNCHRONOUS));
    starts++;
    if (fail_timer) return 0;
    callback(7, 0, user, 0, 0);
    return 7;
}
static DWORD WINAPI test_WaitForSingleObject(HANDLE event, DWORD timeout)
{
    assert(event == event_handle && timeout == INFINITE);
    waits++;
    return wait_result;
}
static MMRESULT WINAPI test_timeKillEvent(UINT timer)
{
    assert(timer == 7 && closes == 0);
    stops++;
    return TIMERR_NOERROR;
}
static BOOL WINAPI test_CloseHandle(HANDLE event)
{
    assert(event == event_handle);
    closes++;
    return TRUE;
}
#undef CreateEvent
#define CreateEvent test_CreateEvent
#define timeSetEvent test_timeSetEvent
#define WaitForSingleObject test_WaitForSingleObject
#define timeKillEvent test_timeKillEvent
#define CloseHandle test_CloseHandle
#include "../windows/tdtimer-windows.c"
#undef CloseHandle
#undef timeKillEvent
#undef WaitForSingleObject
#undef timeSetEvent
#undef CreateEvent

static void tick(void *argument) { assert(argument == &parameter); ticks++; }
static void reset(void)
{
    fail_event = fail_timer = creates = starts = waits = stops = closes = ticks = 0;
    event_handle = &parameter;
    wait_result = WAIT_OBJECT_0;
}
int main(void)
{
    reset(); fail_event = 1;
    assert(TdTimer_Start(tick, &parameter, 50) != 0);
    assert(creates == 1 && starts == 0 && waits == 0 && stops == 0 && closes == 0);
    reset(); fail_timer = 1;
    assert(TdTimer_Start(tick, &parameter, 50) != 0);
    assert(starts == 1 && waits == 0 && stops == 0 && closes == 1 && ticks == 0);
    reset(); wait_result = WAIT_FAILED;
    assert(TdTimer_Start(tick, &parameter, 50) != 0);
    assert(waits == 1 && stops == 1 && closes == 1 && ticks == 1);
    reset(); wait_result = WAIT_TIMEOUT;
    assert(TdTimer_Start(tick, &parameter, 50) != 0);
    assert(waits == 1 && stops == 1 && closes == 1);
    reset();
    assert(TdTimer_Start(tick, &parameter, 50) == 0);
    assert(creates == 1 && starts == 1 && waits == 1 && stops == 1 && closes == 1 && ticks == 1);
    puts("PASS: Windows timer creation/wait failures and cleanup (5 scenarios)");
    return 0;
}
