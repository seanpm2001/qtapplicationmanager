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

#include <QtAppManApplication/applicationinterface.h>

QT_BEGIN_NAMESPACE_AM

class NativeRuntime;

class NativeRuntimeApplicationInterface : public ApplicationInterface
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.qt.ApplicationManager.ApplicationInterface")

public:
    NativeRuntimeApplicationInterface(NativeRuntime *runtime);

    QString applicationId() const override;
    QVariantMap name() const override;
    QUrl icon() const override;
    QString version() const override;
    QVariantMap systemProperties() const override;
    QVariantMap applicationProperties() const override;

    virtual void finishedInitialization() override;

signals:
    void applicationFinishedInitialization();

private:
    NativeRuntime *m_runtime;
};

class NativeRuntimeInterface : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.qt.ApplicationManager.RuntimeInterface")

public:
    NativeRuntimeInterface(NativeRuntime *runtime);

signals:
    Q_SCRIPTABLE void startApplication(const QString &baseDir, const QString &app, const QString &document,
                                       const QString &mimeType, const QVariantMap &application,
                                       const QVariantMap &systemProperties);
};

QT_END_NAMESPACE_AM
// We mean it. Dummy comment since syncqt needs this also for completely private Qt modules.

