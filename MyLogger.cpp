#include "MyLogger.h"
#include <fstream>
#include <chrono>
#include <ctime>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "python2.7/Python.h"



static std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);


void __my_log(const char* str, bool writeToStdout)
{
	OutputDebugStringA(str);
	dbg_log << str << std::flush;

	if(writeToStdout) PySys_WriteStdout(str);
}


void __my_log_fmt(bool writeToStdout, char* buf, const char* fmt, ...)
{
	using namespace std::chrono;
	const std::time_t t = system_clock::to_time_t(system_clock::now());
	std::tm tm;
	localtime_s(&tm, &t);
	size_t len = std::strftime(buf, 1024, "%H:%M:%S: ", &tm);

	va_list args;
	va_start(args, fmt);
	vsprintf_s(buf + len, 1024 - len, fmt, args);
	va_end(args);

	__my_log(buf, writeToStdout);
}