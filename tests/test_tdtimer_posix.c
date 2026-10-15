#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../tdtimer.h"

static jmp_buf timer_started;
static const struct {
    int milliseconds;
    time_t seconds;
    long nanoseconds;
} intervals[] = {
    { 50, 0, 50000000 },
    { 999, 0, 999000000 },
    { 1000, 1, 0 },
    { 2147, 2, 147000000 },
    { 2148, 2, 148000000 },
    { 3000, 3, 0 },
    { 4294, 4, 294000000 },
    { 4295, 4, 295000000 },
    { 5000, 5, 0 },
    { 5001, 5, 1000000 },
    { 60000, 60, 0 },
    { INT_MAX, 2147483, 647000000 }
};
static unsigned int current_interval;

void throw_exception(int exitcode, const char *message)
{
    fprintf(stderr, "Unexpected timer error %d: %s\n", exitcode, message);
    abort();
}

int __wrap_timer_settime(timer_t timerid, int flags,
                        const struct itimerspec *value,
                        struct itimerspec *old_value)
{
    assert(flags == 0 && old_value == NULL);
    assert(value->it_value.tv_sec == intervals[current_interval].seconds);
    assert(value->it_value.tv_nsec == intervals[current_interval].nanoseconds);
    assert(value->it_interval.tv_sec == value->it_value.tv_sec);
    assert(value->it_interval.tv_nsec == value->it_value.tv_nsec);
    assert(timer_delete(timerid) == 0);
    /* Inspect the timer request without waiting for long intervals. */
    longjmp(timer_started, 1);
}

static void unexpected_tick(void *param)
{
    (void)param;
    abort();
}

int main(void)
{
    for (current_interval = 0;
         current_interval < sizeof(intervals) / sizeof(intervals[0]);
         current_interval++) {
        if (setjmp(timer_started) == 0) {
            TdTimer_Start(unexpected_tick, NULL, intervals[current_interval].milliseconds);
            abort();
        }
    }
    puts("PASS: Linux timer intervals, overflow boundaries, 3/5 seconds and INT_MAX");
    return 0;
}
