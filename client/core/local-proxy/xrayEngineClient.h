#ifndef XRAYENGINECLIENT_H
#define XRAYENGINECLIENT_H

#include <QString>

class XrayEngineClient
{
public:
    ~XrayEngineClient();

    bool start(const QString &configJson);
    bool stop();
    bool isRunning() const;
    QString lastError() const;

private:
    // Owner token of the shared xray engine (see Xray::start in the service).
    // 0 means this client doesn't own a running engine.
    qint64 m_token = 0;
    QString m_lastError;
};

#endif // XRAYENGINECLIENT_H
