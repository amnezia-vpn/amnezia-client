#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <tuple>

enum class QJsonIOPathType
{
    JSONIO_MODE_ARRAY,
    JSONIO_MODE_OBJECT
};

typedef QPair<QString, QJsonIOPathType> QJsonIONodeType;

struct QJsonIOPath : QList<QJsonIONodeType>
{
    template<typename type1, typename type2, typename... types>
    QJsonIOPath(const type1 t1, const type2 t2, const types... ts)
    {
        AppendPath(t1);
        AppendPath(t2);
        (AppendPath(ts), ...);
    }

    void AppendPath(size_t index)
    {
        append({ QString::number(index), QJsonIOPathType::JSONIO_MODE_ARRAY });
    }

    void AppendPath(const QString &key)
    {
        append({ key, QJsonIOPathType::JSONIO_MODE_OBJECT });
    }

    template<typename t>
    QJsonIOPath &operator<<(const t &str)
    {
        AppendPath(str);
        return *this;
    }

    template<typename t>
    QJsonIOPath &operator+=(const t &val)
    {
        AppendPath(val);
        return *this;
    }

    QJsonIOPath &operator<<(const QJsonIOPath &other)
    {
        for (const auto &x : other)
            this->append(x);
        return *this;
    }

    template<typename t>
    QJsonIOPath &operator<<(const t &val) const
    {
        auto _new = *this;
        return _new << val;
    }

    template<typename t>
    QJsonIOPath operator+(const t &val) const
    {
        auto _new = *this;
        return _new << val;
    }

    QJsonIOPath operator+(const QJsonIOPath &other) const
    {
        auto _new = *this;
        for (const auto &x : other)
            _new.append(x);
        return _new;
    }
};

class QJsonIO
{
  public:
    const static inline QJsonValue Null = QJsonValue::Null;
    const static inline QJsonValue Undefined = QJsonValue::Undefined;

    template<typename current_key_type, typename... t_other_types>
    static QJsonValue GetValue(const QJsonValue &parent, const current_key_type &current, const t_other_types &...other)
    {
        if constexpr (sizeof...(t_other_types) == 0)
            if constexpr (std::is_integral_v<current_key_type>)
                return parent.toArray()[current];
            else
                return parent.toObject()[current];
        else if constexpr (std::is_integral_v<current_key_type>)
            return GetValue(parent.toArray()[current], other...);
        else
            return GetValue(parent.toObject()[current], other...);
    }

    template<typename... key_types_t>
    static QJsonValue GetValue(QJsonValue value, const std::tuple<key_types_t...> &keys, const QJsonValue &defaultValue = Undefined)
    {
        std::apply([&](auto &&...args) { ((value = value[args]), ...); }, keys);
        return value.isUndefined() ? defaultValue : value;
    }

    template<typename parent_type, typename t_value_type, typename current_key_type, typename... t_other_key_types>
    static void SetValue(parent_type &parent, const t_value_type &val, const current_key_type &current, const t_other_key_types &...other)
    {
        // If current parent is an array, increase its size to fit the "key"
        if constexpr (std::is_integral_v<current_key_type>)
            for (auto i = parent.size(); i <= current; i++)
                parent.insert(i, {});

        // If the t_other_key_types has nothing....
        // Means we have reached the end of recursion.
        if constexpr (sizeof...(t_other_key_types) == 0)
            parent[current] = val;
        else if constexpr (std::is_integral_v<typename std::tuple_element_t<0, std::tuple<t_other_key_types...>>>)
        {
            // Means we still have many keys
            // So this element is an array.
            auto _array = parent[current].toArray();
            SetValue(_array, val, other...);
            parent[current] = _array;
        }
        else
        {
            auto _object = parent[current].toObject();
            SetValue(_object, val, other...);
            parent[current] = _object;
        }
    }

    static QJsonValue GetValue(const QJsonValue &parent, const QJsonIOPath &path, const QJsonValue &defaultValue = QJsonIO::Undefined)
    {
        QJsonValue val = parent;
        for (const auto &[k, t] : path)
        {
            if (t == QJsonIOPathType::JSONIO_MODE_ARRAY)
                val = val.toArray()[k.toInt()];
            else
                val = val.toObject()[k];
        }
        return val.isUndefined() ? defaultValue : val;
    }

    template<typename parent_type, typename value_type>
    static void SetValue(parent_type &parent, const value_type &t, const QJsonIOPath &path)
    {
        QList<std::tuple<QString, QJsonIOPathType, QJsonValue>> _stack;
        QJsonValue lastNode = parent;
        for (const auto &[key, type] : path)
        {
            _stack.prepend({ key, type, lastNode });
            if (type == QJsonIOPathType::JSONIO_MODE_ARRAY)
                lastNode = lastNode.toArray().at(key.toInt());
            else
                lastNode = lastNode.toObject()[key];
        }

        lastNode = t;

        for (const auto &[key, type, node] : _stack)
        {
            if (type == QJsonIOPathType::JSONIO_MODE_ARRAY)
            {
                const auto index = key.toInt();
                auto nodeArray = node.toArray();
                for (auto i = nodeArray.size(); i <= index; i++)
                    nodeArray.insert(i, {});
                nodeArray[index] = lastNode;
                lastNode = nodeArray;
            }
            else
            {
                auto nodeObject = node.toObject();
                nodeObject[key] = lastNode;
                lastNode = nodeObject;
            }
        }

        if constexpr (std::is_same_v<parent_type, QJsonObject>)
            parent = lastNode.toObject();
        else if constexpr (std::is_same_v<parent_type, QJsonArray>)
            parent = lastNode.toArray();
        else
            Q_UNREACHABLE();
    }
};
