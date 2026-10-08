#include "countryCatalog.h"

#include <limits>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "logger.h"

namespace
{
    Logger logger("CountryCatalog");

    constexpr QLatin1String bundledPath(":/countries/regions.json");

    constexpr QLatin1String keyVersion("version");
    constexpr QLatin1String keySplitThreshold("splitThreshold");
    constexpr QLatin1String keyCollapseThreshold("collapseThreshold");
    constexpr QLatin1String keyRegions("regions");
    constexpr QLatin1String keySubregions("subregions");
    constexpr QLatin1String keyCountries("countries");
    constexpr QLatin1String keyId("id");
    constexpr QLatin1String keyOrder("order");
    constexpr QLatin1String keyCode("code");
    constexpr QLatin1String keyIso("iso");
    constexpr QLatin1String keyRegion("region");
    constexpr QLatin1String keySubregion("subregion");
    constexpr QLatin1String keySubsubregion("subsubregion");
    constexpr QLatin1String keyNameEn("nameEn");
    constexpr QLatin1String keyNameRu("nameRu");
    constexpr QLatin1String keyCity("city");
    constexpr QLatin1String keyAliases("aliases");
    constexpr QLatin1String keySplit("split");
    constexpr QLatin1String keyUseCases("useCases");
    constexpr QLatin1String keyLocationIds("locationIds");
    constexpr QLatin1String keyNameLocalized("nameLocalized");

    constexpr QLatin1String splitAlways("always");
    constexpr QLatin1String splitNever("never");

    countryCatalog::SplitMode parseSplit(const QJsonObject &object)
    {
        const QString split = object.value(keySplit).toString();
        if (split == splitAlways) {
            return countryCatalog::SplitMode::Always;
        }
        if (split == splitNever) {
            return countryCatalog::SplitMode::Never;
        }
        return countryCatalog::SplitMode::Auto;
    }
}

namespace countryCatalog
{

Catalog Catalog::fromJson(const QByteArray &json)
{
    Catalog catalog;

    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        logger.error() << "failed to parse the country catalog:" << parseError.errorString();
        return catalog;
    }

    const QJsonObject root = document.object();
    catalog.m_version = root.value(keyVersion).toInt();
    if (root.contains(keySplitThreshold)) {
        catalog.m_splitThreshold = root.value(keySplitThreshold).toInt(catalog.m_splitThreshold);
    }
    if (root.contains(keyCollapseThreshold)) {
        catalog.m_collapseThreshold = root.value(keyCollapseThreshold).toInt(catalog.m_collapseThreshold);
    }

    const QJsonArray regions = root.value(keyRegions).toArray();
    for (const QJsonValue &value : regions) {
        const QJsonObject regionObject = value.toObject();

        Region region;
        region.id = regionObject.value(keyId).toString();
        if (region.id.isEmpty()) {
            continue;
        }
        region.order = regionObject.value(keyOrder).toInt();
        region.split = parseSplit(regionObject);

        const QJsonArray subregions = regionObject.value(keySubregions).toArray();
        for (const QJsonValue &subregionValue : subregions) {
            const QJsonObject subregionObject = subregionValue.toObject();

            Subregion subregion;
            subregion.id = subregionObject.value(keyId).toString();
            if (subregion.id.isEmpty()) {
                continue;
            }
            subregion.order = subregionObject.value(keyOrder).toInt();
            subregion.split = parseSplit(subregionObject);

            const QJsonArray subsubregions = subregionObject.value(keySubregions).toArray();
            for (const QJsonValue &subsubregionValue : subsubregions) {
                const QJsonObject subsubregionObject = subsubregionValue.toObject();

                Subsubregion subsubregion;
                subsubregion.id = subsubregionObject.value(keyId).toString();
                if (subsubregion.id.isEmpty()) {
                    continue;
                }
                subsubregion.order = subsubregionObject.value(keyOrder).toInt();
                subregion.subsubregions.push_back(subsubregion);
            }

            region.subregions.push_back(subregion);
        }

        catalog.m_regions.push_back(region);
    }

    const QJsonArray countries = root.value(keyCountries).toArray();
    for (const QJsonValue &value : countries) {
        const QJsonObject countryObject = value.toObject();

        Entry entry;
        entry.code = countryObject.value(keyCode).toString();
        if (entry.code.isEmpty()) {
            continue;
        }
        entry.isoCode = countryObject.value(keyIso).toString(entry.code).toUpper();
        entry.regionId = countryObject.value(keyRegion).toString();
        entry.subregionId = countryObject.value(keySubregion).toString();
        entry.subsubregionId = countryObject.value(keySubsubregion).toString();
        entry.nameEn = countryObject.value(keyNameEn).toString();
        entry.nameRu = countryObject.value(keyNameRu).toString();
        entry.city = countryObject.value(keyCity).toString();

        const QJsonArray aliases = countryObject.value(keyAliases).toArray();
        for (const QJsonValue &aliasValue : aliases) {
            const QString alias = aliasValue.toString();
            if (!alias.isEmpty()) {
                entry.aliases.push_back(alias);
            }
        }

        catalog.m_byCode.insert(entry.code.toUpper(), entry);
        if (!catalog.m_byIso.contains(entry.isoCode)) {
            catalog.m_byIso.insert(entry.isoCode, entry);
        }

    }

    const QJsonArray useCases = root.value(keyUseCases).toArray();
    for (const QJsonValue &value : useCases) {
        const QJsonObject useCaseObject = value.toObject();

        UseCase useCase;
        useCase.id = useCaseObject.value(keyId).toString();
        if (useCase.id.isEmpty()) {
            continue;
        }
        useCase.order = useCaseObject.value(keyOrder).toInt();

        const QJsonObject names = useCaseObject.value(keyNameLocalized).toObject();
        for (auto it = names.constBegin(); it != names.constEnd(); ++it) {
            const QString name = it.value().toString().trimmed();
            if (!name.isEmpty()) {
                useCase.nameLocalized.insert(it.key(), name);
            }
        }

        const QJsonArray locationIds = useCaseObject.value(keyLocationIds).toArray();
        for (const QJsonValue &locationId : locationIds) {
            const QString id = locationId.toString();
            if (!id.isEmpty()) {
                useCase.locationIds.push_back(id);
            }
        }
        catalog.m_useCases.push_back(useCase);
    }

    logger.info() << "loaded" << catalog.m_regions.size() << "regions and"
                  << catalog.m_byCode.size() << "locations, catalog version"
                  << catalog.m_version;

    return catalog;
}

Catalog Catalog::bundled()
{
    QFile file(bundledPath);
    if (!file.open(QIODevice::ReadOnly)) {
        logger.error() << "the bundled country catalog is missing at" << bundledPath;
        return {};
    }
    return fromJson(file.readAll());
}

bool Catalog::isEmpty() const
{
    return m_regions.isEmpty() || m_byCode.isEmpty();
}

int Catalog::splitThreshold() const
{
    return m_splitThreshold;
}

int Catalog::collapseThreshold() const
{
    return m_collapseThreshold;
}

bool Catalog::splitByCount(int visibleCount, bool wasSplit) const
{
    if (visibleCount > m_splitThreshold) {
        return true;
    }
    if (visibleCount < m_collapseThreshold) {
        return false;
    }
    return wasSplit;
}

int Catalog::version() const
{
    return m_version;
}

const QVector<Region> &Catalog::regions() const
{
    return m_regions;
}

const QVector<UseCase> &Catalog::useCases() const
{
    return m_useCases;
}

const Entry *Catalog::find(const QString &countryCode, const QString &isoCode) const
{
    const auto byCode = m_byCode.constFind(countryCode.toUpper());
    if (byCode != m_byCode.constEnd()) {
        return &byCode.value();
    }

    const auto byIso = m_byIso.constFind(isoCode.toUpper());
    if (byIso != m_byIso.constEnd()) {
        return &byIso.value();
    }

    return nullptr;
}



bool Catalog::hasSubregions(const QString &regionId) const
{
    for (const Region &region : m_regions) {
        if (region.id == regionId) {
            return !region.subregions.isEmpty();
        }
    }
    return false;
}

bool Catalog::isSplit(const QString &regionId, int visibleCount, bool wasSplit) const
{
    for (const Region &region : m_regions) {
        if (region.id != regionId) {
            continue;
        }
        if (region.subregions.isEmpty() || region.split == SplitMode::Never) {
            return false;
        }
        if (region.split == SplitMode::Always) {
            return true;
        }
        return splitByCount(visibleCount, wasSplit);
    }
    return false;
}

bool Catalog::isSplit(const QString &regionId, const QString &subregionId, int visibleCount, bool wasSplit) const
{
    for (const Region &region : m_regions) {
        if (region.id != regionId) {
            continue;
        }
        for (const Subregion &subregion : region.subregions) {
            if (subregion.id != subregionId) {
                continue;
            }
            if (subregion.subsubregions.isEmpty() || subregion.split == SplitMode::Never) {
                return false;
            }
            if (subregion.split == SplitMode::Always) {
                return true;
            }
            return splitByCount(visibleCount, wasSplit);
        }
        return false;
    }
    return false;
}

}
