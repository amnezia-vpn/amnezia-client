// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine.h"

#include "session.h"

#include <QDebug>

namespace amnezia::masterdnsvpn {

// PIMPL — keeps engine.h free of any include of session.h, which lets
// callers outside `client/masterdnsvpn/` ship their own session-less
// translation units (xrayProtocol-style integration on desktop, JNI shim
// on Android).
struct EnginePrivate {
    std::unique_ptr<Session> session;
    Engine::State state = Engine::State::Idle;
    QString lastError;
    quint64 receivedAtSnapshot = 0;
    quint64 sentAtSnapshot = 0;
};

Engine::Engine(QObject *parent) : QObject(parent), d(std::make_unique<EnginePrivate>()) {}

Engine::~Engine()
{
    // Explicit teardown — Session owns sockets that need to be closed
    // before its parent QObject is gone.
    if (d->session) {
        d->session->stop();
    }
}

bool Engine::start(const QJsonObject &config)
{
    if (d->state != State::Idle && d->state != State::Failed) {
        d->lastError = QStringLiteral("Engine already started");
        return false;
    }

    d->session = std::make_unique<Session>(this);

    // Translate Session state changes into our public State enum + signal.
    connect(d->session.get(), &Session::stateChanged, this, [this](Session::State s) {
        State next = State::Idle;
        switch (s) {
        case Session::State::Idle:
            next = State::Idle;
            break;
        case Session::State::Initialising:
        case Session::State::Authenticating:
            next = State::Starting;
            break;
        case Session::State::Established:
            next = State::Connected;
            break;
        case Session::State::TearingDown:
            next = State::Stopping;
            break;
        case Session::State::Stopped:
            next = State::Idle;
            break;
        case Session::State::Failed:
            d->lastError = d->session ? d->session->lastError() : QString();
            next = State::Failed;
            break;
        }
        if (d->state != next) {
            d->state = next;
            emit stateChanged(next);
        }
    });

    // Pump byte-counter deltas to the public bytesChanged signal — Engine
    // emits *deltas* (matching VpnProtocol::setBytesChanged in the rest
    // of the codebase), not absolute values.
    connect(d->session.get(), &Session::bytesChanged, this, [this](quint64 rx, quint64 tx) {
        emit bytesChanged(rx, tx);
    });

    if (!d->session->start(config)) {
        d->lastError = d->session->lastError();
        d->state = State::Failed;
        emit stateChanged(State::Failed);
        return false;
    }
    return true;
}

void Engine::stop()
{
    if (!d->session) {
        return;
    }
    d->session->stop();
    d->session.reset();
    if (d->state != State::Idle) {
        d->state = State::Idle;
        emit stateChanged(State::Idle);
    }
}

Engine::State Engine::state() const
{
    return d->state;
}

QString Engine::lastError() const
{
    return d->lastError;
}

quint16 Engine::socksPort() const
{
    return d->session ? d->session->socksPort() : 0;
}

quint64 Engine::bytesReceived() const
{
    return d->session ? d->session->bytesReceived() : 0;
}

quint64 Engine::bytesSent() const
{
    return d->session ? d->session->bytesSent() : 0;
}

} // namespace amnezia::masterdnsvpn
