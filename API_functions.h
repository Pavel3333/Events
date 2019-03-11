#pragma once


// Код ошибки
enum [[nodiscard]] MyErr : int {
	OK = 0 // ошибки нет
};

#define return_err return (MyErr)
#define return_ok return (MyErr::OK)


#define ID_SIZE 4
#define DWNLD_TOKEN_SIZE 252

#define HEVENTS_COUNT 2

#define STAGES_COUNT   8
#define MESSAGES_COUNT 8
#define SECTIONS_COUNT 10


enum MODEL_ID : uint8_t {
	BALL = 0,
	CANDY_CANE,
	FIR,
	SANTA_CLAUS,
	SNOWMAN,
	GIFTS_BOXES,
	PIG,
	LOLLIPOP,
	MANDARINE,
	WOOD_TOILET,
	UNKNOWN = 0xff // не модель
};

#define MAIN_COMPETITION_MAP 217

#define FUN_COMPETITION_MAP 4


enum STAGE_ID : uint8_t {
	WAITING = 0,
	START,
	COMPETITION,
	GET_SCORE,
	ITEMS_NOT_EXISTS,
	END_BY_TIME,
	END_BY_COUNT,
	STREAMER_MODE
};

enum EVENT_ID : uint8_t {
	IN_HANGAR = 0,
	IN_BATTLE_GET_FULL,
	IN_BATTLE_GET_SYNC,
	DEL_LAST_MODEL
};

enum MODS_ID : uint8_t {
	HELLOWEEN = 1,
	NY_EVENT  = 2
};

struct ModelSync {
	uint16_t syncID = 0;
	float* coords = nullptr;
};

struct ModelsFullSection {
	bool isInitialised = false;

	MODEL_ID ID = MODEL_ID::UNKNOWN;
	char* path = nullptr;

	std::vector<float*> models;
};

struct ModelsSyncSection {
	bool isInitialised = false;

	MODEL_ID ID = MODEL_ID::UNKNOWN;
	char* path = nullptr;

	std::vector<ModelSync> models;
};

struct map {
	uint16_t minimap_count = 0;

	STAGE_ID stageID = STAGE_ID::COMPETITION;
	uint32_t time_preparing = 0;

	//types of positions sections

	std::vector<ModelsFullSection> modelsSects;
};

struct map_sync {
	uint16_t all_models_count = 0;

	//types of positions sections

	std::vector<ModelsSyncSection> modelsSects_creating;
	std::vector<ModelsSyncSection> modelsSects_deleting;
};

extern uint16_t SCORE[SECTIONS_COUNT];

extern LPCWSTR EVENT_NAMES[4];

extern char* COLOURS[MESSAGES_COUNT];
extern char* MESSAGES[MESSAGES_COUNT];
extern char* MODEL_NAMES[SECTIONS_COUNT];

extern DWORD WINAPI TimerThread(LPVOID);
extern DWORD WINAPI HandlerThread(LPVOID);

extern bool isInited;

extern uint8_t first_check;

extern bool battleEnded;

extern bool isModelsAlreadyCreated;

extern int8_t scoreID;
extern STAGE_ID lastStageID;
extern bool isStreamer;

extern map      current_map;
extern map_sync sync_map;

float getDist2Points(const float[3], const float[3]);

const std::vector<float*>* findModelsByID(std::vector<ModelsFullSection>&, uint8_t);
