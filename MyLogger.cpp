﻿#include "MyLogger.h"
#include <fstream>
#include <chrono>
#include <ctime>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "python2.7/Python.h"
#include <cstdarg>



static std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);

static size_t getTime(char* buf, uint16_t size) {
	using namespace std::chrono;
	const std::time_t t = system_clock::to_time_t(system_clock::now());
	std::tm tm;
	localtime_s(&tm, &t);

	return std::strftime(buf, size, "%H:%M:%S: ", &tm);
}

static size_t timedFmt(char* buf, const char* fmt, va_list args) {
	size_t len = getTime(buf, MAX_DBG_LINE_SIZE);

	vsprintf_s(buf + len, MAX_DBG_LINE_SIZE - len, fmt, args);

	return len;
}

void __my_log_write_data_to_file(char* name, char* data, size_t size) 
{
	char time[64];
	char filename[MAX_PATH];

	getTime(time, MAX_DBG_TIME_SIZE);
	
	sprintf_s(filename, MAX_PATH, "NY_Event_debug_data_%s_%s.txt", name, time);

	std::ofstream dbg_file(filename, std::ios::binary);

	dbg_file.write(data, size);

	dbg_file.close();
}

void __my_log(const char* str)
{
	OutputDebugStringA(str);
	dbg_log << str << std::flush;
}

void __my_log_fmt(char* buf, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	timedFmt(buf, fmt, args);
	va_end(args);

	__my_log(buf);
}

void __my_log_fmt_with_pystdout(char* buf, const char* fmt, ...)
{
	size_t len;

	va_list args;
	va_start(args, fmt);
	len = timedFmt(buf, fmt, args);
	va_end(args);

	__my_log(buf);

	PySys_WriteStdout(buf + len);
}
