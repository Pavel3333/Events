#include "API_functions.h"
#include "curl/curl.h"
#include <fstream>
#include <sstream>
#include "MyLogger.h"

#undef debug_log
#define debug_log false

uint32_t curl_init();
void     curl_clean();

uint8_t send_token(uint32_t, uint8_t, EVENT_ID, MODEL_ID, float* coords_del = nullptr);

uint8_t parse_event(EVENT_ID);

char* get_response_data();