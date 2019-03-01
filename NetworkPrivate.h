#pragma once
#include "Network.h"


#pragma pack(push)
#pragma pack(1)

struct ReqMain {
    MODS_ID mod_id;    // id мода
    uint8_t map_id;    // id карты
    uint32_t id;       // id игрока
    EVENT_ID event_id; // код события
};

struct ReqMain_DelModel {
    MODS_ID  mod_id;     // id мода
    uint8_t  map_id;     // id карты
    uint32_t id;         // id игрока
    EVENT_ID event_id;   // код события
    MODEL_ID model_id;   // код модели
    float coords_del[3]; // координаты модели, которую следует убрать
};

struct RspMain {
    uint8_t  zero_byte;      // ноль для проверки
    STAGE_ID stage_id;       // 0/1 (СТАРТ / соревнование идет)
    uint32_t time_preparing; // оставшееся время
};

struct RspModelsHeader {
    uint8_t  sections_count; // число секций
    uint16_t minimap_count;  // число моделей (общее)
};

struct RspModelSect {
    MODEL_ID model_id;          // тип модели
    uint16_t models_count_sect; // число моделей в секции
};

#pragma pack(pop)


// проверка на корректность
static_assert( sizeof(ReqMain)          == 7  );
static_assert( sizeof(ReqMain_DelModel) == 20 );
static_assert( sizeof(RspMain)          == 6  );
static_assert( sizeof(RspModelsHeader)  == 3  );
static_assert( sizeof(RspModelSect)     == 3  );
