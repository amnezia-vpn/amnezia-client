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
        QString subsubregionId;
        QString nameEn;
        QString nameRu;
        QString city;
        QStringList aliases;
    };

    enum class SplitMode {
        Auto,
        Always,
        Never
    };

    struct Subsubregion
    {
        QString id;
        int order = 0;
    };

    struct Subregion
    {
        QString id;
        int order = 0;
        SplitMode split = SplitMode::Auto;
        QVector<Subsubregion> subsubregions;
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

        bool isSplit(const QString &regionId, int visibleCount) const;
        bool isSplit(const QString &regionId, const QString &subregionId, int visibleCount) const;

    private:
        QVector<Region> m_regions;
        QVector<UseCase> m_useCases;
        QHash<QString, Entry> m_byCode;
        QHash<QString, Entry> m_byIso;
        int m_splitThreshold = 15;
        int m_version = 0;
    };
} // namespace countryCatalog

#endif // COUNTRYCATALOG_H
