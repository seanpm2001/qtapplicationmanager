/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2019 Luxoft Sweden AB
** Copyright (C) 2018 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtApplicationManager module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#pragma once

#include <QByteArray>
#include <QString>
#include <QtAppManCommon/global.h>

QT_BEGIN_NAMESPACE_AM

namespace Cryptography {

QByteArray generateRandomBytes(int size);

void initialize();

void enableOpenSsl3LegacyProvider(); // needs to be called before any other crypto functions

QString errorString(qint64 osCryptoError, const char *errorDescription = nullptr);

}

QT_END_NAMESPACE_AM
