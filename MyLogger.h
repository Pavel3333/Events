#pragma once

#define debug_log                true
#define extended_debug_log       true
#define super_extended_debug_log true

#define trace_log false

#define INIT_LOCAL_MSG_BUFFER \
	static char __log_buf_private[1024]


void __log_private(char* buf, const char* fmt, ...);
void __log_private_with_pystdout(char* buf, const char* fmt, ...);


#define debugLog(fmt, ...) debugLogEx(INFO, fmt, ##__VA_ARGS__)
#define extendedDebugLog(fmt, ...) extendedDebugLogEx(INFO, fmt, ##__VA_ARGS__)
#define superExtendedDebugLog(fmt, ...) superExtendedDebugLogEx(INFO, fmt, ##__VA_ARGS__)


#if debug_log
#define debugLogEx(level, fmt, ...) \
	__log_private_with_pystdout(__log_buf_private, "[NY_Event][" #level "]: " fmt "\n", ##__VA_ARGS__)


#if extended_debug_log
#define extendedDebugLogEx(level, fmt, ...) \
	__log_private(__log_buf_private, "[NY_Event][" #level "]: " fmt "\n", ##__VA_ARGS__)


#if super_extended_debug_log
#define superExtendedDebugLogEx(level, fmt, ...) \
	__log_private(__log_buf_private, "[NY_Event][" #level "]: " fmt "\n", ##__VA_ARGS__)


#else
#define superExtendedDebugLogEx(...) ((void)0)
#endif
#else
#define extendedDebugLogEx(...)      ((void)0)
#define superExtendedDebugLogEx(...) ((void)0)
#endif
#else
#define debugLogEx(...)              ((void)0)
#define extendedDebugLogEx(...)      ((void)0)
#define superExtendedDebugLogEx(...) ((void)0)
#endif

#if trace_log
#define traceLog \
	dbg_log << __LINE__ << " - " __FUNCTION__ << std::endl;
#else
#define traceLog ((void)0);
#endif
