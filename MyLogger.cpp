#include "MyLogger.h"
#include <fstream>
#include <chrono>
#include <ctime>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "python2.7/Python.h"



static std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);


void __my_log(const char* str)
{
	OutputDebugStringA(str);
	dbg_log << str << std::flush;
}


size_t __my_log_fmt(char* buf, const char* fmt, ...)
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

	__my_log(buf);

	return len;
}

size_t __my_log_fmt(char* buf, const char* fmt, va_list args)
{
	using namespace std::chrono;
	const std::time_t t = system_clock::to_time_t(system_clock::now());
	std::tm tm;
	localtime_s(&tm, &t);
	size_t len = std::strftime(buf, 1024, "%H:%M:%S: ", &tm);

	vsprintf_s(buf + len, 1024 - len, fmt, args);

	__my_log(buf);

	return len;
}

void __my_log_fmt_with_pystdout(char* buf, const char* fmt, ...)
{
	size_t len;

	va_list args;
	va_start(args, fmt);
	len = __my_log_fmt(buf, fmt, args);
	va_end(args);

	PySys_WriteStdout(buf + len);
}
