#include "CppTimer.h"
#include <fcntl.h>

/**
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 *
 * (C) 2020, Bernd Porr <mail@bernporr.me.uk>
 * 
 * This is inspired by the timer_create man page.
 **/

CppTimer::CppTimer() {
	// We create a static handler catches the signal SIG
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG, &sa, NULL) == -1)
		throw("Could not create signal handler");
	
	if(pipe(pipeFd) < 0)
		throw("Could not create pipe");
	
	// Create the timer
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	// Cruical is that the signal carries the pointer to this class instance here
	// because the handler just handles anything that comes in!
	sev.sigev_value.sival_ptr = this;
	// create the timer
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		throw("Could not create timer");
};

void CppTimer::start(long secs, long nanosecs,std::function<void()> callbackIn, int type) {
	switch(type){
		case(PERIODIC):
			//starts after specified period of nanoseconds
			its.it_value.tv_sec = secs;
			its.it_value.tv_nsec = nanosecs;
			its.it_interval.tv_sec = secs;
			its.it_interval.tv_nsec = nanosecs;
			break;
		case(ONESHOT):
			//fires once after specified period of nanoseconds
			its.it_value.tv_sec = secs;
			its.it_value.tv_nsec = nanosecs;
			its.it_interval.tv_sec = 0;
			its.it_interval.tv_nsec = 0;
			break;
	}
	callback = callbackIn;
	if (timer_settime(timerid, 0, &its, NULL) == -1)
		throw("Could not start timer");
	discardPipe();
	running = true;
}

void CppTimer::discardPipe() {
	char buf;
	fcntl(pipeFd[0], F_SETFL, O_NONBLOCK);
	while(read(pipeFd[0], &buf, 1) > 0);
	fcntl(pipeFd[0], F_SETFL, 0);
}

void CppTimer::block()
{
	if(running)
	{
		char buf;
		read(pipeFd[0], &buf, 1);
	}
}

void CppTimer::stop() {
	// disarm
	struct itimerspec itsnew;
	itsnew.it_value.tv_sec = 0;
	itsnew.it_value.tv_nsec = 0;
	itsnew.it_interval.tv_sec = 0;
	itsnew.it_interval.tv_nsec = 0;
	timer_settime(timerid, 0, &itsnew, &its);
	running = false;
}

bool CppTimer::isRunning()
{
	return running;
}

CppTimer::~CppTimer() {
	stop();
	// delete the timer
	timer_delete(timerid);
	// default action for signal handling
	signal(SIG, SIG_IGN);
}
