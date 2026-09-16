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
        QString code;        //!< catalog key: ISO code ("GB") or multi-location code ("us-east")
        QString isoCode;     //!< country ISO code, used for the flag and for exact-match search
        QString regionId;
        QString subregionId; //!< empty when the region is not split
        QString nameEn;
        QString nameRu;
        QString city;        //!< qualifier for multi-location countries: East, West, Montreal
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

        int regionOrder(const QString &regionId) const;
        int subregionOrder(const QString &regionId, const QString &subregionId) const;
        bool hasSubregions(const QString &regionId) const;

    private:
        QVector<Region> m_regions;
        QHash<QString, Entry> m_byCode;   //!< upper-cased catalog key
        QHash<QString, Entry> m_byIso;    //!< first entry seen per ISO code
        int m_splitThreshold = 15;
    };
} // namespace countryCatalog

#endif // COUNTRYCATALOG_H
