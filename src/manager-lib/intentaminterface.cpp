// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "global.h"
#if defined(AM_MULTI_PROCESS)
#  include <QDBusMessage>
#  include <QDBusConnection>
#  include <QDBusPendingCallWatcher>
#  include <QDBusPendingReply>

#  include "intentinterface_adaptor.h"
#  include "dbus-utilities.h"
#  include "nativeruntime.h"
#endif
#include <QDebug>
#include <QMetaObject>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlContext>
#include <QQmlInfo>

#include "error.h"
#include "exception.h"
#include "logging.h"
#include "utilities.h"
#include "intentserver.h"
#include "intentclient.h"
#include "intentserverrequest.h"
#include "intentclientrequest.h"
#include "intentaminterface.h"
#include "qmlinprocruntime.h"
#include "application.h"
#include "applicationmanager.h"
#include "package.h"
#include "packagemanager.h"
#include "applicationinfo.h"

QT_BEGIN_NAMESPACE_AM


//////////////////////////////////////////////////////////////////////////
// vvv IntentAMImplementation vvv


IntentServer *IntentAMImplementation::createIntentServerAndClientInstance(PackageManager *packageManager,
                                                                          int disambiguationTimeout,
                                                                          int startApplicationTimeout,
                                                                          int replyFromApplicationTimeout,
                                                                          int replyFromSystemTimeout)
{
    auto intentServerAMInterface = new IntentServerAMImplementation;
    auto intentClientAMInterface = new IntentClientAMImplementation(intentServerAMInterface);
    auto intentServer = IntentServer::createInstance(intentServerAMInterface);
    auto intentClient = IntentClient::createInstance(intentClientAMInterface);

    intentServer->setDisambiguationTimeout(disambiguationTimeout * timeoutFactor());
    intentServer->setStartApplicationTimeout(startApplicationTimeout * timeoutFactor());

    // These timeouts are for the same thing - the time needed for the application's handler to
    // generate a reply - but one is for the server side, while the other for the client side.
    // Having two separate config values would be confusing, so we set the application side to
    // 90% of the server side, because the communication overhead is not included there.
    {
        int t = replyFromApplicationTimeout * timeoutFactor();
        intentServer->setReplyFromApplicationTimeout(t);
        intentClient->setReplyFromApplicationTimeout(t <= 0 ? t : int(t * 0.9));
    }

    intentClient->setReplyFromSystemTimeout(replyFromSystemTimeout * timeoutFactor());

    // this way, deleting the server (the return value of this factory function) will get rid
    // of both client and server as well as both their AM interfaces
    intentClient->setParent(intentServer);

    // connect the APIs of the PackageManager and the IntentServer
    // the Intent API does not use AM internal types, so we have to translate using id strings:
    // the idea behind this is that the Intent subsystem could be useful outside of the AM as well,
    // so we try not to use AM specific classes in the intent-server and intent-client modules.
    QObject::connect(packageManager, &PackageManager::packageAdded,
                     intentServer, &IntentServer::addPackage);
    QObject::connect(packageManager, &PackageManager::packageAboutToBeRemoved,
                     intentServer, &IntentServer::removePackage);

    QObject::connect(&packageManager->internalSignals, &PackageManagerInternalSignals::registerApplication,
                     intentServer, [intentServer](ApplicationInfo *applicationInfo, Package *package) {
        intentServer->addApplication(applicationInfo->id(), package->id());
    });
    QObject::connect(&packageManager->internalSignals, &PackageManagerInternalSignals::unregisterApplication,
                     intentServer, [intentServer](ApplicationInfo *applicationInfo, Package *package) {
        intentServer->removeApplication(applicationInfo->id(), package->id());
    });

    QObject::connect(&packageManager->internalSignals, &PackageManagerInternalSignals::registerIntent,
                     intentServer, [intentServer](IntentInfo *intentInfo, Package *package) {
        if (!intentServer->addIntent(intentInfo->id(), package->id(), intentInfo->handlingApplicationId(),
                                     intentInfo->requiredCapabilities(),
                                     intentInfo->visibility() == IntentInfo::Public ? Intent::Public
                                                                                    : Intent::Private,
                                     intentInfo->parameterMatch(), intentInfo->names(),
                                     intentInfo->descriptions(),
                                     QUrl::fromLocalFile(package->info()->baseDir().absoluteFilePath(intentInfo->icon())),
                                     intentInfo->categories(), intentInfo->handleOnlyWhenRunning())) {
            throw Exception(Error::Intents, "could not add intent %1 for package %2")
                .arg(intentInfo->id()).arg(package->id());
        }
        qCDebug(LogSystem).nospace().noquote() << " ++ intent: " << intentInfo->id() << " [package: " << package->id() << "]";
    });
    QObject::connect(&packageManager->internalSignals, &PackageManagerInternalSignals::unregisterIntent,
                     intentServer, [intentServer](IntentInfo *intentInfo, Package *package) {
        Intent *intent = intentServer->packageIntent(intentInfo->id(), package->id(),
                                                     intentInfo->parameterMatch());
        qCDebug(LogSystem).nospace().noquote() << " -- intent: " << intentInfo->id() << " [package: " << package->id() << "]";
        Q_ASSERT(intent);
        intentServer->removeIntent(intent);
    });
    return intentServer;
}


// ^^^ IntentAMImplementation ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentServerAMImplementation vvv


void IntentServerAMImplementation::setIntentClientSystemInterface(IntentClientSystemInterface *iface)
{
    m_icsi = iface;
}

IntentClientSystemInterface *IntentServerAMImplementation::intentClientSystemInterface() const
{
    return m_icsi;
}

void IntentServerAMImplementation::initialize(IntentServer *server)
{
    IntentServerSystemInterface::initialize(server);

    // this is a dummy connection for the System UI, so that we can route replies
    // back to it without any ugly hacks in the core IntentServer code.
    IntentServerInProcessIpcConnection::createSystemUi(this);

    // The IntentServer itself doesn't know about the p2p D-Bus or the AM itself, so we need to
    // wire it up to both interfaces from the outside
    connect(AbstractRuntime::signaler(), &RuntimeSignaler::aboutToStart,
            intentServer(), [this](AbstractRuntime *runtime) {
#if defined(AM_MULTI_PROCESS)
        if (NativeRuntime *nativeRuntime = qobject_cast<NativeRuntime *>(runtime)) {
            connect(nativeRuntime, &NativeRuntime::applicationConnectedToPeerDBus,
                    intentServer(), [this](const QDBusConnection &connection, Application *application) {
                qCDebug(LogIntents) << "IntentServer: applicationConnectedToPeerDBus"
                                    << (application ? application->id() : qSL("<launcher>"));

                IntentServerDBusIpcConnection::create(connection, application, this);
            });

            connect(nativeRuntime, &NativeRuntime::applicationReadyOnPeerDBus,
                    intentServer(), [](const QDBusConnection &connection, Application *application) {
                auto peer = IntentServerDBusIpcConnection::find(connection);

                if (!peer) {
                    qCWarning(LogIntents) << "IntentServer: applicationReadyOnPeerDBus() was emitted, "
                                             "but no previous applicationConnectedToPeerDBus() was seen";
                    return;
                }
                peer->setReady(application);
            });

            connect(nativeRuntime, &NativeRuntime::applicationDisconnectedFromPeerDBus,
                    intentServer(), [](const QDBusConnection &connection, Application *) {
                delete IntentServerDBusIpcConnection::find(connection);
            });
        } else
#endif // defined(AM_MULTI_PROCESS)
            if (QmlInProcRuntime *qmlRuntime = qobject_cast<QmlInProcRuntime *>(runtime)) {
                connect(qmlRuntime, &QmlInProcRuntime::stateChanged,
                    intentServer(), [this, qmlRuntime](Am::RunState newState) {
                if (!qmlRuntime->application())
                    return;

                if (newState == Am::Running)
                    IntentServerInProcessIpcConnection::create(qmlRuntime->application(), this);
                else if (newState == Am::NotRunning)
                    delete IntentServerIpcConnection::find(qmlRuntime->application()->id());
            });
        }
    });
}

bool IntentServerAMImplementation::checkApplicationCapabilities(const QString &applicationId,
                                                                const QStringList &requiredCapabilities)
{
    if (applicationId == IntentClient::instance()->systemUiId()) // The System UI bypasses the capabilities check
        return true;

    const auto app = ApplicationManager::instance()->application(applicationId);
    if (!app)
        return false;

    auto capabilities = app->capabilities();
    for (const auto &cap : requiredCapabilities) {
        if (!capabilities.contains(cap))
            return false;
    }
    return true;
}

IntentServerSystemInterface::IpcConnection *IntentServerAMImplementation::findClientIpc(const QString &appId)
{
    auto peer = IntentServerIpcConnection::find(appId);
    return (peer && peer->isReady()) ? reinterpret_cast<IpcConnection *>(peer) : nullptr;
}

void IntentServerAMImplementation::startApplication(const QString &appId)
{
    ApplicationManager::instance()->startApplication(appId);
}

void IntentServerAMImplementation::requestToApplication(IntentServerSystemInterface::IpcConnection *clientIPC,
                                                        IntentServerRequest *isr)
{
    reinterpret_cast<IntentServerIpcConnection *>(clientIPC)->requestToApplication(isr);
}

void IntentServerAMImplementation::replyFromSystem(IntentServerSystemInterface::IpcConnection *clientIPC,
                                                   IntentServerRequest *isr)
{
    reinterpret_cast<IntentServerIpcConnection *>(clientIPC)->replyFromSystem(isr);
}


// ^^^ IntentServerAMImplementation ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentClientAMImplementation vvv


IntentClientAMImplementation::IntentClientAMImplementation(IntentServerAMImplementation *serverInterface)
    : IntentClientSystemInterface()
    , m_issi(serverInterface)
{
    serverInterface->setIntentClientSystemInterface(this);
}

QString IntentClientAMImplementation::currentApplicationId(QObject *hint)
{
    QmlInProcRuntime *runtime = QmlInProcRuntime::determineRuntime(hint);

    return runtime ? runtime->application()->info()->id() : IntentClient::instance()->systemUiId();
}

void IntentClientAMImplementation::initialize(IntentClient *intentClient) Q_DECL_NOEXCEPT_EXPR(false)
{
    IntentClientSystemInterface::initialize(intentClient);

    qmlRegisterSingletonType<IntentClient>("QtApplicationManager", 2, 0, "IntentClient",
                                           [](QQmlEngine *, QJSEngine *) -> QObject * {
        QQmlEngine::setObjectOwnership(IntentClient::instance(), QQmlEngine::CppOwnership);
        return IntentClient::instance();
    });
    qmlRegisterRevision<IntentClient, 1>("QtApplicationManager", 2, 1);

    qmlRegisterUncreatableType<IntentClientRequest>("QtApplicationManager", 2, 0, "IntentRequest",
                                                    qSL("Cannot create objects of type IntentRequest"));
    qmlRegisterUncreatableType<IntentClientRequest, 1>("QtApplicationManager", 2, 1, "IntentRequest",
                                                       qSL("Cannot create objects of type IntentRequest"));
    qmlRegisterType<IntentHandler>("QtApplicationManager.Application", 2, 0, "IntentHandler");

    qmlRegisterType<IntentServerHandler>("QtApplicationManager.SystemUI", 2, 0, "IntentServerHandler");
}

void IntentClientAMImplementation::requestToSystem(QPointer<IntentClientRequest> icr)
{
    // we need to delay the request by one event loop iteration to (a) avoid a race condition
    // on app startup and (b) have consistent behavior in single- and multi-process mode

    QMetaObject::invokeMethod(m_ic, [icr, this]() {
        if (!icr)
            return;

        IntentServerRequest *isr = m_issi->requestToSystem(icr->requestingApplicationId(), icr->intentId(),
                                                           icr->applicationId(), icr->parameters());
        QUuid requestId = isr ? isr->requestId() : QUuid();

        QMetaObject::invokeMethod(m_ic, [icr, requestId, this]() {
            emit requestToSystemFinished(icr.data(), requestId, requestId.isNull(),
                                         requestId.isNull() ? qL1S("No matching intent handler registered.") : QString());
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void IntentClientAMImplementation::replyFromApplication(QPointer<IntentClientRequest> icr)
{
    if (icr) {
        emit m_issi->replyFromApplication(icr->applicationId(), icr->requestId(), !icr->succeeded(),
                                          icr->result());
    }
}


// ^^^ IntentClientAMImplementation ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentServerIpcConnection vvv


QList<IntentServerIpcConnection *> IntentServerIpcConnection::s_ipcConnections;

IntentServerIpcConnection::IntentServerIpcConnection(bool inProcess, Application *application,
                                                     IntentServerAMImplementation *iface)
    : QObject()
    , m_application(application)
    , m_interface(iface)
    , m_inprocess(inProcess)
{
    connect(this, &IntentServerIpcConnection::applicationIsReady,
            m_interface, &IntentServerSystemInterface::applicationWasStarted);
}

IntentServerIpcConnection::~IntentServerIpcConnection()
{
    s_ipcConnections.removeOne(this);
}

bool IntentServerIpcConnection::isReady() const
{
    return m_ready;
}

void IntentServerIpcConnection::setReady(Application *application)
{
    if (m_ready)
        return;
    m_application = application;
    m_ready = true;
    emit applicationIsReady((isInProcess() && !application) ? IntentClient::instance()->systemUiId()
                                                            : application->id());
}

IntentServerIpcConnection *IntentServerIpcConnection::find(const QString &appId)
{
    for (auto ipcConnection : std::as_const(s_ipcConnections)) {
        if (ipcConnection->applicationId() == appId)
            return ipcConnection;
    }
    return nullptr;
}

Application *IntentServerIpcConnection::application() const
{
    return m_application;
}

QString IntentServerIpcConnection::applicationId() const
{
    return m_application ? m_application->id() : QString();
}

bool IntentServerIpcConnection::isInProcess() const
{
    return m_inprocess;
}


// ^^^ IntentServerIpcConnection ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentServerInProcessIpcConnection vvv


IntentServerInProcessIpcConnection::IntentServerInProcessIpcConnection(Application *application,
                                                                       IntentServerAMImplementation *iface)
    : IntentServerIpcConnection(true /*inProcess*/, application, iface)
{ }

IntentServerInProcessIpcConnection *IntentServerInProcessIpcConnection::create(Application *application,
                                                                               IntentServerAMImplementation *iface)
{
    auto ipcConnection = new IntentServerInProcessIpcConnection(application, iface);
    if (application)
        ipcConnection->setReady(application);
    else
        ipcConnection->m_ready = true;

    s_ipcConnections << ipcConnection;
    return ipcConnection;
}

IntentServerInProcessIpcConnection *IntentServerInProcessIpcConnection::createSystemUi(IntentServerAMImplementation *iface)
{
    auto ipcConnection = create(nullptr, iface);
    ipcConnection->m_isSystemUi = true;
    ipcConnection->setParent(iface);
    return ipcConnection;
}

QString IntentServerInProcessIpcConnection::applicationId() const
{
    return m_isSystemUi ? IntentClient::instance()->systemUiId()
                        : IntentServerIpcConnection::applicationId();
}

void IntentServerInProcessIpcConnection::requestToApplication(IntentServerRequest *isr)
{
    // we need decouple the server/client interface at this point to have a consistent
    // behavior in single- and multi-process mode
    QMetaObject::invokeMethod(this, [this, isr]() {
        auto clientInterface = m_interface->intentClientSystemInterface();
        emit clientInterface->requestToApplication(isr->requestId(), isr->intentId(),
                                                   isr->isBroadcast() ? qSL(":broadcast:") : isr->requestingApplicationId(),
                                                   isr->selectedIntent()->applicationId(), isr->parameters());
    }, Qt::QueuedConnection);
}

void IntentServerInProcessIpcConnection::replyFromSystem(IntentServerRequest *isr)
{
    // we need decouple the server/client interface at this point to have a consistent
    // behavior in single- and multi-process mode
    QMetaObject::invokeMethod(this, [this, isr]() {
        auto clientInterface = m_interface->intentClientSystemInterface();
        emit clientInterface->replyFromSystem(isr->requestId(), !isr->succeeded(), isr->result());
    }, Qt::QueuedConnection);
}


// ^^^ IntentServerInProcessIpcConnection ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentServerDBusIpcConnection vvv

#if defined(AM_MULTI_PROCESS)

IntentServerDBusIpcConnection::IntentServerDBusIpcConnection(QDBusConnection connection,
                                                             Application *application,
                                                             IntentServerAMImplementation *iface)
    : IntentServerIpcConnection(false /* !inProcess*/, application, iface)
{
    m_connectionName = connection.name();
    m_adaptor = new IntentInterfaceAdaptor(this);
    connection.registerObject(qSL("/IntentServer"), this, QDBusConnection::ExportAdaptors);
}

IntentServerDBusIpcConnection::~IntentServerDBusIpcConnection()
{
    QDBusConnection(m_connectionName).unregisterObject(qSL("/IntentServer"));
}

IntentServerDBusIpcConnection *IntentServerDBusIpcConnection::create(QDBusConnection connection,
                                                                     Application *application,
                                                                     IntentServerAMImplementation *iface)
{
    auto ipcConnection = new IntentServerDBusIpcConnection(connection, application, iface);
    s_ipcConnections << ipcConnection;
    return ipcConnection;
}

IntentServerDBusIpcConnection *IntentServerDBusIpcConnection::find(QDBusConnection connection)
{
    QString connectionName = connection.name();

    for (auto ipcConnection : std::as_const(s_ipcConnections)) {
        if (ipcConnection->isInProcess())
            continue;
        auto dbusIpcConnection = static_cast<IntentServerDBusIpcConnection *>(ipcConnection);
        if (dbusIpcConnection->m_connectionName == connectionName)
            return dbusIpcConnection;
    }
    return nullptr;
}

void IntentServerDBusIpcConnection::requestToApplication(IntentServerRequest *isr)
{
    Q_ASSERT(isr && isr->selectedIntent());

    emit m_adaptor->requestToApplication(isr->requestId().toString(), isr->intentId(),
                                         isr->selectedIntent()->applicationId(),
                                         convertFromJSVariant(isr->parameters()).toMap());
}

void IntentServerDBusIpcConnection::replyFromSystem(IntentServerRequest *isr)
{
    Q_ASSERT(isr);

    emit m_adaptor->replyFromSystem(isr->requestId().toString(), !isr->succeeded(),
                                    convertFromJSVariant(isr->result()).toMap());
}

QString IntentServerDBusIpcConnection::requestToSystem(const QString &intentId,
                                                       const QString &applicationId,
                                                       const QVariantMap &parameters)
{
    auto requestingApplicationId = application() ? application()->id() : QString();
    auto isr = m_interface->requestToSystem(requestingApplicationId, intentId, applicationId,
                                            convertFromDBusVariant(parameters).toMap());
    if (!isr) {
        sendErrorReply(QDBusError::NotSupported, qL1S("No matching intent handler registered."));
        return QString();
    } else {
        return isr->requestId().toString();
    }
}

void IntentServerDBusIpcConnection::replyFromApplication(const QString &requestId, bool error,
                                                         const QVariantMap &result)
{
    emit m_interface->replyFromApplication(application()->id(), QUuid::fromString(requestId), error,
                                           convertFromDBusVariant(result).toMap());
}

#endif // defined(AM_MULTI_PROCESS)


// ^^^ IntentServerDBusIpcConnection ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentInterfaceAdaptor vvv

QT_END_NAMESPACE_AM

#if defined(AM_MULTI_PROCESS)

IntentInterfaceAdaptor::IntentInterfaceAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{ }

IntentInterfaceAdaptor::~IntentInterfaceAdaptor()
{ }

void IntentInterfaceAdaptor::replyFromApplication(const QString &requestId, bool error,
                                                  const QVariantMap &result)
{
    auto peer = static_cast<QT_PREPEND_NAMESPACE_AM(IntentServerDBusIpcConnection) *>(parent());
    peer->replyFromApplication(requestId, error, result);
}

QString IntentInterfaceAdaptor::requestToSystem(const QString &intentId, const QString &applicationId,
                                                const QVariantMap &parameters)
{
    auto peer = static_cast<QT_PREPEND_NAMESPACE_AM(IntentServerDBusIpcConnection) *>(parent());
    return peer->requestToSystem(intentId, applicationId, parameters);
}

#endif // defined(AM_MULTI_PROCESS)

QT_BEGIN_NAMESPACE_AM

// ^^^ IntentInterfaceAdaptor ^^^
//////////////////////////////////////////////////////////////////////////
// vvv IntentServerHandler vvv

/*! \qmltype IntentServerHandler
    \inqmlmodule QtApplicationManager.SystemUI
    \inherits IntentHandler
    \ingroup system-ui-instantiable
    \brief A handler for intent requests received within the System UI.

    If intents need to be handled from within the System UI, you need to have a corresponding
    IntentServerHandler instance that is actually able to handle incoming requests. This class gives
    you the flexibility to handle multiple, different intent ids via a single IntentServerHandler
    instance or have a dedicated IntentServerHandler instance for every intent id (or any
    combination of those).

    \note For handling intent requests within an application, you have to use the application side
          component IntentHandler, which works the same way, but provides all the necessary
          meta-data in the application's info.yaml manifest file.

    For more information see IntentHandler and the description of the \l{manifest-intent}
    {meta-data in the manifest documentation}.

    Callbacks connected to the onRequestReceived signal have access to the sender's application ID.
    Due to security restrictions, this is \b not the case for such handlers implemented in an
    application context via IntentHandler.
*/

/*! \qmlproperty url IntentServerHandler::icon

    The intent's icon.
    This corresponds to the \c icon field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty object IntentServerHandler::names

    An object with all the language code to localized name mappings for this intent.
    This corresponds to the \c name field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty object IntentServerHandler::descriptions

    An object with all the language code to localized description mappings for this intent.
    This corresponds to the \c description field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty list<string> IntentServerHandler::categories

    The intent's categories.
    This corresponds to the \c categories field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty enum IntentServerHandler::visibility

    The intent's visibility. Can be either \c IntentObject.Public (the default) or
    \c IntentObject.Private.
    This corresponds to the \c visibility field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty list<string> IntentServerHandler::requiredCapabilities

    The intent's required capabilities.
    This corresponds to the \c requiredCapabilities field in the \l{manifest-intent}
    {manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/
/*! \qmlproperty object IntentServerHandler::parameterMatch

    The intent's parameter requirements.
    This corresponds to the \c parameterMatch field in the \l{manifest-intent}{manifest documentation}.

    \note Any changes to this property after component completion will have no effect.
*/

IntentServerHandler::IntentServerHandler(QObject *parent)
    : AbstractIntentHandler(parent)
    , m_intent(new Intent())
{ }

IntentServerHandler::~IntentServerHandler()
{
    IntentServer *is = IntentServer::instance();

    for (const auto &intent : m_registeredIntents)
        is->removeIntent(intent);

    delete m_intent;
}

QUrl IntentServerHandler::icon() const
{
    return m_intent->icon();
}

QVariantMap IntentServerHandler::names() const
{
    return m_intent->names();
}

QVariantMap IntentServerHandler::descriptions() const
{
    return m_intent->descriptions();
}

QStringList IntentServerHandler::categories() const
{
    return m_intent->categories();
}

Intent::Visibility IntentServerHandler::visibility() const
{
    return m_intent->visibility();
}

QStringList IntentServerHandler::requiredCapabilities() const
{
    return m_intent->requiredCapabilities();
}

QVariantMap IntentServerHandler::parameterMatch() const
{
    return m_intent->parameterMatch();
}

void IntentServerHandler::setIcon(const QUrl &icon)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the icon property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_icon = icon;
}

void IntentServerHandler::setNames(const QVariantMap &names)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the names property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_names.clear();
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        m_intent->m_names.insert(it.key(), it.value().toString());
}

void IntentServerHandler::setDescriptions(const QVariantMap &descriptions)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the descriptions property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_descriptions.clear();
    for (auto it = descriptions.cbegin(); it != descriptions.cend(); ++it)
        m_intent->m_descriptions.insert(it.key(), it.value().toString());
}

void IntentServerHandler::setCategories(const QStringList &categories)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the categories property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_categories = categories;
}

void IntentServerHandler::setVisibility(Intent::Visibility visibility)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the visibility property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_visibility = visibility;
}

void IntentServerHandler::setRequiredCapabilities(const QStringList &requiredCapabilities)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the requiredCapabilities property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_requiredCapabilities = requiredCapabilities;
}

void IntentServerHandler::setParameterMatch(const QVariantMap &parameterMatch)
{
    if (isComponentCompleted()) {
        qmlWarning(this) << "Cannot change the parameterMatch property of an IntentServerHandler after creation.";
        return;
    }
    m_intent->m_parameterMatch = parameterMatch;
}

void IntentServerHandler::componentComplete()
{
    if (QmlInProcRuntime::determineRuntime(this)) {
        qmlWarning(this) << "Using IntentServerHandler for handling events in an application "
                            "context does not work. Use IntentHandler instead";
        return;
    }

    QString sysUiId = IntentClient::instance()->systemUiId();

    IntentServer *is = IntentServer::instance();
    is->addPackage(sysUiId);
    is->addApplication(sysUiId, sysUiId);

    const auto ids = intentIds();
    for (const auto &intentId : ids) {
        // convert from QVariantMap to QMap<QString, QString>
        QMap<QString, QString> names, descriptions;
        const auto qvm_names = m_intent->names();
        const auto qvm_descriptions = m_intent->descriptions();
        for (auto it = qvm_names.cbegin(); it != qvm_names.cend(); ++it)
            names.insert(it.key(), it.value().toString());
        for (auto it = qvm_descriptions.cbegin(); it != qvm_descriptions.cend(); ++it)
            descriptions.insert(it.key(), it.value().toString());

        auto intent = is->addIntent(intentId, sysUiId, sysUiId, m_intent->requiredCapabilities(),
                                    m_intent->visibility(), m_intent->parameterMatch(), names,
                                    descriptions, m_intent->icon(), m_intent->categories(),
                                    m_intent->handleOnlyWhenRunning());
        if (intent)
            m_registeredIntents << intent;
        else
            qmlWarning(this) << "IntentServerHandler: could not add intent" << intentId;
    }

    AbstractIntentHandler::componentComplete();
}

QT_END_NAMESPACE_AM

#include "moc_intentaminterface.cpp"
