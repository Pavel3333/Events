#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>
#include <tchar.h>

#define debug_log                true
#define extended_debug_log       false
#define super_extended_debug_log false

#define trace_log false

#define INIT_LOCAL_MSG_BUFFER \
	char log_buf_c[512];      \
	WCHAR log_buf[512];                          

#if debug_log
#define debugLog(X)           \
	OutputDebugString(_T(X)); \
	PySys_WriteStdout(X);     \
    dbg_log << X;

#define debugLogFmt(fmt, ...) {                  \
	wsprintfW(log_buf, _T(fmt), __VA_ARGS__);    \
	OutputDebugString(log_buf);                  \
	PySys_WriteStdout(fmt, __VA_ARGS__);         \
    sprintf_s(log_buf_c, 512, fmt, __VA_ARGS__); \
    dbg_log << log_buf_c;                        \
	}

#if extended_debug_log
#define extendedDebugLog(X)                      \
	OutputDebugString(_T(X));                    \
	dbg_log << X; 
#define extendedDebugLogFmt(fmt, ...) {          \
	wsprintfW(log_buf, _T(fmt), __VA_ARGS__);    \
	OutputDebugString(log_buf);                  \
    sprintf_s(log_buf_c, 512, fmt, __VA_ARGS__); \
    dbg_log << log_buf_c;                        \
	}


#if super_extended_debug_log
#define superExtendedDebugLog(X)                 \
	OutputDebugString(_T(X));                    \
	dbg_log << X; 
#define superExtendedDebugLogFmt(fmt, ...) {     \
	wsprintfW(log_buf, _T(fmt), __VA_ARGS__);    \
	OutputDebugString(log_buf);                  \
    sprintf_s(log_buf_c, 512, fmt, __VA_ARGS__); \
    dbg_log << log_buf_c;                        \
	}

#else
#define superExtendedDebugLog(X)      0
#define superExtendedDebugLogFmt(...) 0
#endif
#else
#define extendedDebugLog(X)           0
#define extendedDebugLogFmt(...)      0
#define superExtendedDebugLog(X)      0
#define superExtendedDebugLogFmt(...) 0
#endif
#else
#define debugLog(X)                   0
#define debugLogFmt(...)              0
#define extendedDebugLog(X)           0
#define extendedDebugLogFmt(...)      0
#define superExtendedDebugLog(X)      0
#define superExtendedDebugLogFmt(...) 0
#endif

#if trace_log
#define traceLog() {                                           \
	dbg_log << __LINE__ << " - " << __FUNCTION__ << std::endl; \
}
#else
#define traceLog() 0
#endif

#define BEGIN_USING_MODELS                                     \
	DWORD M_MODELS_NOT_USING_WaitResult = WaitForSingleObject( \
	M_MODELS_NOT_USING,    				                       \
	INFINITE);                                                 \
	switch (M_MODELS_NOT_USING_WaitResult) {

#define END_USING_MODELS }

#define BEGIN_USING_NETWORK                                    \
	DWORD M_MODELS_NOT_USING_WaitResult = WaitForSingleObject( \
	M_NETWORK_NOT_USING,    				                   \
	INFINITE);                                                 \
	switch (M_MODELS_NOT_USING_WaitResult) {

#define END_USING_NETWORK }

#define NET_BUFFER_SIZE 16384
#define MARKERS_SIZE 12

#define ID_SIZE 4
#define DWNLD_TOKEN_SIZE 252

#define HEVENTS_COUNT 2

#define STAGES_COUNT   8
#define MESSAGES_COUNT 8
#define SECTIONS_COUNT 10

#define BALL        0
#define CANDY_CANE  1
#define FIR         2
#define SANTA_CLAUS 3
#define SNOWMAN     4
#define GIFTS_BOXES 5
#define PIG         6
#define LOLLIPOP    7
#define MANDARINE   8
#define WOOD_TOILET 9


enum STAGE_ID {
	WAITING = 0,
	START,
	COMPETITION,
	GET_SCORE,
	ITEMS_NOT_EXISTS,
	END_BY_TIME,
	END_BY_COUNT,
	STREAMER_MODE
};


enum EVENT_ID {
	IN_HANGAR = 0,
	IN_BATTLE_GET_FULL,
	IN_BATTLE_GET_SYNC,
	DEL_LAST_MODEL
};


enum MODS_ID {
	HELLOWEEN = 1,
	NY_EVENT  = 2
};


extern uint16_t SCORE[SECTIONS_COUNT];

extern LPCWSTR EVENT_NAMES[4];

extern char* COLOURS[MESSAGES_COUNT];
extern char* MESSAGES[MESSAGES_COUNT];
extern char* MODEL_NAMES[SECTIONS_COUNT];

extern bool isInited;

extern bool battleEnded;

extern bool isModelsAlreadyCreated;

extern int8_t scoreID;
extern STAGE_ID lastStageID;
extern bool isStreamer;

extern std::ofstream dbg_log;


struct ModelsSection  {
	bool isInitialised = false;

	uint8_t ID    = NULL;
	char* path    = nullptr;

	std::vector<float*> models;
};

struct map {
	uint16_t minimap_count  = 0;

	STAGE_ID stageID        = STAGE_ID::COMPETITION;
	uint32_t time_preparing = 0;

	//types of positions sections

	std::vector<ModelsSection> modelsSects;
};

struct map_sync {
	uint16_t all_models_count = NULL;

	//types of positions sections

	std::vector<ModelsSection> modelsSects_creating;
	std::vector<ModelsSection> modelsSects_deleting;
};

extern map      current_map;
extern map_sync sync_map;

struct data_c {
	uint16_t version = NULL; //version_id
	bool enabled = true;
};

struct i18n_c {
	uint16_t version = NULL; //version_id
};

struct Config {
	char* ids = "NY_Event";
	char* author = "by Pavel3333 & RAINN VOD";
	char* version = "v1.0.0.1 (10.02.2019)";
	char* patch = "1.4.0.0";
	uint16_t version_id = 100U;
	data_c data;
	i18n_c i18n;
};

extern Config config;

double getDist2Points(double*, float*);

uint32_t curl_init();
void     curl_clean();

const std::vector<float*>* findModelsByID(std::vector<ModelsSection>&, uint8_t);

uint8_t parse_config();
uint8_t parse_sync();
uint8_t parse_del_model();

bool file_exists(const char*);

uint8_t send_token(uint32_t, uint8_t, EVENT_ID, uint8_t modelID = NULL, float* coords_del = nullptr);
