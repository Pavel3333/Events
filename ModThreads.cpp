#include "pch.h"
#include "ModThreads.h"


uint8_t getThreadCount()
{
	uint8_t threadCount = static_cast<uint8_t>(std::thread::hardware_concurrency());

	if (threadCount > 4) threadCount -= 4;
	else threadCount = 1;

	return threadCount;
}
