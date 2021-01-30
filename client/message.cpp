#include "message.h"

Message::Message(State state, const QStringList& args) :
    m_valid(true),
    m_state(state),
    m_args(args)
{

}

bool Message::isValid() const
{
    return m_valid;
}

QString Message::textState() const
{
    switch (m_state) {
    case State::Unknown: return "Unknown";
    case State::Initialize: return "Initialize";
    case State::StartRequest: return "StartRequest";
    case State::Started: return "Started";
    case State::FinishRequest: return "FinishRequest";
    case State::Finished: return "Finished";
    case State::RoutesAddRequest: return "RoutesAddRequest";
    case State::RouteDeleteRequest: return "RouteDeleteRequest";
    case State::ClearSavedRoutesRequest: return "ClearSavedRoutesRequest";
    case State::FlushDnsRequest: return "FlushDnsRequest";
    case State::InstallDriverRequest: return "InstallDriverRequest";
    default:
        ;
    }
    return QString();
}

QString Message::rawData() const
{
    return m_rawData;
}

Message::State Message::state() const
{
    return m_state;
}

QString Message::toString() const
{
    if (!isValid()) {
        return QString();
    }

    return QString("%1%2%3")
            .arg(textState())
            .arg(m_dataSeparator)
            .arg(argsToString());
}

QString Message::argAtIndex(int index) const
{
    if ((index + 1) > args().size()) {
        return QString();
    }

    return args().at(index);
}

QStringList Message::args() const
{
    return m_args;
}

QString Message::argsToString() const
{
    return m_args.join(m_argSeparator);
}

Message::Message(const QString& data)
{
    m_rawData = data;
    m_valid = false;
    if (data.isEmpty()) {
        return;
    }

    QStringList dataList = data.split(m_dataSeparator);
    if ((dataList.size() != 2)) {
        return;
    }

    bool stateFound = false;
    for (int i = static_cast<int>(State::Unknown); i <= static_cast<int>(State::InstallDriverRequest); i++ ) {
        m_state = static_cast<State>(i);
        if (textState() == dataList.at(0)) {
            stateFound = true;
            break;
        }
    }

    if (!stateFound) {
        return;
    }

    m_args = dataList.at(1).split(m_argSeparator);
    m_valid = true;
}

