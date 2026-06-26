#ifndef AGW_FAILOVER_PROXY_LIST_H
#define AGW_FAILOVER_PROXY_LIST_H

#include <string>
#include <vector>

#include "agw/types.h"

namespace agw::failover
{

    // Строит список URL S3-хранилищ из базовых адресов и контекста (паритет с appendStorageUrls).
    // Порядок: для service-specific путей — все base из primary, затем generic для всех primary;
    // затем то же для fallback. Перемешивание базовых адресов делает вызывающий ДО вызова
    // (как в оригинале: shuffle(primary)/shuffle(fallback), потом expand).
    //   service-specific: <base> + base64url("endpoints-<service>-<country>") + ".json" (только если service непуст)
    //   generic:          <base> + "endpoints.json"
    std::vector<std::string> buildStorageUrls(const std::vector<std::string> &primaryBaseUrls,
                                              const std::vector<std::string> &fallbackBaseUrls,
                                              const FailoverContext &ctx);

    // Разбирает тело ответа S3 в список прокси-адресов.
    //   dev:  тело — открытый JSON-массив строк;
    //   prod: тело — base64; AES-256-CBC, ключ/IV из SHA-512(pubKeyPem) (hex: [:64]→ключ 32, [64:96]→IV 16).
    // Бросает при сбое расшифровки (вызывающий тогда идёт к следующему хранилищу). JSON, не являющийся
    // массивом, даёт пустой список (не ошибка) — паритет с QJsonDocument::array().
    std::vector<std::string> decodeProxyList(const std::string &body, bool isDevEnvironment,
                                             const std::string &pubKeyPem);

} // namespace agw::failover

#endif // AGW_FAILOVER_PROXY_LIST_H
