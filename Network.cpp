/**
 * CLIENT-SERVER PART
 */

#include "pch.h"
#include "CConfig.h"
#include "NetworkPrivate.h"
#include "MyLogger.h"
#include <wininet.h>


INIT_LOCAL_MSG_BUFFER;

static std::mutex g_parse_mutex;

static char response_buffer[NET_BUFFER_SIZE];
static size_t response_size = 0;


MyErr curl_init()
{
	/*
	//инициализация curl

	if (curl_global_init(CURL_GLOBAL_ALL)) {
		return_err 1;
	}

	curl_handle = curl_easy_init();

	if (!curl_handle) {
		return_err 2;
	}

	*/
	return_ok;
}


void curl_fini()
{
	/*
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	curl_handle = nullptr;
	*/
}


//writing response from server into array ptr and return size of response
static size_t write_data(char *ptr, size_t size, size_t nmemb, char* data)
{
	/*
	if (data == NULL || response_size + size * nmemb > NET_BUFFER_SIZE) return 0; // Error if out of buffer

	memcpy(&data[response_size], ptr, size*nmemb);// appending data into the end
	response_size += size * nmemb;  // changing position
	return size * nmemb;
	*/
}


static uint8_t send_to_server(std::string_view request)
{
	/*
	if (!curl_handle) {
		return 1;
	}

	response_size = 0;

	curl_httppost* formpost = nullptr;
	curl_httppost* lastptr = nullptr;

	curl_formadd(&formpost, &lastptr,
		CURLFORM_COPYNAME, "request",
		CURLFORM_PTRCONTENTS, request.data(),
		CURLFORM_CONTENTSLENGTH, request.length(),
		CURLFORM_END);

	// setting user agent
	const char* user_agent = "NY_Event";
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent);

	// setting url
	const char* url = "http://api.pavel3333.ru/events/index.php";
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	// setting POST form
	curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, formpost);

	// setting function for write data
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

	// setting buffer
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response_buffer);

	// setting max buffer size
	curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, NET_BUFFER_SIZE);

	// requesting
	CURLcode res = curl_easy_perform(curl_handle);

	if (res != CURLcode::CURLE_OK) {
		debugLogEx(ERROR, "curl error code: %d", res);
	}

	return static_cast<uint8_t>(res);
	*/
	return 0;
}

uint8_t send_token(uint32_t id, uint8_t map_id, EVENT_ID eventID, MODEL_ID modelID, float* coords_del)
{
	std::string_view request_raw;

	// Код наполнения токена по типу события

	switch (eventID) {
		case EVENT_ID::IN_HANGAR:
		case EVENT_ID::IN_BATTLE_GET_FULL:
		case EVENT_ID::IN_BATTLE_GET_SYNC: {
			ReqMain req;

			req.mod_id   = MODS_ID::NY_EVENT; //mod
			req.map_id   = map_id;            //map ID
			req.id       = id;
			req.event_id = eventID;           //код события

			request_raw = std::string_view(
				reinterpret_cast<char*>(&req), sizeof(ReqMain));
			break;
		}
		case EVENT_ID::DEL_LAST_MODEL: {
			if (coords_del == nullptr) {
				return 24;
			}

			ReqMain_DelModel req;

			req.mod_id   = MODS_ID::NY_EVENT; // mod
			req.map_id   = map_id;            // map ID
			req.id       = id;                // id игрока
			req.event_id = eventID;           // код события
			req.model_id = modelID;           // код модели
			std::copy_n(coords_del, 3, req.coords_del);

			request_raw = std::string_view(
				reinterpret_cast<char*>(&req), sizeof(ReqMain_DelModel));
			break;
		}
	}

	//-------------------------------------

	if (request_raw.empty()) {
		return 1;
	}

	uint8_t code = send_to_server(request_raw);

	if (code || !response_size) { //get token
		return 2;
	}

	return 0;
}


uint8_t parse_event_safe(EVENT_ID eventID)
{
	uint8_t res;

	g_parse_mutex.lock();
	if (res = parse_event(eventID)) {
		writeDebugDataToFile(Config::ids, PARSING, response_buffer, response_size);
	} 
	g_parse_mutex.unlock();

	return res;
}

//EVENT_ID::IN_HANGAR
uint8_t parse_event_IN_HANGAR()
{
	uint16_t* length = nullptr;

	if (response_size == 3) {
		length = reinterpret_cast<uint16_t*>(response_buffer);

		//смотрим длину данных
		if (*length == response_size) {
			return response_buffer[2];
		}

		return 1;
	}

	return 2;
}

//EVENT_ID::IN_BATTLE_GET_FULL
uint8_t parse_event_IN_BATTLE_GET_FULL()
{
	uint16_t* length = nullptr;
	uint32_t  offset = NULL;

	uint8_t error_code;

	uint8_t sections_count;
	uint8_t model_type = NULL;
	uint16_t models_count_sect = NULL;

	if (!response_size) {
		return 3;
	}

	length = reinterpret_cast<uint16_t*>(response_buffer);

	if (*length != response_size) {
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

				ModelsFullSection model_sect{
					false,
					(MODEL_ID)model_type,
					path_buffer
				};

				//----------------------------------

				model_sect.models.resize(models_count_sect);

				for (uint16_t j = NULL; j < models_count_sect; j++) {
					float* coords = new float[3];

					for (uint8_t k = NULL; k < 3; k++) {
						memcpy(&coords[k], response_buffer + offset, 4);

						offset += 4;
					}

					model_sect.models[j] = coords;
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
}

//EVENT_ID::IN_BATTLE_GET_SYNC
uint8_t parse_event_IN_BATTLE_GET_SYNC()
{
	uint16_t* length = nullptr;
	uint32_t offset = NULL;

	uint8_t error_code;

	std::vector<ModelsSyncSection>* sect = nullptr;

	if (!response_size) {
		return 9;
	}

	length = reinterpret_cast<uint16_t*>(response_buffer);

	if (*length != response_size) {
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
		    координаты создаваемых моделей:
			  2б  - синхронизационный номер
			  12б - координаты (3*float)
		1 - код удаления моделей
		  1б - число секций для удаления
		  2б - число удаляемых моделей
		    координаты удаляемых моделей:
			  2б  - синхронизационный номер
			  12б - координаты (3*float)
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

					ModelsSyncSection model_sect {
						false,
						(MODEL_ID)model_type,
						path_buffer
					};

					//----------------------------------

					model_sect.models.resize(models_count_sect);

					for (uint16_t j = 0; j < models_count_sect; j++) {
						memcpy(&(model_sect.models[j].syncID), response_buffer + offset, 2);

						offset += 2;

						float* coords = new float[3];

						memcpy(coords, response_buffer + offset, 12);
						offset += 12;

						model_sect.models[j].coords = coords;
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
}

//EVENT_ID::DEL_LAST_MODEL
uint8_t parse_event_DEL_LAST_MODEL()
{
	uint16_t* length = nullptr;

	if (!response_size)
		return 15;

	length = reinterpret_cast<uint16_t*>(response_buffer);
	if (*length != response_size)
		return 16;

	if (response_size == 3) { //код ошибки
		uint8_t error_code = response_buffer[2];

		switch (error_code) {
		case 7:
			current_map.stageID = STAGE_ID::WAITING;
			break;
		case 8:
			current_map.stageID = STAGE_ID::END_BY_TIME;
			break;
		case 9:
			current_map.stageID = STAGE_ID::END_BY_COUNT;
			break;
		}

		return error_code;
	}
	else if (response_size == 9) {
		RspMain* rsp = reinterpret_cast<RspMain*>(response_buffer + 2);

		// Первый байт - ноль для проверки
		if (rsp->zero_byte != 0)
			return 17;

		// второй байт - 0/1 (СТАРТ / соревнование идет)
		current_map.stageID = rsp->stage_id;

		// остальные четыре байта - оставшееся время
		current_map.time_preparing = rsp->time_preparing;

		// успех
		return 0;
	}

	return 19;
}

uint8_t parse_event(EVENT_ID eventID)
{
	//инициализация переменных для каждого события

	//парсинг пакета с сервера

	switch (eventID) {
		case EVENT_ID::IN_HANGAR:          //запрос из ангара       
			return parse_event_IN_HANGAR();
		case EVENT_ID::IN_BATTLE_GET_FULL: // при входе в бой
			return parse_event_IN_BATTLE_GET_FULL();
		case EVENT_ID::IN_BATTLE_GET_SYNC: // синхронизация
			return parse_event_IN_BATTLE_GET_SYNC();
		case EVENT_ID::DEL_LAST_MODEL:     // удаление ближайшей модели
			return parse_event_DEL_LAST_MODEL();
	}

	return 21;
}
