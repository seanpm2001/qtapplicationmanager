// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QHttpServer>
#include "psconfiguration.h"


class PSHttpInterfacePrivate
{
public:
    PSConfiguration *cfg = nullptr;
    QHttpServer *server = nullptr;
    QString listenAddress;
};
