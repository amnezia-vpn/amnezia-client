#include "QJsonIO.hpp"
#include "QJsonStruct.hpp"
#include "TestIO.hpp"
#include "TestOut.hpp"

#include <QCoreApplication>
#include <QJsonDocument>
#include <iostream>

int main(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    {
        ToJsonOnlyData data;
        data.x = "1string";
        data.y = 2;
        data.ints << 0;
        data.ints << 100;
        data.ints << 900;
        data.sub.subString = "subs";
        data.subs["subs-1"] = { "subs1-data" };
        data.subs["subs-2"] = { "subs2-data" };
        data.subs["subs-3"] = { "subs3-data" };
        data.z = 3;
        auto x = data.toJson();
        std::cout << QJsonDocument(x).toJson().toStdString() << std::endl;
    }
    //
    {
        auto f = JsonIOTest::fromJson( //
            QJsonObject{
                { "inner", QJsonObject{ { "str", "innerString" },                                 //
                                        { "jobj", QJsonObject{ { "key", "value" } } },            //
                                        { "jarray", QJsonArray{ "array0", "array1", "array2" } }, //
                                        { "baseStr", "baseInnerString" } } },                     //
                { "str", "data1" },                                                               //
                { "map", QJsonObject{ { "mapStr", "mapData" } } },                                //
                { "listOfString", QJsonArray{ "1", "2", "3", "4", "5" } },                        //
                { "listOfNumber", QJsonArray{ 1, 2, 3, 4, 5 } },                                  //
                { "listOfBool", QJsonArray{ true, false, false, true, true } },                   //
                { "listOfListOfString", QJsonArray{ QJsonArray{ "1" },                            //
                                                    QJsonArray{ "1", "2" },                       //
                                                    QJsonArray{ "1", "2", "3" },                  //
                                                    QJsonArray{ "1", "2", "3", "4" },             //
                                                    QJsonArray{ "1", "2", "3", "4", "5" } } },    //
            });
        auto x = f.toJson();
        std::cout << QJsonDocument(x).toJson().toStdString() << std::endl;
    }
    {
        QJsonObject obj{
            { "inner", QJsonObject{ { "str", "innerString" }, { "baseStr", "baseInnerString" } } }, //
            { "str", "data1" },                                                                     //
            { "map", QJsonObject{ { "mapStr", "mapData" } } },                                      //
            { "listOfString", QJsonArray{ "1", "2", "3", "4", "5" } },                              //
            { "listOfNumber", QJsonArray{ 1, 2, 3, 4, 5 } },                                        //
            { "listOfBool", QJsonArray{ true, false, false, true, true } },                         //
            { "listOfListOfString", QJsonArray{ QJsonArray{ "1" },                                  //
                                                QJsonArray{ "1", "2" },                             //
                                                QJsonArray{ "1", "2", "3" },                        //
                                                QJsonArray{ "1", "2", "3", "4" },                   //
                                                QJsonArray{ "1", "2", "3", "4", "5" } } },          //
        };
        auto y = QJsonIO::GetValue(obj, std::tuple{ "listOfListOfString", 2 });
        y.toObject();
    }
    return 0;
}
