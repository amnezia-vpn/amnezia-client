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
    bool m_isRunning = false;
    QString m_lastError;
};

#endif // XRAYENGINECLIENT_H
