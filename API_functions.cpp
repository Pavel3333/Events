#include "pch.h"
#include "API_functions.h"


map      current_map;
map_sync sync_map;

int8_t scoreID = -1;

const char* EVENT_NAMES[] {
	"NY_Event_HangarEvent",
	"NY_Event_StartTimerEvent",
	"NY_Event_StartTimerEvent",
	"NY_Event_DelEvent"
};

const char* COLOURS[MESSAGES_COUNT] {
	"0000CDFF",
	"00E600FF",
	"000000FF",
	"00E600FF",
	"FF0000FF",
	"FF0000FF",
	"FF0000FF",
	"000000FF"
};

const char* MESSAGES[MESSAGES_COUNT] {
	"\\c%s;Waiting for competition...",
	"\\c%s;START!!!",
	"",
	"\\c%s;+%d score!",
	"\\c%s;No items near!",
	"\\c%s;Competition is over: time is up",
	"\\c%s;Competition is over: all items found",
	"\\c%s;Streamer mode"
};

const uint16_t SCORE[SECTIONS_COUNT] { 22, 14, 18, 6666, 10, 6, 26, 34, 30, 100 };

const char* MODEL_NAMES[SECTIONS_COUNT] {
	"ball",
	"candy_cane",
	"fir",
	"Santa_Claus",
	"snowman",
	"gifts_boxes",
	"pig",
	"lollipop",
	"mandarine",
	"wood_toilet"
};

//---------------------------------------------API functions--------------------------------------------------------

const std::vector<float*>* findModelsByID(std::vector<ModelsFullSection>& modelsSects, MODEL_ID ID) {
	for (const auto &it : modelsSects) {
		if (it.isInitialised && it.ID == ID) {
			return &it.models;
		}
	}

	return nullptr;
}

// эту функцию можно заоптимизировать
float getDist2Points(const float point1[3], const float point2[3])
{
	if (!point1 || !point2) {
		return -1.0;
	}

	float dist_x = point2[0] - point1[0];
	float dist_y = point2[1] - point1[1];
	float dist_z = point2[2] - point1[2];

	return sqrtf(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);
}

//-----------------

//generate random bytes
void generate_random_bytes(unsigned char* out, size_t length)
{
	srand(static_cast<uint32_t>(time(nullptr)));
	unsigned char ret = 0;

	for (size_t i = 0; i < length; i++) {
		ret = rand() % 256;

		while (ret == '"' || ret == '\t' || ret == '\n')
			ret = rand() % 256;

		out[i] = ret;
	}
}
