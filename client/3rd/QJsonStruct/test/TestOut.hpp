#pragma once
#include "QJsonStruct.hpp"

struct SubData
{
    QString subString;
    JSONSTRUCT_REGISTER_TOJSON(subString)
};

struct ToJsonOnlyData
{
    QString x;
    int y;
    int z;
    QList<int> ints;
    SubData sub;
    QMap<QString, SubData> subs;
    JSONSTRUCT_REGISTER_TOJSON(x, y, z, sub, ints, subs)
};
