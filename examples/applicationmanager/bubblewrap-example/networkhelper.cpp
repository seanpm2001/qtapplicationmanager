// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "networkhelper.h"

#include <QNetworkInterface>

NetworkHelper::NetworkHelper(QObject *parent)
    : QObject{parent}
{
    const auto addresses = QNetworkInterface::allAddresses();
    for (const auto &address : addresses) {
        m_ipAddresses += address.toString();
    }

    qDebug() << "FOOOO" << addresses << m_ipAddresses;
}

QStringList NetworkHelper::ipAddresses() const
{
    return m_ipAddresses;
}
