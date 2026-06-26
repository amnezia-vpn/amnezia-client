#include "services/subscription.h"

#include <cstdio>

namespace agw::services {

namespace {

// timegm нет в стандарте переносимо — считаем epoch для UTC вручную (Гауссова формула дней).
std::time_t toEpochUtc(int y, int mon, int d, int h, int mi, int s)
{
    // дни от эпохи Unix до начала года/месяца (алгоритм «days from civil»).
    y -= (mon <= 2);
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (mon + (mon > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + static_cast<long long>(doe) - 719468;
    return static_cast<std::time_t>(days * 86400LL + h * 3600 + mi * 60 + s);
}

} // namespace

bool parseIso8601Utc(const std::string &iso, std::time_t &outEpoch)
{
    if (iso.empty()) {
        return false;
    }
    int y = 0, mon = 0, d = 0, h = 0, mi = 0, s = 0;
    // YYYY-MM-DDThh:mm:ss (разделитель T или пробел); дробь/зона разбираются ниже.
    if (std::sscanf(iso.c_str(), "%4d-%2d-%2d%*c%2d:%2d:%2d", &y, &mon, &d, &h, &mi, &s) < 6) {
        // допускаем дату без времени: YYYY-MM-DD
        if (std::sscanf(iso.c_str(), "%4d-%2d-%2d", &y, &mon, &d) < 3) {
            return false;
        }
    }
    if (mon < 1 || mon > 12 || d < 1 || d > 31) {
        return false;
    }

    std::time_t epoch = toEpochUtc(y, mon, d, h, mi, s);

    // смещение зоны: ищем '+'/'-' после позиции времени (не минус в дате).
    const std::size_t tpos = iso.find_first_of("Tt ");
    if (tpos != std::string::npos) {
        const std::size_t z = iso.find_first_of("+-", tpos);
        if (z != std::string::npos) {
            int oh = 0, om = 0;
            if (std::sscanf(iso.c_str() + z + 1, "%2d:%2d", &oh, &om) >= 1) {
                const int sign = (iso[z] == '-') ? -1 : 1;
                epoch -= sign * (oh * 3600 + om * 60); // приводим к UTC
            }
        }
    }
    outEpoch = epoch;
    return true;
}

bool isSubscriptionExpired(const std::string &endDateIso, std::time_t nowUtc)
{
    std::time_t end = 0;
    if (!parseIso8601Utc(endDateIso, end)) {
        return false;
    }
    return end <= nowUtc;
}

bool isSubscriptionExpiringSoon(const std::string &endDateIso, std::time_t nowUtc, int withinDays)
{
    std::time_t end = 0;
    if (!parseIso8601Utc(endDateIso, end)) {
        return false;
    }
    if (end <= nowUtc) {
        return false;
    }
    return end <= nowUtc + static_cast<std::time_t>(withinDays) * 86400;
}

} // namespace agw::services
