#pragma once
#include "Network.h"


#pragma pack(push)
#pragma pack(1)

// придумать имена структурам:

struct ReqPacket7b {
    MODS_ID mod_id;    // id-мода
    uint8_t map_id;    // id-карты
    uint32_t id;       // ?
    EVENT_ID event_id; // код события

	// пока так
	char _zero;
};

struct ReqPacket20b {
    MODS_ID  mod_id;     // id-мода
    uint8_t  map_id;     // id-карты
    uint32_t id;         // ?
    EVENT_ID event_id;   // код события
    MODEL_ID model_id;   // код модели
    float coords_del[3]; // ?

	// пока так
	char _zero;
};

struct RspPacket6b {
    uint8_t  zero_byte;      // ноль для проверки
    STAGE_ID stage_id;       // 0/1 (СТАРТ / соревнование идет)
    uint32_t time_preparing; // оставшееся время
};

struct RspPacket3b_sect {
    uint8_t  sections_count; // число секций
    uint16_t minimap_count;  // ?
};

struct RspPacket3b_model {
    MODEL_ID model_id;          // тип модели
    uint16_t models_count_sect; // ?
};


#pragma pack(pop)


// проверка на корректность
static_assert( sizeof(ReqPacket7b)       == 8  );
static_assert( sizeof(ReqPacket20b)      == 21 );
static_assert( sizeof(RspPacket6b)       == 6  );
static_assert( sizeof(RspPacket3b_sect)  == 3  );
static_assert( sizeof(RspPacket3b_model) == 3  );
