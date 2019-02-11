﻿#include "API_functions.h"

#include <cmath>
#include <filesystem>
#include <sstream>
#include "curl/curl.h"
#pragma comment(lib,"libcurl.lib")

#undef debug_log
#define debug_log false

unsigned char wr_buf[NET_BUFFER_SIZE + 1];
size_t wr_index = NULL;

map      current_map;
map_sync sync_map;

CURL *  curl_handle = NULL;

int8_t scoreID = -1;

LPCWSTR EVENT_NAMES[] { traceLog();
	L"NY_Event_HangarEvent",
	L"NY_Event_StartTimerEvent",
	L"NY_Event_StartTimerEvent",
	L"NY_Event_DelEvent"
};

char* COLOURS[MESSAGES_COUNT] { traceLog();
	"FFFF00FF",
	"00E600FF",
	"000000FF",
	"00E600FF",
	"FF4D00FF",
	"FFFF00FF",
	"FFFF00FF",
	"00E600FF"
};

char* MESSAGES[MESSAGES_COUNT] { traceLog();
	"\\c%s;Waiting for competition...",
	"\\c%s;START!!!",
	"",
	"\\c%s;+%d score!",
	"\\c%s;No items near!",
	"\\c%s;Competition is over: time is up",
	"\\c%s;Competition is over: all items found",
	"\\c%s;Streamer mode"
};

uint16_t SCORE[SECTIONS_COUNT]{ traceLog(); 13, 5, 11, 3000, 3, 7, 9, 15, 17, 100 };

char* MODEL_NAMES[SECTIONS_COUNT] { traceLog();
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

const std::vector<float*>* findModelsByID(std::vector<ModelsSection>& modelsSects, uint8_t ID) { traceLog();
	for (const auto &it : modelsSects) { traceLog();
		if (it.isInitialised && it.ID == ID) { traceLog();
			return &it.models;
		}
	}

	return nullptr;
}

bool file_exists(const char *fname)
{ traceLog();
	return std::filesystem::exists(fname);
}

double getDist2Points(double* point1, float* point2) { traceLog();
	if (point1 == nullptr || point2 == nullptr) { traceLog();
		return -1.0;
	}

	double dist_x = point2[0] - (double)point1[0];
	double dist_y = point2[1] - (double)point1[1];
	double dist_z = point2[2] - (double)point1[2];

	return sqrt(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);
}

//-----------------

//generate random bytes
void generate_random_bytes(unsigned char* out, size_t length) { traceLog();
	unsigned char ret = NULL;
	for (uint_fast32_t i = NULL; i < length; i++) { traceLog();
		ret = rand() % 256;
		while (ret == '"' || ret == '\t' || ret == '\n') ret = rand() % 256;
		out[i] = ret;
	}
}

//writing response from server into array ptr and return size of response
static size_t write_data(char *ptr, size_t size, size_t nmemb, char* data){ traceLog();
	if (data == NULL || wr_index + size * nmemb > NET_BUFFER_SIZE) return 0U; // Error if out of buffer

	memcpy(&data[wr_index], ptr, size*nmemb);// appending data into the end
	wr_index += size * nmemb;  // changing position
	return size * nmemb;
}

std::string urlencode(unsigned char* s, size_t size)
{ traceLog();
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for (size_t i = NULL; i < size; i++)
	{ traceLog();
		const char& c = s[i];
		if ((c > 47U && c < 58U) ||//0-9
			(c > 64U && c < 91U) ||//abc...xyz
			(c > 96U && c < 123U) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{ traceLog();
			e << c;
		}
		else
		{ traceLog();
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}

	return e.str();
}

//---------------------------------------------------------------------------------------------------------------

//------------------------------------------CLIENT-SERVER PART---------------------------------------------------

uint32_t curl_init() { traceLog();
	if (curl_global_init(CURL_GLOBAL_ALL))
	{ traceLog();
		return 1;
	}

	curl_handle = curl_easy_init();

	if (!curl_handle) { traceLog();
		return 2;
	}

	return 0;
}

void curl_clean() { traceLog();
	curl_easy_cleanup(curl_handle);

	curl_global_cleanup();
}

//getting token in pre-auth step
static uint8_t get_token(std::string input) { traceLog();
	if (!curl_handle) { traceLog();
		return 1U;
	}

	memset(wr_buf, NULL, NET_BUFFER_SIZE + 1U); // filling buffer by NULL
	wr_index = NULL;

	char user_agent[] = "NY_Event";

	size_t length = input.length();

	char* url = new char[47 + length + 1];

	char url_[48] = "http://api.pavel3333.ru/events/index.php?token=";

	memcpy(url, url_, 47U);
	memcpy(&url[47], input.c_str(), length);
	url[47 + length] = NULL;

#if debug_log
	std::ofstream fil("url_pos.txt", std::ios::binary);

	fil << url;

	fil.close();
#endif

	//setting user agent
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent);
	// setting url
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	//setting function for write data
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
	//setting buffer
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, wr_buf);
	//setting max buffer size
	curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 32768U);
	// requesting
	CURLcode res = curl_easy_perform(curl_handle);

	memset(url, NULL, 47U + length);
	delete[] url;

	return res;
}

uint8_t parse_config() { traceLog(); // при входе в бой
	if (!wr_index) { traceLog();
		return 21U;
	}

	uint32_t offset = NULL;

	uint16_t length = NULL;

	memcpy(&length, wr_buf, 2U);

	if (length != wr_index) { traceLog();
		return 22U;
	}

	offset += 2U;

	if (wr_index == 3U) { traceLog(); //код ошибки
		uint8_t error_code = wr_buf[offset];

		if (error_code < 10U) { traceLog();
			if (error_code == 7U) { traceLog();
				current_map.stageID = STAGE_ID::WAITING;

				return NULL;
			}
			else if (error_code == 8U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_TIME;

				return NULL;
			}
			else if (error_code == 9U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_COUNT;

				return NULL;
			}

			return (uint32_t)wr_buf[offset];
		}

		return (uint32_t)wr_buf[offset];
	}
	else if (wr_index >= 8U) { traceLog();
		/*
		Всё прошло успешно.
		Первый байт - ноль для проверки,
		второй байт - 0/1 (СТАРТ / соревнование идет),
		остальные четыре байта - оставшееся время
		*/

		if (wr_buf[offset] != 0) { traceLog();
			return 23U;
		}

		current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

		if (current_map.stageID == STAGE_ID::COMPETITION)
			isStreamer = false;
		else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
			isStreamer = true;

		memcpy(&(current_map.time_preparing), wr_buf + offset + 2U, 4U);

		offset += 6U;

		if (wr_index > 8U) { traceLog(); //парсинг координат
			uint8_t sections_count = wr_buf[offset];

			offset++;

			if (sections_count > SECTIONS_COUNT) { traceLog();
				return 24U; //проверяем валидность числа секций
			}

			memcpy(&(current_map.minimap_count), wr_buf + offset, 2U);

			offset += 2U;

			uint8_t model_type = NULL;
			uint16_t models_count_sect = NULL;

			for (uint16_t i = NULL; i < sections_count; i++) { traceLog();
				model_type = wr_buf[offset];

				offset++;

				if (model_type >= SECTIONS_COUNT) { traceLog();
					return 25U;  //проверяем валидность типа модели
				}

				memcpy(&models_count_sect, wr_buf + offset, 2U);

				offset += 2U;

				if (!models_count_sect) { traceLog();
					continue; //нет моделей, идем далее
				}

				char *path_buffer = new char[80];

				sprintf_s(path_buffer, 80U, "objects/pavel3333_NewYear/%s/%s.model", MODEL_NAMES[model_type], MODEL_NAMES[model_type]); //форматируем путь к модели
				//sprintf_s(path_buffer, 80U, "objects/misc/bbox/sphere1.model"); //форматируем путь к модели

				//инициализация новой секции моделей

				ModelsSection model_sect{ traceLog();
					false,
					model_type,
					path_buffer
				};

				//----------------------------------

				model_sect.models.resize(models_count_sect);

				for (uint16_t i = NULL; i < models_count_sect; i++) { traceLog();
					float* coords = new float[3];

					for (uint8_t j = NULL; j < 3U; j++) { traceLog();
						memcpy(&coords[j], wr_buf + offset, 4U);

						offset += 4U;
					}

					model_sect.models[i] = coords;
				}

				model_sect.isInitialised = true;

				//добавление секции

				current_map.modelsSects.push_back(model_sect);

				//-----------------
			}

			isModelsAlreadyCreated = true; //если парсинг пакета был удачен и это было событие полного создания моделей, то мы получили полный пакет моделей
		}

		return NULL;
	}

	return 26U;
}

uint8_t parse_sync() { traceLog(); // синхронизация
	if (!wr_index) { traceLog();
		return 21U;
	}

	uint32_t offset = NULL;

	uint16_t length = NULL;

	memcpy(&length, wr_buf, 2U);

	if (length != wr_index) { traceLog();
		return 22U;
	}

	offset += 2U;

	if (wr_index == 3U) { traceLog(); //код ошибки
		uint8_t error_code = wr_buf[offset];

		if (error_code < 10U) { traceLog();
			if (error_code == 7U) { traceLog();
				current_map.stageID = STAGE_ID::WAITING;

				return NULL;
			}
			else if (error_code == 8U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_TIME;

				return NULL;
			}
			else if (error_code == 9U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_COUNT;

				return NULL;
			}

			return (uint32_t)wr_buf[offset];
		}

		return (uint32_t)wr_buf[offset];
	}
	else if (wr_index >= 8U) { traceLog();
		/*
		Всё прошло успешно.
		Первый байт - ноль для проверки,
		второй байт - 0/1 (СТАРТ / соревнование идет),
		остальные четыре байта - оставшееся время

		0 - код создания моделей
		  1б - число секций для создания
		  2б - число создаваемых моделей
		       координаты создаваемых моделей
		1 - код удаления моделей
		  1б - число секций для удаления
		  2б - число удаляемых моделей
		       координаты удаляемых моделей
		*/

		if (wr_buf[offset] != 0) { traceLog();
			return 23U;
		}

		current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

		if (current_map.stageID == STAGE_ID::COMPETITION)
			isStreamer = false;
		else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
			isStreamer = true;

		memcpy(&(current_map.time_preparing), wr_buf + offset + 2U, 4U);

		offset += 6U;

		if (wr_index > 8U) { traceLog(); //парсинг координат
			std::vector<ModelsSection>* sect = nullptr;

			for (uint8_t modelSectionID = NULL; modelSectionID < 2U; modelSectionID++) { traceLog();
				if      (modelSectionID == 0U && wr_buf[offset] == 0U) sect = &(sync_map.modelsSects_creating);
				else if (modelSectionID == 1U && wr_buf[offset] == 1U) sect = &(sync_map.modelsSects_deleting);
				else { traceLog();
					OutputDebugString(_T("[NY_Event]: Found unexpected section while synchronizing!\n"));

					break;
				}

				offset++;

				uint8_t sections_count = wr_buf[offset];

				offset++;

				if (sections_count > SECTIONS_COUNT) { traceLog();
					return 24U; //проверяем валидность числа секций
				}

				memcpy(&(sync_map.all_models_count), wr_buf + offset, 2U);

				offset += 2U;

				uint8_t model_type = NULL;
				uint16_t models_count_sect = NULL;

				for (uint16_t i = NULL; i < sections_count; i++) { traceLog();
					model_type = wr_buf[offset];

					offset++;

					if (model_type >= SECTIONS_COUNT) { traceLog();
						return 25U;  //проверяем валидность типа модели
					}

					memcpy(&models_count_sect, wr_buf + offset, 2U);

					offset += 2U;

					if (!models_count_sect) { traceLog();
						continue; //нет моделей, идем далее
					}

					char *path_buffer = new char[80];

					sprintf_s(path_buffer, 80U, "objects/pavel3333_NewYear/%s/%s.model", MODEL_NAMES[model_type], MODEL_NAMES[model_type]); //форматируем путь к модели
					//sprintf_s(path_buffer, 80U, "objects/misc/bbox/sphere1.model"); //форматируем путь к модели

					//инициализация новой секции моделей

					ModelsSection model_sect { traceLog();
						false,
						model_type,
						path_buffer
					};

					//----------------------------------

					model_sect.models.resize(models_count_sect);

					for (uint16_t i = NULL; i < models_count_sect; i++) { traceLog();
						float* coords = new float[3];

						for (uint8_t j = NULL; j < 3U; j++) { traceLog();
							memcpy(&coords[j], wr_buf + offset, 4U);

							offset += 4U;
						}

						model_sect.models[i] = coords;
					}

					model_sect.isInitialised = true;

					//добавление секции

					sect->push_back(model_sect);

					//-----------------
				}
			}

			return NULL;
		}

		return NULL;
	}

	return 25U;
}

uint8_t parse_del_model() { traceLog();
	if (!wr_index) { traceLog();
		return 21U;
	}

	uint32_t offset = NULL;

	uint16_t length = NULL;

	memcpy(&length, wr_buf, 2U);

	if (length != wr_index) { traceLog();
		return 22U;
	}

	offset += 2U;

	if (wr_index == 3U) { traceLog(); //код ошибки
		uint8_t error_code = wr_buf[offset];

		if (error_code < 10U) { traceLog();
			if (error_code == 7U) { traceLog();
				current_map.stageID = STAGE_ID::WAITING;
			}
			else if (error_code == 8U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_TIME;
			}
			else if (error_code == 9U) { traceLog();
				current_map.stageID = STAGE_ID::END_BY_COUNT;
			}

			return error_code;
		}

		return error_code;
	}
	else if (wr_index >= 9U) { traceLog();
		/*
		Всё прошло успешно.
		Первый байт - ноль для проверки,
		второй байт - 0/1 (СТАРТ / соревнование идет),
		остальные четыре байта - оставшееся время
		*/

		if (wr_buf[offset] != 0) { traceLog();
			return 23U;
		}

		current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

		memcpy(&(current_map.time_preparing), wr_buf + offset + 2U, 4U);

		offset += 6U;

		if (wr_buf[offset] == 0U) return NULL;

		return 27U;
	}

	return 26U;
}

uint8_t send_token(uint32_t id, uint8_t map_id, EVENT_ID eventID, uint8_t modelID, float* coords_del) { traceLog();
	unsigned char* token = nullptr;

	uint16_t size = NULL;

	//Код наполнения токена по типу события

	if (eventID == EVENT_ID::IN_HANGAR || eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog();
		size = 7U;

		token = new unsigned char[size + 1];

		token[0] = MODS_ID::NY_EVENT;    //mod
		token[1] = map_id;             //map ID

		memcpy(&token[2], &id, 4U);

		token[6] = eventID; //код события
	}
	else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog();
		if (coords_del == nullptr) { traceLog();
			return 24U;
		}

		size = 20U;

		token = new unsigned char[size + 1];

		token[0] = MODS_ID::NY_EVENT;    //mod
		token[1] = map_id;             //map ID

		memcpy(&token[2], &id, 4U);

		token[6] = eventID; //код события
		token[7] = modelID; //код модели

		memcpy(&token[8],  &coords_del[0], 4U);
		memcpy(&token[12], &coords_del[1], 4U);
		memcpy(&token[16], &coords_del[2], 4U);
	}

	//-------------------------------------

	if (token == nullptr) { traceLog();
		return 21U;
	}

	token[size] = NULL;

#if debug_log
	std::ofstream tok("token_pos.bin", std::ios::binary);

	tok.write((const char*)token, size);

	tok.close();
#endif

	std::string new_token = urlencode(token, size);

	delete[] token;

	uint8_t code = get_token(new_token);

	new_token.~basic_string();

	if (code || !wr_index) { traceLog(); //get token
		return 22U;
	}

#if debug_log
	std::ofstream resp("responce_pos.bin", std::ios::binary);

	resp.write((const char*)wr_buf, wr_index);

	resp.close();
#endif

	uint16_t len;
	uint16_t offset = NULL;

	if (eventID == EVENT_ID::IN_HANGAR) { traceLog(); //запрос из ангара: 3 байта - 2 байта длина; 1 байт либо 0, либо 6, либо ошибка (больше 9);
		//проверяем ответ от сервера

		if (wr_index == 3U) { traceLog();
			memcpy(&len, wr_buf, 2U); //смотрим длину данных

			if (len == wr_index) { traceLog();
				return (uint32_t)(wr_buf[2]);
			}

			return 25U;
		}

		return 24U;
	}
	else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog(); 
		return NULL;
	}
	else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog();
		return NULL;
	}

	return 23U; //неизвестный ивент

	/*if (!map_id) { traceLog();
		//if (wr_index < 3U) { traceLog();
			return (uint32_t)(wr_buf[0]);
		//}
		//else return 10U;
	}
	else { traceLog();
		if (wr_index < 3U) { traceLog();
			return 9U;
		}
		else return NULL;
	}*/
	
}