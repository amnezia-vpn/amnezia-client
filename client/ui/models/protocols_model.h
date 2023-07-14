#ifndef PROTOCOLS_MODEL_H
#define PROTOCOLS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "../controllers/pageController.h"
#include "settings.h"

class ProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ProtocolNameRole = Qt::UserRole + 1,
        ProtocolPageRole,
        RawConfigRole
    };

    ProtocolsModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &content);

    QJsonObject getConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    PageLoader::PageEnum protocolPage(Proto protocol) const;

    std::shared_ptr<Settings> m_settings;

    DockerContainer m_container;
    QJsonObject m_content;
};

#endif // PROTOCOLS_MODEL_H
