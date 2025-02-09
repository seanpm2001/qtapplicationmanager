// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtCore/QUuid>
#include <QtCore/QVector>
#include <QtCore/QPointer>
#include <QtAppManCommon/global.h>
#include <QtAppManIntentServer/intent.h>

QT_BEGIN_NAMESPACE_AM

class IntentServer;

class IntentServerRequest
{
    Q_GADGET

public:
    IntentServerRequest(const QString &requestingApplicationId, const QString &intentId,
                        const QVector<Intent *> &potentialIntents, const QVariantMap &parameters,
                        bool broadcast);

    enum class State {
        ReceivedRequest,
        WaitingForDisambiguation,
        Disambiguated,
        WaitingForApplicationStart,
        StartedApplication,
        WaitingForReplyFromApplication,
        ReceivedReplyFromApplication,
    };

    Q_ENUM(State)

    State state() const;
    QUuid requestId() const;
    QString intentId() const;
    QString requestingApplicationId() const;
    Intent *selectedIntent() const;
    QList<Intent *> potentialIntents() const;
    QVariantMap parameters() const;
    bool succeeded() const;
    QVariantMap result() const;
    bool isBroadcast() const;

    void setState(State newState);
    void setSelectedIntent(Intent *intent);

    void setRequestFailed(const QString &errorMessage);
    void setRequestSucceeded(const QVariantMap &result);

private:
    QUuid m_id;
    State m_state;
    bool m_succeeded = false;
    bool m_broadcast = false;
    QString m_intentId;
    QString m_requestingApplicationId;
    QPointer<Intent> m_selectedIntent;
    QVector<QPointer<Intent>> m_potentialIntents;
    QVariantMap m_parameters;
    QVariantMap m_result;
};

QT_END_NAMESPACE_AM
