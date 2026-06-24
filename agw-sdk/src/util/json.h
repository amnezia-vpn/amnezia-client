#ifndef AGW_UTIL_JSON_H
#define AGW_UTIL_JSON_H

#include <string>

#include <nlohmann/json.hpp>

namespace agw::util {

// Тип JSON внутри SDK. Используем дефолтный nlohmann::json: его объект — отсортированный по ключам
// (std::map), что совпадает с QJsonObject для ASCII-ключей.
using Json = nlohmann::json;

// Сериализация байт-в-байт как QJsonDocument(obj).toJson() в режиме Indented:
//  - отступ 4 пробела на уровень;
//  - перенос строки '\n' после каждого элемента и в конце документа;
//  - "ключ": значение (пробел после двоеточия);
//  - ключи отсортированы (это уже обеспечивает nlohmann::json);
//  - экранирование строк по правилам Qt (\b \f \n \r \t, \u00XX для прочих control, " и \\,
//    остальное — как UTF-8).
// Числа форматируются best-effort (целые — точно); полный паритет double — задача Тир 2,
// в транспорте v1 числа в собираемом SDK JSON не встречаются (json_keys и body — только строки).
std::string qtIndentedDump(const Json &j);

} // namespace agw::util

#endif // AGW_UTIL_JSON_H
