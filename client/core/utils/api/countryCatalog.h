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

    enum class SplitMode {
        Auto,
        Always,
        Never
    };

    struct Region
    {
        QString id;
        int order = 0;
        SplitMode split = SplitMode::Auto;
        QVector<Subregion> subregions;
    };

    struct UseCase
    {
        QString id;
        int order = 0;
        QStringList locationIds;
    };

    class Catalog
    {
    public:
        Catalog() = default;

        static Catalog fromJson(const QByteArray &json);
        static Catalog bundled();

        bool isEmpty() const;
        int splitThreshold() const;
        int version() const;
        const QVector<Region> &regions() const;
        const QVector<UseCase> &useCases() const;

        const Entry *find(const QString &countryCode, const QString &isoCode) const;
        bool hasSubregions(const QString &regionId) const;

        bool isSplit(const QString &regionId) const;

    private:
        QVector<Region> m_regions;
        QVector<UseCase> m_useCases;
        QHash<QString, Entry> m_byCode;
        QHash<QString, Entry> m_byIso;
        QHash<QString, int> m_catalogCounts;
        int m_splitThreshold = 15;
        int m_version = 0;
    };
} // namespace countryCatalog

#endif // COUNTRYCATALOG_H
