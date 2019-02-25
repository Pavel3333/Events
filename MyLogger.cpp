#include "MyLogger.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <Windows.h>
#include "python2.7/Python.h"


static std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);


void __log_private(char* buf, const char* fmt, ...)
{
	using namespace std::chrono;
	std::time_t t = system_clock::to_time_t(system_clock::now());
	size_t len = std::strftime(buf, 1024, "%H:%M:%S: ", std::localtime(&t));

	va_list args;
	va_start(args, fmt);
	sprintf_s(buf + len, 1024 - len, fmt, args);
	va_end(args);

	OutputDebugStringA(buf);
	dbg_log << buf << std::flush;
}

void __log_private_with_pystdout(char* buf, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	__log_private(buf, fmt, args);
	PySys_WriteStdout(fmt, args);
	va_end(args);
}
