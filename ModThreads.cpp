#include "ModThreads.h"

#include <thread>

uint8_t getThreadCount()
{
	uint8_t threadCount = std::thread::hardware_concurrency();

	if (threadCount > 4) threadCount -= 4;
	else threadCount = 1;

	return threadCount;
}
