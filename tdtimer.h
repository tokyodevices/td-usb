// tdtimer.h
#pragma once

#include <stdint.h>

// Monotonic clock for elapsed-time deadlines.
uint64_t TdTimer_GetMilliseconds(void);

int TdTimer_Start(void pCallback(void *), void *pParam, int interval);
