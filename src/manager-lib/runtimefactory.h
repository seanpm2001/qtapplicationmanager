// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>

#include <QtAppManCommon/global.h>

QT_BEGIN_NAMESPACE_AM

class Application;
class AbstractRuntime;
class AbstractRuntimeManager;
class AbstractContainer;


class RuntimeFactory : public QObject
{
    Q_OBJECT
public:
    static RuntimeFactory *instance();
    ~RuntimeFactory();

    QStringList runtimeIds() const;

    AbstractRuntimeManager *manager(const QString &id);
    AbstractRuntime *create(AbstractContainer *container, Application *app);
    AbstractRuntime *createQuickLauncher(AbstractContainer *container, const QString &id);

    void setConfiguration(const QVariantMap &configuration);
    void setSystemProperties(const QVariantMap &thirdParty, const QVariantMap &builtIn);
    void setSlowAnimations(bool isSlow);
    void setSystemOpenGLConfiguration(const QVariantMap &openGLConfiguration);
    void setIconTheme(const QStringList &themeSearchPaths, const QString &themeName);

    bool registerRuntime(AbstractRuntimeManager *manager);
    bool registerRuntime(AbstractRuntimeManager *manager, const QString &identifier);

private:
    RuntimeFactory(QObject *parent = nullptr);
    RuntimeFactory(const RuntimeFactory &);
    RuntimeFactory &operator=(const RuntimeFactory &);
    static RuntimeFactory *s_instance;

    QHash<QString, AbstractRuntimeManager *> m_runtimes;

    // To be passed to newly created runtimes
    bool m_slowAnimations{false};
};

QT_END_NAMESPACE_AM
