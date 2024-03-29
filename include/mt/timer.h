/**
 * timer.h : Defines the helper to create timer
 *
 * Author: Anton (ud) Golovkov, udattsk@gmail.com
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * Official repository: https://github.com/ud84/mt
 */

#pragma once

#ifndef _WIN32
#include <chrono>
#include <thread>
#else
#include <windows.h>
#endif

#include <atomic>
#include <functional>
#include <exception>
#include <cstdint>

namespace mt
{

#ifndef _WIN32
class timer
{
public:
	explicit timer(std::function<void(void)> callback_)
		: runned(false), thread(), callback(callback_), interval(1000)
	{
	}

	~timer()
	{
		stop();
	}

	void start(const uint32_t interval_ = 1000 /* in milliseconds */)
	{
		if (!runned)
		{
			interval = interval_;
			runned = true;

			thread = std::thread(&timer::run, this);
		}
	}

	void stop()
	{
		if (runned)
		{
			runned = false;
			if (thread.joinable()) thread.join();
		}
	}
	
	timer(const timer&) = delete;
	timer& operator=(const timer&) = delete;

private:
	std::atomic<bool> runned;
	std::thread thread;
	std::function<void(void)> callback;

	uint32_t interval;

	void run()
	{
		while (runned)
		{
			auto begin = std::chrono::high_resolution_clock::now();

			callback();
						
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin).count();
			std::this_thread::sleep_for(std::chrono::microseconds((interval * 1000) - elapsed));
		}
	}
};
#else
class timer
{
public:
	explicit timer(std::function<void(void)> callback_)
		: callback(callback_), hTimer(NULL), hTimerQueue(NULL), runned(false)
	{
	}

	~timer()
	{
		stop();
	}

	void start(const uint32_t interval = 1000)
	{
		if (runned || hTimerQueue != NULL)
		{
			return;
		}
		
		hTimerQueue = CreateTimerQueue();
		if (hTimerQueue == NULL)
		{
			throw std::exception("Error CreateTimerQueue()");
		}

		if (!CreateTimerQueueTimer(&hTimer, hTimerQueue,
			(WAITORTIMERCALLBACK)TimerRoutine, this, 0, interval, WT_EXECUTEDEFAULT))
		{
			throw std::exception("Error CreateTimerQueueTimer()");
		}
		runned = true;
	}

	void stop()
	{
		if (!runned)
		{
			return;
		}
				
		if (hTimer != NULL && hTimerQueue != NULL)
		{
			DeleteTimerQueueTimer(hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
			hTimer = NULL;
		}
		if (hTimerQueue != NULL)
		{
			DeleteTimerQueue(hTimerQueue);
			hTimerQueue = NULL;
		}
		runned = false;
	}

	timer(const timer&) = delete;
	timer& operator=(const timer&) = delete;

private:
	std::function<void(void)> callback;

	HANDLE hTimer;
    HANDLE hTimerQueue;

	std::atomic<bool> runned;
	
	static VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN)
	{
		static_cast<timer*>(lpParam)->callback();
	}
};
#endif

}
