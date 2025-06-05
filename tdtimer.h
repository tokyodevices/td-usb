// tdtimer.h
#pragma once

// Forward declaration
typedef struct td_context_t td_context_t;

int TdTimer_Start(int (*pCallback)(td_context_t *), td_context_t *pParam, int interval);
int TdTimer_Stop();
