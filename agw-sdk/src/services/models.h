#ifndef AGW_SERVICES_MODELS_H
#define AGW_SERVICES_MODELS_H

#include "agw/models.h"
#include "util/json.h"

namespace agw::services {

// Парсинг ответа в публичную модель ApiConfig (паритет с amnezia::ApiConfig::fromJson).
ApiConfig parseApiConfig(const util::Json &json);

} // namespace agw::services

#endif // AGW_SERVICES_MODELS_H
