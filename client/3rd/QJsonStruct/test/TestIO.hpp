#pragma once
#include "QJsonStruct.hpp"
#ifndef _X
    #include <QList>
    #include <QString>
    #include <QStringList>
#endif

struct BaseStruct
{
    QString baseStr;
    int o;
    JSONSTRUCT_REGISTER(BaseStruct, F(baseStr, o))
};

struct BaseStruct2
{
    QString baseStr2;
    int o2;
    JSONSTRUCT_REGISTER(BaseStruct, F(baseStr2, o2))
};
struct TestInnerStruct
    : BaseStruct
    , BaseStruct2
{
    QJsonObject jobj;
    QJsonArray jarray;
    QString str;
    JSONSTRUCT_REGISTER(TestInnerStruct, B(BaseStruct, BaseStruct2), F(str, jobj, jarray))
};

struct JsonIOTest
{
    QString str;
    QList<int> listOfNumber;
    QList<bool> listOfBool;
    QList<QString> listOfString;
    QList<QList<QString>> listOfListOfString;

    QMap<QString, QString> map;
    TestInnerStruct inner;

    JSONSTRUCT_REGISTER(JsonIOTest, F(str, listOfNumber, listOfBool, listOfString, listOfListOfString, map, inner));
    JsonIOTest(){};
};
