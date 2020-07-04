#ifndef __CPP_TIMER_H_
#define __CPP_TIMER_H_

/**
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 *
 * (C) 2020, Bernd Porr <mail@bernporr.me.uk>
 * 
 * This is inspired by the timer_create man page.
 **/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <functional>
#include <atomic>

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

/**
 * Timer class which repeatedly fires. It's wrapper around the
 * POSIX per-process timer.
 **/
class CppTimer {

public:
	static constexpr int PERIODIC = 0;
	static constexpr int ONESHOT  = 1;
	static constexpr long MS_TO_NS  = 1000000;
	/**
	 * Creates an instance of the timer and connects the
	 * signal handler to the timer.
	 **/
	CppTimer();

	/**
	 * Starts the timer. The timer fires first after
	 * the specified time in nanoseconds and then at
	 * that interval in PERIODIC mode. In ONESHOT mode
	 * the timer fires once after the specified time in
	 * nanoseconds.
	 **/
	virtual void start(long secs, long nanosecs, std::function<void()> callbackIn, int type = PERIODIC); 

	/**
	* Stops the timer by disarming it. It can be re-started
	* with start().
	**/
	virtual void stop();

	/**
	 * Destructor disarms the timer, deletes it and
	 * disconnect the signal handler.
	 **/
	virtual ~CppTimer();
	
	bool isRunning();
	
	void block();

private:
	timer_t timerid = 0;
	struct sigevent sev;
	struct sigaction sa;
	struct itimerspec its;
	int pipeFd[2];
	std::atomic<bool> running = false;
	std::function<void()> callback;
	void discardPipe();
		
	static void handler(int sig, siginfo_t *si, void *uc ) {
		CppTimer *timer = reinterpret_cast<CppTimer *> (si->si_value.sival_ptr);
		timer->callback();
		char buf = '\n'; 
		write(timer->pipeFd[1], &buf, 1);
		timer->running = false;
	}
};


#endif
