#pragma once

#include <cstdint>

#define MOD_VERSION "v1.0.0.4 (" __TIMESTAMP__ ")"

struct data_c {
	uint16_t version = 0; //version_id
	bool enabled = true;
};

struct i18n_c {
	uint16_t version = 0; //version_id
};

class Config {
public:
	static char* ids;
	static char* author;
	static char* version;
	static char* patch;
	static uint16_t version_id;
	static data_c data;
	static i18n_c i18n;
};
