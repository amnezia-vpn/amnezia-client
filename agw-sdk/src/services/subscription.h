#ifndef AGW_SERVICES_SUBSCRIPTION_H
#define AGW_SERVICES_SUBSCRIPTION_H

#include <ctime>
#include <string>

namespace agw::services {

// Парсинг ISO-8601 даты (…Z, ±hh:mm или без зоны=UTC) в epoch-секунды UTC.
// Возвращает false, если строка пустая или не распарсилась.
bool parseIso8601Utc(const std::string &iso, std::time_t &outEpoch);

// Паритет с apiUtils::isSubscriptionExpired / isSubscriptionExpiringSoon.
// Время "сейчас" инъектируется (nowUtc) — для детерминизма и Qt-free. Пустая/битая дата → false.
bool isSubscriptionExpired(const std::string &endDateIso, std::time_t nowUtc);
bool isSubscriptionExpiringSoon(const std::string &endDateIso, std::time_t nowUtc, int withinDays = 30);

} // namespace agw::services

#endif // AGW_SERVICES_SUBSCRIPTION_H
