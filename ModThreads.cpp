#include "stdafx.h"

#include "ModThreads.h"

#include <vector>
#include <thread>

std::vector<Thread_1> threads_1;

uint8_t getThreadCount()
{
	uint8_t threadCount = std::thread::hardware_concurrency();

	if (threadCount > 4) threadCount -= 4;
	else threadCount = 1;

	return threadCount;
}