#include "API_functions.h"

uint32_t curl_init();
void     curl_clean();

uint8_t send_token(uint32_t, uint8_t, EVENT_ID, MODEL_ID, float* coords_del = nullptr);

uint8_t parse_event_safe(EVENT_ID eventID);
uint8_t parse_event(EVENT_ID);
