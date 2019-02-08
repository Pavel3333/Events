#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>
#include <tchar.h>

#define NETWORK_USING     ResetEvent(EVENT_NETWORK_NOT_USING->hEvent);
#define NETWORK_NOT_USING \
	if(!SetEvent(EVENT_NETWORK_NOT_USING->hEvent)) { \
		OutputDebugString(_T("NetworkNotUsingEvent not setted!\n")); \
	}

#define BEGIN_NETWORK_USING { \
	ResetEvent(EVENT_NETWORK_NOT_USING->hEvent);

#define END_NETWORK_USING \
	if(!SetEvent(EVENT_NETWORK_NOT_USING->hEvent)) { \
		OutputDebugString(_T("NetworkNotUsingEvent not setted!\n")); \
	}\
}

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


typedef struct {
	uint8_t IN_HANGAR          = 0U;
	uint8_t IN_BATTLE_GET_FULL = 1U;
	uint8_t IN_BATTLE_GET_SYNC = 2U;
	uint8_t DEL_LAST_MODEL     = 3U;
} EVENT_ID;

typedef struct {
	uint8_t HELLOWEEN        = 1U;
	uint8_t NY_EVENT         = 2U;
} MODS_ID;

extern MODS_ID  ModsID;
extern EVENT_ID EventsID;


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

typedef struct {
	uint16_t all_models_count = NULL;

	//types of positions sections

	std::vector<ModelsSection> modelsSects_creating;
	std::vector<ModelsSection> modelsSects_deleting;
} map_sync;

extern map      current_map;
extern map_sync sync_map;

struct data_c {
	uint16_t version = NULL; //version_id
	bool enabled = true;
};

struct i18n_c {
	uint16_t version = NULL; //version_id
};

class Config {
public:
	char* ids = "NY_Event";
	char* author = "by Pavel3333 & RAINN VOD";
	char* version = "v1.0.0.0 (06.02.2019)";
	char* patch = "1.4.0.0";
	uint16_t version_id = 100U;
	data_c data;
	i18n_c i18n;
};

extern Config config;

double getDist2Points(double*, float*);

uint32_t curl_init();
void     curl_clean();

const std::vector<float*>* findModelsByID(std::vector<ModelsSection>*, uint8_t);

uint8_t parse_config();
uint8_t parse_sync();
uint8_t parse_del_model();

bool file_exists(const char*);

uint8_t send_token(uint32_t, uint8_t, uint8_t, uint8_t modelID, float* coords_del);