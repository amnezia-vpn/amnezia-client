#ifndef IGATEWAYTRANSPORT_H
#define IGATEWAYTRANSPORT_H

#include <QByteArray>
#include <QString>
#include <functional>

#include "core/defs.h"

namespace amnezia::transport
{

struct DecryptionResult
{
    QByteArray decrypted;
    bool isOk = false;
};

using DecryptionHook = std::function<DecryptionResult(const QByteArray &encrypted)>;

class IGatewayTransport
{
public:
    virtual ~IGatewayTransport() = default;

    virtual QString name() const = 0;

    virtual amnezia::ErrorCode send(const QString &endpointTemplate,
                                    const QByteArray &requestBody,
                                    QByteArray &decryptedResponse,
                                    const DecryptionHook &decryptionHook) = 0;
};

} // namespace amnezia::transport

#endif // IGATEWAYTRANSPORT_H
