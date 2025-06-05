// tdtimer-mac.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include "../td-usb.h"
#include "../tdtimer.h"

static pthread_t timer_thread;
static int (*timer_callback)(td_context_t *) = NULL;
static td_context_t *timer_param = NULL;
static int timer_interval = 0;
static int timer_running = 0;

static void *timer_worker(void *arg)
{
    while (timer_running) {
        usleep(timer_interval * 1000); // interval is in milliseconds
        if (timer_running && timer_callback) {
            timer_callback(timer_param);
        }
    }
    return NULL;
}

int TdTimer_Start(int (*pCallback)(td_context_t *), td_context_t *pParam, int interval)
{
    if (timer_running) {
        TdTimer_Stop();
    }
    
    timer_callback = pCallback;
    timer_param = pParam;
    timer_interval = interval;
    timer_running = 1;
    
    if (pthread_create(&timer_thread, NULL, timer_worker, NULL) != 0) {
        timer_running = 0;
        return -1;
    }
    
    return 0;
}

int TdTimer_Stop()
{
    if (timer_running) {
        timer_running = 0;
        pthread_join(timer_thread, NULL);
    }
    return 0;
} 