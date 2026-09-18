#ifndef COUNTRYCATALOG_H
#define COUNTRYCATALOG_H

#include <QByteArray>
#include <QHash>
#include <QLatin1String>
#include <QString>
#include <QStringList>
#include <QVector>

namespace countryCatalog
{
    constexpr QLatin1String otherRegionId("other");

    struct Entry
    {
        QString code;
        QString isoCode;
        QString regionId;
        QString subregionId;
        QString nameEn;
        QString nameRu;
        QString city;
        QStringList aliases;
        bool countsTowardSplit = true;
    };

    struct Subregion
    {
        QString id;
        int order = 0;
    };

    struct Region
    {
        QString id;
        int order = 0;
        QVector<Subregion> subregions;
    };

    class Catalog
    {
    public:
        Catalog() = default;

        static Catalog fromJson(const QByteArray &json);
        static Catalog bundled();

        bool isEmpty() const;
        int splitThreshold() const;
        const QVector<Region> &regions() const;

        const Entry *find(const QString &countryCode, const QString &isoCode) const;
        bool hasSubregions(const QString &regionId) const;

    private:
        QVector<Region> m_regions;
        QHash<QString, Entry> m_byCode;
        QHash<QString, Entry> m_byIso;
        int m_splitThreshold = 15;
    };
} // namespace countryCatalog

#endif // COUNTRYCATALOG_H
