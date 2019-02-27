#include "Network.h"

//------------------------------------------CLIENT-SERVER PART---------------------------------------------------

CURL *  curl_handle = NULL;

unsigned char response_buffer[NET_BUFFER_SIZE + 1];
size_t response_size = NULL;

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

//writing response from server into array ptr and return size of response
static size_t write_data(char *ptr, size_t size, size_t nmemb, char* data) {
	if (data == NULL || response_size + size * nmemb > NET_BUFFER_SIZE) return 0; // Error if out of buffer

	memcpy(&data[response_size], ptr, size*nmemb);// appending data into the end
	response_size += size * nmemb;  // changing position
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

static uint8_t send_to_server(std::string input) {
	if (!curl_handle) {
		return 1;
	}

	memset(response_buffer, NULL, NET_BUFFER_SIZE + 1); // filling buffer by NULL
	response_size = NULL;

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
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response_buffer);
	//setting max buffer size
	curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 32768);
	// requesting
	CURLcode res = curl_easy_perform(curl_handle);

	memset(url, NULL, 47 + length);
	delete[] url;

	return res;
}

uint8_t send_token(uint32_t id, uint8_t map_id, EVENT_ID eventID, MODEL_ID modelID, float* coords_del) {
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

		memcpy(token + 8, coords_del, 12);
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

	uint8_t code = send_to_server(new_token);

	new_token.~basic_string();

	if (code || !response_size) { //get token
		return 2;
	}

#if debug_log
	std::ofstream resp("responce_pos.bin", std::ios::binary);

	resp.write((const char*)response_buffer, response_size);

	resp.close();
#endif

	return NULL;
}

uint8_t parse_event(EVENT_ID eventID)
{
	INIT_LOCAL_MSG_BUFFER;

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
			if (response_size == 3) {
				memcpy(&length, response_buffer, 2); //смотрим длину данных

				if (length == response_size) {
					return response_buffer[2];
				}

				return 1;
			}

			return 2;

			break;
		case EVENT_ID::IN_BATTLE_GET_FULL: // при входе в бой
			if (!response_size) {
				return 3;
			}

			memcpy(&length, response_buffer, 2);

			if (length != response_size) {
				return 4;
			}

			offset += 2;

			if (response_size == 3) { //код ошибки
				error_code = response_buffer[offset];

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

					return response_buffer[offset];
				}

				return response_buffer[offset];
			}
			else if (response_size >= 8) {
				/*
				Всё прошло успешно.
				Первый байт - ноль для проверки,
				второй байт - 0/1 (СТАРТ / соревнование идет),
				остальные четыре байта - оставшееся время
				*/

				if (response_buffer[offset] != 0) {
					return 5;
				}

				current_map.stageID = (STAGE_ID)response_buffer[offset + 1];

				if (current_map.stageID == STAGE_ID::COMPETITION)
					isStreamer = false;
				else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
					isStreamer = true;

				memcpy(&(current_map.time_preparing), response_buffer + offset + 2, 4);

				offset += 6;

				if (response_size > 8) { //парсинг координат
					sections_count = response_buffer[offset];

					offset++;

					if (sections_count > SECTIONS_COUNT) {
						return 6; //проверяем валидность числа секций
					}

					memcpy(&(current_map.minimap_count), response_buffer + offset, 2);

					offset += 2;

					for (uint16_t i = NULL; i < sections_count; i++) {
						model_type = response_buffer[offset];

						offset++;

						if (model_type >= SECTIONS_COUNT) {
							return 7;  //проверяем валидность типа модели
						}

						memcpy(&models_count_sect, response_buffer + offset, 2);

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
							(MODEL_ID)model_type,
							path_buffer
						};

						//----------------------------------

						model_sect.models.resize(models_count_sect);

						for (uint16_t i = NULL; i < models_count_sect; i++) {
							float* coords = new float[3];

							for (uint8_t j = NULL; j < 3; j++) {
								memcpy(&coords[j], response_buffer + offset, 4);

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
			if (!response_size) {
				return 9;
			}

			memcpy(&length, response_buffer, 2);

			if (length != response_size) {
				return 10;
			}

			offset += 2;

			if (response_size == 3) { //код ошибки
				error_code = response_buffer[offset];

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

					return response_buffer[offset];
				}

				return response_buffer[offset];
			}
			else if (response_size >= 8) {
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

				if (response_buffer[offset] != 0) {
					return 11;
				}

				current_map.stageID = (STAGE_ID)response_buffer[offset + 1];

				if (current_map.stageID == STAGE_ID::COMPETITION)
					isStreamer = false;
				else if (current_map.stageID == STAGE_ID::STREAMER_MODE)
					isStreamer = true;

				memcpy(&(current_map.time_preparing), response_buffer + offset + 2, 4);

				offset += 6;

				if (response_size > 8) { //парсинг координат
					for (uint8_t modelSectionID = NULL; modelSectionID < 2; modelSectionID++) {
						if (modelSectionID == 0 && response_buffer[offset] == 0) sect = &(sync_map.modelsSects_creating);
						else if (modelSectionID == 1 && response_buffer[offset] == 1) sect = &(sync_map.modelsSects_deleting);
						else {
							extendedDebugLog("Found unexpected section while synchronizing!\n");

							break;
						}

						offset++;

						uint8_t sections_count = response_buffer[offset];

						offset++;

						if (sections_count > SECTIONS_COUNT) {
							return 12; //проверяем валидность числа секций
						}

						memcpy(&(sync_map.all_models_count), response_buffer + offset, 2);

						offset += 2;

						uint8_t model_type = NULL;
						uint16_t models_count_sect = NULL;

						for (uint16_t i = NULL; i < sections_count; i++) {
							model_type = response_buffer[offset];

							offset++;

							if (model_type >= SECTIONS_COUNT) {
								return 13;  //проверяем валидность типа модели
							}

							memcpy(&models_count_sect, response_buffer + offset, 2);

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
								(MODEL_ID)model_type,
								path_buffer
							};

							//----------------------------------

							model_sect.models.resize(models_count_sect);

							for (uint16_t i = 0; i < models_count_sect; i++) {
								float* coords = new float[3];

								memcpy(coords, response_buffer + offset, 12);
								offset += 12;
								
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
			if (!response_size) {
				return 15;
			}

			memcpy(&length, response_buffer, 2);

			if (length != response_size) {
				return 16;
			}

			offset += 2;

			if (response_size == 3) { //код ошибки
				uint8_t error_code = response_buffer[offset];

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
			else if (response_size >= 9) {
				/*
				Всё прошло успешно.
				Первый байт - ноль для проверки,
				второй байт - 0/1 (СТАРТ / соревнование идет),
				остальные четыре байта - оставшееся время
				*/

				if (response_buffer[offset] != 0) {
					return 17;
				}

				current_map.stageID = (STAGE_ID)response_buffer[offset + 1];

				memcpy(&(current_map.time_preparing), response_buffer + offset + 2, 4);

				offset += 6;

				if (response_buffer[offset] == NULL) return NULL;

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