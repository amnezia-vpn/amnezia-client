#ifndef AGW_CONFIG_H
#define AGW_CONFIG_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agw/http.h"
#include "agw/types.h"

namespace agw
{

    // Конфигурация долгоживущего клиента. Все секреты/адреса приходят снаружи (ничего не зашито).
    struct Config
    {
        // Формат-строка эндпоинта с одним местом подстановки хоста ("%1"), как в оригинале
        // (endpoint.arg(host)). См. развилку в плане — по умолчанию воспроизводим %1.
        std::string gatewayEndpoint;

        // PEM публичного RSA-ключа шлюза (бывш. PROD/DEV_AGW_PUBLIC_KEY).
        std::string agwPublicKeyPem;

        // S3-эндпоинты для failover (Фаза 3). Базовые адреса, к которым добавляются пути endpoints*.json.
        std::vector<std::string> s3PrimaryEndpoints;
        std::vector<std::string> s3FallbackEndpoints;

        bool isDevEnvironment = false;

        int requestTimeoutMsecs = 12000;     // дефолт из apiConstants.h
        int proxyHealthTimeoutMsecs = 1000;  // как в коде (lmbd-health)
        int proxyStorageTimeoutMsecs = 3000; // как в коде (чтение S3-списка)
        int threadPoolSize = 4;              // Фаза 4

        // Хук перед запросом: SDK сообщает хост (один раз на post, с исходным хостом — прокси не
        // вайтлистятся, паритет). Сюда приложение кладёт kill-switch / iOS requestInetAccess.
        std::function<void(const std::string &host)> onBeforeRequest;

        // Лог-хук (опционален). Секреты и тела SDK не логирует.
        std::function<void(LogLevel, const std::string &message)> log;

        // Транспорт. Если null — берётся makeDefaultHttpClient() (libcurl).
        std::shared_ptr<IHttpClient> httpClient;
    };

} // namespace agw

#endif // AGW_CONFIG_H
