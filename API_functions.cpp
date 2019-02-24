#include "API_functions.h"

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

LPCWSTR EVENT_NAMES[] {
	L"NY_Event_HangarEvent",
	L"NY_Event_StartTimerEvent",
	L"NY_Event_StartTimerEvent",
	L"NY_Event_DelEvent"
};

char* COLOURS[MESSAGES_COUNT] {
	"0000CDFF",
	"00E600FF",
	"000000FF",
	"00E600FF",
	"FF0000FF",
	"FF0000FF",
	"FF0000FF",
	"000000FF"
};

char* MESSAGES[MESSAGES_COUNT] {
	"\\c%s;Waiting for competition...",
	"\\c%s;START!!!",
	"",
	"\\c%s;+%d score!",
	"\\c%s;No items near!",
	"\\c%s;Competition is over: time is up",
	"\\c%s;Competition is over: all items found",
	"\\c%s;Streamer mode"
};

uint16_t SCORE[SECTIONS_COUNT] { 13, 5, 11, 3000, 3, 7, 9, 15, 17, 100 };

char* MODEL_NAMES[SECTIONS_COUNT] {
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

const std::vector<float*>* findModelsByID(std::vector<ModelsSection>& modelsSects, uint8_t ID) {
	for (const auto &it : modelsSects) {
		if (it.isInitialised && it.ID == ID) {
			return &it.models;
		}
	}

	return nullptr;
}

bool file_exists(const char *fname)
{
	return std::filesystem::exists(fname);
}

float getDist2Points(const float point1[3], const float point2[3]) {
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
void generate_random_bytes(unsigned char* out, size_t length) {
	unsigned char ret = NULL;
	for (uint_fast32_t i = NULL; i < length; i++) {
		ret = rand() % 256;
		while (ret == '"' || ret == '\t' || ret == '\n') ret = rand() % 256;
		out[i] = ret;
	}
}

//writing response from server into array ptr and return size of response
static size_t write_data(char *ptr, size_t size, size_t nmemb, char* data){
	if (data == NULL || wr_index + size * nmemb > NET_BUFFER_SIZE) return 0; // Error if out of buffer

	memcpy(&data[wr_index], ptr, size*nmemb);// appending data into the end
	wr_index += size * nmemb;  // changing position
	return size * nmemb;
}

std::string urlencode(unsigned char* s, size_t size)
{
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for (size_t i = NULL; i < size; i++)
	{
		const char& c = s[i];
		if ((c > 47 && c < 58) ||//0-9
			(c > 64 && c < 91) ||//abc...xyz
			(c > 96 && c < 123) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}

	return e.str();
}

//---------------------------------------------------------------------------------------------------------------

//------------------------------------------CLIENT-SERVER PART---------------------------------------------------

uint32_t curl_init() {
	if (curl_global_init(CURL_GLOBAL_ALL))
	{
		return 1;
	}

	curl_handle = curl_easy_init();

	if (!curl_handle) {
		return 2;
	}

	return 0;
}

void curl_clean() {
	curl_easy_cleanup(curl_handle);

	curl_global_cleanup();
}

//getting token in pre-auth step
static uint8_t get_token(std::string input) {
	if (!curl_handle) {
		return 1;
	}

	memset(wr_buf, NULL, NET_BUFFER_SIZE + 1); // filling buffer by NULL
	wr_index = NULL;

	char user_agent[] = "NY_Event";

	size_t length = input.length();

	char* url = new char[47 + length + 1];

	char url_[48] = "http://api.pavel3333.ru/events/index.php?token=";

	memcpy(url, url_, 47);
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
	curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 32768);
	// requesting
	CURLcode res = curl_easy_perform(curl_handle);

	memset(url, NULL, 47 + length);
	delete[] url;

	return res;
}

uint8_t parse_event(EVENT_ID eventID) {
	//инициализация переменных для каждого события

	//EVENT_ID::IN_HANGAR

	uint16_t length = NULL;
	uint32_t offset = NULL;

	//EVENT_ID::IN_BATTLE_GET_FULL

	uint8_t error_code;

	uint8_t sections_count;
	uint8_t model_type         = NULL;
	uint16_t models_count_sect = NULL;

	//EVENT_ID::IN_BATTLE_GET_SYNC

	std::vector<ModelsSection>* sect = nullptr;

	//парсинг пакета с сервера

	switch (eventID) {
		case EVENT_ID::IN_HANGAR:          //запрос из ангара       
			if (wr_index == 3) {
				memcpy(&length, wr_buf, 2); //смотрим длину данных

				if (length == wr_index) {
					return wr_buf[2];
				}

				return 1;
			}

			return 2;

			break;
		case EVENT_ID::IN_BATTLE_GET_FULL: // при входе в бой
			if (!wr_index) {
				return 3;
			}

			memcpy(&length, wr_buf, 2);

			if (length != wr_index) {
				return 4;
			}

			offset += 2;

			if (wr_index == 3) { //код ошибки
				error_code = wr_buf[offset];

				if (error_code < 10) {
					if (error_code == 7) {
						current_map.stageID = STAGE_ID::WAITING;

						return NULL;
					}
					else if (error_code == 8) {
						current_map.stageID = STAGE_ID::END_BY_TIME;

						return NULL;
					}
					else if (error_code == 9) {
						current_map.stageID = STAGE_ID::END_BY_COUNT;

						return NULL;
					}

					return wr_buf[offset];
				}

				return wr_buf[offset];
			}
			else if (wr_index >= 8) {
				/*
				Всё прошло успешно.
				Первый байт - ноль для проверки,
				второй байт - 0/1 (СТАРТ / соревнование идет),
				остальные четыре байта - оставшееся время
				*/

				if (wr_buf[offset] != 0) {
					return 5;
				}

				current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

				if (current_map.stageID == STAGE_ID::COMPETITION)
					isStreamer = false;
				else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
					isStreamer = true;

				memcpy(&(current_map.time_preparing), wr_buf + offset + 2, 4);

				offset += 6;

				if (wr_index > 8) { //парсинг координат
					sections_count = wr_buf[offset];

					offset++;

					if (sections_count > SECTIONS_COUNT) {
						return 6; //проверяем валидность числа секций
					}

					memcpy(&(current_map.minimap_count), wr_buf + offset, 2);

					offset += 2;

					for (uint16_t i = NULL; i < sections_count; i++) {
						model_type = wr_buf[offset];

						offset++;

						if (model_type >= SECTIONS_COUNT) {
							return 7;  //проверяем валидность типа модели
						}

						memcpy(&models_count_sect, wr_buf + offset, 2);

						offset += 2;

						if (!models_count_sect) {
							continue; //нет моделей, идем далее
						}

						char *path_buffer = new char[80];

						sprintf_s(path_buffer, 80U, "objects/pavel3333_NewYear/%s/%s.model", MODEL_NAMES[model_type], MODEL_NAMES[model_type]); //форматируем путь к модели
						//sprintf_s(path_buffer, 80U, "objects/misc/bbox/sphere1.model"); //форматируем путь к модели

						//инициализация новой секции моделей

						ModelsSection model_sect{
							false,
							model_type,
							path_buffer
						};

						//----------------------------------

						model_sect.models.resize(models_count_sect);

						for (uint16_t i = NULL; i < models_count_sect; i++) {
							float* coords = new float[3];

							for (uint8_t j = NULL; j < 3; j++) {
								memcpy(&coords[j], wr_buf + offset, 4);

								offset += 4;
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

			return 8;

			break;
		case EVENT_ID::IN_BATTLE_GET_SYNC: // синхронизация
			if (!wr_index) {
				return 9;
			}

			memcpy(&length, wr_buf, 2);

			if (length != wr_index) {
				return 10;
			}

			offset += 2;

			if (wr_index == 3) { //код ошибки
				error_code = wr_buf[offset];

				if (error_code < 10) {
					if (error_code == 7) {
						current_map.stageID = STAGE_ID::WAITING;

						return NULL;
					}
					else if (error_code == 8) {
						current_map.stageID = STAGE_ID::END_BY_TIME;

						return NULL;
					}
					else if (error_code == 9) {
						current_map.stageID = STAGE_ID::END_BY_COUNT;

						return NULL;
					}

					return wr_buf[offset];
				}

				return wr_buf[offset];
			}
			else if (wr_index >= 8) {
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

				if (wr_buf[offset] != 0) {
					return 11;
				}

				current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

				if (current_map.stageID == STAGE_ID::COMPETITION)
					isStreamer = false;
				else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
					isStreamer = true;

				memcpy(&(current_map.time_preparing), wr_buf + offset + 2, 4);

				offset += 6;

				if (wr_index > 8) { //парсинг координат
					for (uint8_t modelSectionID = NULL; modelSectionID < 2; modelSectionID++) {
						if (modelSectionID == 0 && wr_buf[offset] == 0) sect = &(sync_map.modelsSects_creating);
						else if (modelSectionID == 1 && wr_buf[offset] == 1) sect = &(sync_map.modelsSects_deleting);
						else {
							extendedDebugLog("Found unexpected section while synchronizing!\n");

							break;
						}

						offset++;

						uint8_t sections_count = wr_buf[offset];

						offset++;

						if (sections_count > SECTIONS_COUNT) {
							return 12; //проверяем валидность числа секций
						}

						memcpy(&(sync_map.all_models_count), wr_buf + offset, 2);

						offset += 2;

						uint8_t model_type = NULL;
						uint16_t models_count_sect = NULL;

						for (uint16_t i = NULL; i < sections_count; i++) {
							model_type = wr_buf[offset];

							offset++;

							if (model_type >= SECTIONS_COUNT) {
								return 13;  //проверяем валидность типа модели
							}

							memcpy(&models_count_sect, wr_buf + offset, 2);

							offset += 2;

							if (!models_count_sect) {
								continue; //нет моделей, идем далее
							}

							char *path_buffer = new char[80];

							sprintf_s(path_buffer, 80U, "objects/pavel3333_NewYear/%s/%s.model", MODEL_NAMES[model_type], MODEL_NAMES[model_type]); //форматируем путь к модели
							//sprintf_s(path_buffer, 80U, "objects/misc/bbox/sphere1.model"); //форматируем путь к модели

							//инициализация новой секции моделей

							ModelsSection model_sect {
								false,
								model_type,
								path_buffer
							};

							//----------------------------------

							model_sect.models.resize(models_count_sect);

							for (uint16_t i = NULL; i < models_count_sect; i++) {
								float* coords = new float[3];

								for (uint8_t j = NULL; j < 3; j++) {
									memcpy(&coords[j], wr_buf + offset, 4);

									offset += 4;
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

			return 14;

			break;
		case EVENT_ID::DEL_LAST_MODEL:     // удаление ближайшей модели
			if (!wr_index) {
				return 15;
			}

			memcpy(&length, wr_buf, 2);

			if (length != wr_index) {
				return 16;
			}

			offset += 2;

			if (wr_index == 3) { //код ошибки
				uint8_t error_code = wr_buf[offset];

				if (error_code < 10) {
					if (error_code == 7) {
						current_map.stageID = STAGE_ID::WAITING;
					}
					else if (error_code == 8) {
						current_map.stageID = STAGE_ID::END_BY_TIME;
					}
					else if (error_code == 9) {
						current_map.stageID = STAGE_ID::END_BY_COUNT;
					}

					return error_code;
				}

				return error_code;
			}
			else if (wr_index >= 9) {
				/*
				Всё прошло успешно.
				Первый байт - ноль для проверки,
				второй байт - 0/1 (СТАРТ / соревнование идет),
				остальные четыре байта - оставшееся время
				*/

				if (wr_buf[offset] != 0) {
					return 17;
				}

				current_map.stageID = (STAGE_ID)wr_buf[offset + 1];

				memcpy(&(current_map.time_preparing), wr_buf + offset + 2, 4);

				offset += 6;

				if (wr_buf[offset] == NULL) return NULL;

				return 18;
			}

			return 19;

			break;
		default:
			return 20;

			break;
	}

	return 21;
}

uint8_t send_token(uint32_t id, uint8_t map_id, EVENT_ID eventID, uint8_t modelID, float* coords_del) {
	unsigned char* token = nullptr;

	uint16_t size = NULL;

	//Код наполнения токена по типу события

	if (eventID == EVENT_ID::IN_HANGAR || eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) {
		size = 7;

		token = new unsigned char[size + 1];

		token[0] = MODS_ID::NY_EVENT;    //mod
		token[1] = map_id;             //map ID

		memcpy(&token[2], &id, 4);

		token[6] = eventID; //код события
	}
	else if (eventID == EVENT_ID::DEL_LAST_MODEL) {
		if (coords_del == nullptr) {
			return 24;
		}

		size = 20;

		token = new unsigned char[size + 1];

		token[0] = MODS_ID::NY_EVENT;    //mod
		token[1] = map_id;             //map ID

		memcpy(&token[2], &id, 4);

		token[6] = eventID; //код события
		token[7] = modelID; //код модели

		memcpy(&token[8],  &coords_del[0], 4);
		memcpy(&token[12], &coords_del[1], 4);
		memcpy(&token[16], &coords_del[2], 4);
	}

	//-------------------------------------

	if (token == nullptr) {
		return 1;
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

	if (code || !wr_index) { //get token
		return 2;
	}

#if debug_log
	std::ofstream resp("responce_pos.bin", std::ios::binary);

	resp.write((const char*)wr_buf, wr_index);

	resp.close();
#endif

	return NULL;
}
