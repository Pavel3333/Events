#include "pch.h"
#include "MyLogger.h"
#include <ctime>

static std::ofstream dbg_log(MOD_DBG_FILE, std::ios::app);

static size_t getTime(char* buf, const char* fmt, uint16_t size) {
	using namespace std::chrono;
	const std::time_t t = system_clock::to_time_t(system_clock::now());
	std::tm tm;
	localtime_s(&tm, &t);

	return std::strftime(buf, size, fmt, &tm);
}

static size_t timedFmt(char* buf, const char* fmt, va_list args) {
	size_t len = getTime(buf, "%H:%M:%S: ", MAX_DBG_LINE_SIZE);

	vsprintf_s(buf + len, MAX_DBG_LINE_SIZE - len, fmt, args);

	return len;
}

void __my_log_write_data_to_file(char* name, char* data, size_t size) 
{
	char time[64];
	char filename[MAX_PATH];

	getTime(time, "%F_%H_%M_%S", MAX_DBG_TIME_SIZE);
	
	sprintf_s(filename, MAX_PATH, "%s_debug_data_%s_%s.txt", MOD_NAME, name, time);

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
