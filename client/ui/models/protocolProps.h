#ifndef PROTOCOLPROPS_H
#define PROTOCOLPROPS_H

#include <QObject>
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"

class ProtocolProps : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolProps(QObject *parent = nullptr) : QObject(parent) {}
};

#endif // PROTOCOLPROPS_H

