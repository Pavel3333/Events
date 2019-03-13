#pragma once

#define debug_log                true
#define extended_debug_log       true
#define super_extended_debug_log false

#define trace_log false

#define MAX_DBG_LINE_SIZE 1024
#define MAX_DBG_TIME_SIZE 64

#define INIT_LOCAL_MSG_BUFFER \
	static char __log_buf_private[MAX_DBG_LINE_SIZE]

void __my_log(const char*);
void __my_log_fmt(char*, const char*, ...);
void __my_log_fmt_with_pystdout(char*, const char*, ...);

void __my_log_write_data_to_file(char*, char*, char*, size_t);


#define debugLog(fmt, ...) debugLogEx(INFO, fmt, ##__VA_ARGS__)
#define extendedDebugLog(fmt, ...) extendedDebugLogEx(INFO, fmt, ##__VA_ARGS__)
#define superExtendedDebugLog(fmt, ...) superExtendedDebugLogEx(INFO, fmt, ##__VA_ARGS__)


#if debug_log
#define debugLogEx(level, fmt, ...) \
	__my_log_fmt_with_pystdout(__log_buf_private, "[Events][" #level "]: " fmt "\n", ##__VA_ARGS__)


#if extended_debug_log
#define extendedDebugLogEx(level, fmt, ...) \
	__my_log_fmt(__log_buf_private, "[Events][" #level "]: " fmt "\n", ##__VA_ARGS__)
#define writeDebugDataToFile(mod_name, name, data, size) __my_log_write_data_to_file(mod_name, #name, data, size)

#if super_extended_debug_log
#define superExtendedDebugLogEx(level, fmt, ...) \
	__my_log_fmt(__log_buf_private, "[Events][" #level "]: " fmt "\n", ##__VA_ARGS__)


#else
#define superExtendedDebugLogEx(...) ((void)0)
#endif
#else
#define writeDebugDataToFile(...)    ((void)0)
#define extendedDebugLogEx(...)      ((void)0)
#define superExtendedDebugLogEx(...) ((void)0)
#endif
#else
#define debugLogEx(...)              ((void)0)
#define writeDebugDataToFile(...)    ((void)0)
#define extendedDebugLogEx(...)      ((void)0)
#define superExtendedDebugLogEx(...) ((void)0)
#endif

#if trace_log
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define traceLog \
	__my_log(TOSTRING(__LINE__) " - " __FUNCTION__ "\n");
#else
#define traceLog ((void)0);
#endif
