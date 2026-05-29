#ifndef MTPROXY_PUBLIC_HOST_INPUT_H
#define MTPROXY_PUBLIC_HOST_INPUT_H

#include <QString>

#include <QValidator>

/// Shared rules for public host field (IPv4 dotted partial, IPv6 hex, FQDN ASCII).
bool mtproxyPublicHostInputAllowed(const QString &text);

class PublicHostInputValidator : public QValidator {
Q_OBJECT

public:
    explicit PublicHostInputValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const override;
};

#endif
