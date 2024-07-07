#pragma once
#include "macroexpansion.hpp"

#ifndef _X
    #include <QJsonArray>
    #include <QJsonObject>
    #include <QList>
    #include <QVariant>
#endif

/// macro to define an operator==
#define ___JSONSTRUCT_DEFAULT_COMPARE_IMPL(x) (this->x == ___another___instance__.x) &&
#define JSONSTRUCT_COMPARE(CLASS, ...)                                                                                                               \
    bool operator==(const CLASS &___another___instance__) const                                                                                      \
    {                                                                                                                                                \
        return FOR_EACH(___JSONSTRUCT_DEFAULT_COMPARE_IMPL, __VA_ARGS__) true;                                                                       \
    }

// ============================================================================================
// Load JSON IMPL
#define ___DESERIALIZE_FROM_JSON_CONVERT_B_FUNC_IMPL(name) name::loadJson(___json_object_);
#define ___DESERIALIZE_FROM_JSON_CONVERT_A_FUNC(name) ___DESERIALIZE_FROM_JSON_CONVERT_F_FUNC(name)
#define ___DESERIALIZE_FROM_JSON_CONVERT_B_FUNC(...) FOREACH_CALL_FUNC_3(___DESERIALIZE_FROM_JSON_CONVERT_B_FUNC_IMPL, __VA_ARGS__)
#define ___DESERIALIZE_FROM_JSON_CONVERT_F_FUNC(name)                                                                                                \
    if (___json_object_.toObject().contains(#name))                                                                                                  \
    {                                                                                                                                                \
        JsonStructHelper::Deserialize(this->name, ___json_object_.toObject()[#name]);                                                                \
    }                                                                                                                                                \
    else                                                                                                                                             \
    {                                                                                                                                                \
        this->name = ___qjsonstruct_default_check.name;                                                                                              \
    }

// ============================================================================================
// To JSON IMPL
#define ___SERIALIZE_TO_JSON_CONVERT_F_FUNC(name)                                                                                                    \
    if (!(___qjsonstruct_default_check.name == this->name))                                                                                          \
    {                                                                                                                                                \
        ___json_object_.insert(#name, JsonStructHelper::Serialize(name));                                                                            \
    }
#define ___SERIALIZE_TO_JSON_CONVERT_A_FUNC(name) ___json_object_.insert(#name, JsonStructHelper::Serialize(name));
#define ___SERIALIZE_TO_JSON_CONVERT_B_FUNC_IMPL(name) JsonStructHelper::MergeJson(___json_object_, name::toJson());
#define ___SERIALIZE_TO_JSON_CONVERT_B_FUNC(...) FOREACH_CALL_FUNC_3(___SERIALIZE_TO_JSON_CONVERT_B_FUNC_IMPL, __VA_ARGS__)

// ============================================================================================
// Load JSON Wrapper
#define ___DESERIALIZE_FROM_JSON_CONVERT_FUNC_DECL_A(...) FOREACH_CALL_FUNC_2(___DESERIALIZE_FROM_JSON_CONVERT_A_FUNC, __VA_ARGS__)
#define ___DESERIALIZE_FROM_JSON_CONVERT_FUNC_DECL_F(...) FOREACH_CALL_FUNC_2(___DESERIALIZE_FROM_JSON_CONVERT_F_FUNC, __VA_ARGS__)
#define ___DESERIALIZE_FROM_JSON_CONVERT_FUNC_DECL_B(...) FOREACH_CALL_FUNC_2(___DESERIALIZE_FROM_JSON_CONVERT_B_FUNC, __VA_ARGS__)
#define ___DESERIALIZE_FROM_JSON_EXTRACT_B_F(name_option) ___DESERIALIZE_FROM_JSON_CONVERT_FUNC_DECL_##name_option

// ============================================================================================
// To JSON Wrapper
#define ___SERIALIZE_TO_JSON_CONVERT_FUNC_DECL_A(...) FOREACH_CALL_FUNC_2(___SERIALIZE_TO_JSON_CONVERT_A_FUNC, __VA_ARGS__)
#define ___SERIALIZE_TO_JSON_CONVERT_FUNC_DECL_F(...) FOREACH_CALL_FUNC_2(___SERIALIZE_TO_JSON_CONVERT_F_FUNC, __VA_ARGS__)
#define ___SERIALIZE_TO_JSON_CONVERT_FUNC_DECL_B(...) FOREACH_CALL_FUNC_2(___SERIALIZE_TO_JSON_CONVERT_B_FUNC, __VA_ARGS__)
#define ___SERIALIZE_TO_JSON_EXTRACT_B_F(name_option) ___SERIALIZE_TO_JSON_CONVERT_FUNC_DECL_##name_option

// ============================================================================================
#define JSONSTRUCT_REGISTER_NOCOPYMOVE(___class_type_, ...)                                                                                          \
    void loadJson(const QJsonValue &___json_object_)                                                                                                 \
    {                                                                                                                                                \
        ___class_type_ ___qjsonstruct_default_check;                                                                                                 \
        FOREACH_CALL_FUNC(___DESERIALIZE_FROM_JSON_EXTRACT_B_F, __VA_ARGS__);                                                                        \
    }                                                                                                                                                \
    [[nodiscard]] const QJsonObject toJson() const                                                                                                   \
    {                                                                                                                                                \
        ___class_type_ ___qjsonstruct_default_check;                                                                                                 \
        QJsonObject ___json_object_;                                                                                                                 \
        FOREACH_CALL_FUNC(___SERIALIZE_TO_JSON_EXTRACT_B_F, __VA_ARGS__);                                                                            \
        return ___json_object_;                                                                                                                      \
    }

#define JSONSTRUCT_REGISTER(___class_type_, ...)                                                                                                     \
    JSONSTRUCT_REGISTER_NOCOPYMOVE(___class_type_, __VA_ARGS__);                                                                                     \
    [[nodiscard]] static auto fromJson(const QJsonValue &___json_object_)                                                                            \
    {                                                                                                                                                \
        ___class_type_ _t;                                                                                                                           \
        _t.loadJson(___json_object_);                                                                                                                \
        return _t;                                                                                                                                   \
    }

#define ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(type, convert_func)                                                                                \
    static void Deserialize(type &t, const QJsonValue &d)                                                                                            \
    {                                                                                                                                                \
        t = d.convert_func;                                                                                                                          \
    }

class JsonStructHelper
{
  public:
    static void MergeJson(QJsonObject &mergeTo, const QJsonObject &mergeIn)
    {
        for (const auto &key : mergeIn.keys())
            mergeTo[key] = mergeIn.value(key);
    }
    //
    template<typename T>
    static void Deserialize(T &t, const QJsonValue &d)
    {
        if constexpr (std::is_enum_v<T>)
            t = (T) d.toInt();
        else if constexpr (std::is_same_v<T, QJsonObject>)
            t = d.toObject();
        else if constexpr (std::is_same_v<T, QJsonArray>)
            t = d.toArray();
        else
            t.loadJson(d);
    }

    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(QString, toString());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(QChar, toVariant().toChar());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(std::string, toString().toStdString());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(std::wstring, toString().toStdWString());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(bool, toBool());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(double, toDouble());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(float, toVariant().toFloat());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(int, toInt());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(long, toVariant().toLongLong());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(long long, toVariant().toLongLong());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(unsigned int, toVariant().toUInt());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(unsigned long, toVariant().toULongLong());
    ___DECL_JSON_STRUCT_LOAD_SIMPLE_TYPE_FUNC(unsigned long long, toVariant().toULongLong());

    template<typename T>
    static void Deserialize(QList<T> &t, const QJsonValue &d)
    {
        t.clear();
        for (const auto &val : d.toArray())
        {
            T data;
            Deserialize(data, val);
            t.push_back(data);
        }
    }

    template<typename TKey, typename TValue>
    static void Deserialize(QMap<TKey, TValue> &t, const QJsonValue &d)
    {
        t.clear();
        const auto &jsonObject = d.toObject();
        TKey keyVal;
        TValue valueVal;
        for (const auto &key : jsonObject.keys())
        {
            Deserialize(keyVal, key);
            Deserialize(valueVal, jsonObject.value(key));
            t.insert(keyVal, valueVal);
        }
    }

    // =========================== Store Json Data ===========================

    template<typename T>
    static QJsonValue Serialize(const T &t)
    {
        if constexpr (std::is_enum_v<T>)
            return (int) t;
        else if constexpr (std::is_same_v<T, QJsonObject> || std::is_same_v<T, QJsonArray>)
            return t;
        else
            return t.toJson();
    }

#define pure_func(x) (x)
#define ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(type)                                                                                             \
    static QJsonValue Serialize(const type &t)                                                                                                       \
    {                                                                                                                                                \
        return QJsonValue(t);                                                                                                                        \
    }
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(int);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(bool);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(QJsonArray);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(QJsonObject);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(QString);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(long long);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(float);
    ___DECL_JSON_STRUCT_STORE_SIMPLE_TYPE_FUNC(double);

#define ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(type, func)                                                                                      \
    static QJsonValue Serialize(const type &t)                                                                                                       \
    {                                                                                                                                                \
        return QJsonValue::fromVariant(func);                                                                                                        \
    }

    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(std::string, QString::fromStdString(t))
    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(std::wstring, QString::fromStdWString(t))
    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(long, QVariant::fromValue<long>(t))
    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(unsigned int, QVariant::fromValue<unsigned int>(t))
    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(unsigned long, QVariant::fromValue<unsigned long>(t))
    ___DECL_JSON_STRUCT_STORE_VARIANT_TYPE_FUNC(unsigned long long, QVariant::fromValue<unsigned long long>(t))

    template<typename TValue>
    static QJsonValue Serialize(const QMap<QString, TValue> &t)
    {
        QJsonObject mapObject;
        for (const auto &key : t.keys())
        {
            auto valueVal = Serialize(t.value(key));
            mapObject.insert(key, valueVal);
        }
        return mapObject;
    }

    template<typename T>
    static QJsonValue Serialize(const QList<T> &t)
    {
        QJsonArray listObject;
        for (const auto &item : t)
        {
            listObject.push_back(Serialize(item));
        }
        return listObject;
    }
};
