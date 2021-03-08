
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <errno.h>

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "td-usb.h"
#include "tdtimer.h"

timer_t timerid = 0;

static void(*pHandler)(void *);
static void *pUserParam;


static void timer_handler(int sig, siginfo_t *si, void *uc)
{
	pHandler(pUserParam);
}

int _nanosleep(int sec, int nsec)
{
	struct timespec req, rem;

	req.tv_sec = sec;
	req.tv_nsec = nsec;
	rem.tv_sec = 0;
	rem.tv_nsec = 0;
	while (nanosleep(&req, &rem)) {
		if (errno == EINTR) {
			req.tv_sec = rem.tv_sec;
			req.tv_nsec = rem.tv_nsec;
		}
		else {
			perror("nanosleep error");
			return -1;
		}
	}
	return 0;
}


int TdTimer_Start(void pCallback(void *), void *pParam, int Interval)
{
	long long freq_nanosecs;
	struct sigaction sa;
	struct sigevent sev;
	struct itimerspec its;
	
	pHandler = pCallback;
	pUserParam = pParam;

	/* Establish handler for timer signal */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1) throw_exception(21, "sigaction");

	/* Create the timer */
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGRTMIN;	
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) throw_exception(54, "timer_create");

	/* Start the timer */
	freq_nanosecs = Interval * (1000 * 1000);
	its.it_value.tv_sec = freq_nanosecs / 1000000000;
	its.it_value.tv_nsec = freq_nanosecs % 1000000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timer_settime(timerid, 0, &its, NULL) == -1) throw_exception(55, "timer_settime");

	while (1) 
	{		
		_nanosleep(1, 0);    /* sleep 1 sec */
	}
}
