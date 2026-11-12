#ifndef SPLITTUNNELINGUPDATEBACKEND_H
#define SPLITTUNNELINGUPDATEBACKEND_H

#include <QJsonObject>
#include <QObject>

#include "splitTunnelingUpdateResult.h"

class SplitTunnelingUpdateBackend : public QObject
{
    Q_OBJECT

public:
    explicit SplitTunnelingUpdateBackend(QObject *parent = nullptr);
    static SplitTunnelingUpdateBackend *create(QObject *parent);
    virtual bool supportsConfiguration(const QJsonObject &configuration) const;
    virtual void apply(quint64 requestId, const QJsonObject &configuration) = 0;

signals:
    void finished(quint64 requestId, SplitTunnelingUpdateResult result);
};

#endif
