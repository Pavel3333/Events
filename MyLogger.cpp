#include "MyLogger.h"
#include <Windows.h>
#include <fstream>
#include "python2.7/Python.h"


static std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);


void __log_private(char* buf, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	sprintf_s(buf, 1024, fmt, args);
	OutputDebugStringA(buf);
	dbg_log << buf << std::flush;
	va_end(args);
}

void __log_private_with_pystdout(char* buf, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	__log_private(buf, fmt, args);
	PySys_WriteStdout(fmt, args);
	va_end(args);
}
