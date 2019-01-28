#pragma once
#include <Windows.h>

#define debug_log true
#define extended_debug_log true
#define super_extended_debug_log false


#if debug_log
#	define LOG_debug(X) OutputDebugString(TEXT(X))
#else
#	define LOG_debug(X) 0
#endif

#if extended_debug_log
#	define LOG_extended_debug(X) LOG_debug(X)
#else
#	define LOG_extended_debug(X) 0
#endif

#if super_extended_debug_log
#	define LOG_super_extended_debug(X) LOG_debug(X)
#else
#	define LOG_super_extended_debug(X) 0
#endif

