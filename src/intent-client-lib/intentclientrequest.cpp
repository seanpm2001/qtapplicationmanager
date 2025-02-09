// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "intentclientrequest.h"
#include "intentclient.h"

#include <QQmlEngine>
#include <QQmlInfo>
#include <QThread>
#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE_AM

/*! \qmltype IntentRequest
    \inqmlmodule QtApplicationManager
    \ingroup common-non-instantiable
    \brief Each instance represents an outgoing or incoming intent request.

    This type is used both in applications as well as within the System UI to represent an intent
    request. This type can not be instantiated directly, but will be returned by
    IntentClient::sendIntentRequest() (for outgoing requests to the system) and
    IntentHandler::requestReceived() (for incoming requests to the application)

    See the IntentClient type for a short example on how to send intent requests to the system.

    The IntentHandler documenatation provides an example showing the use of this type when receiving
    requests from the system.
*/

/*! \qmlproperty uuid IntentRequest::requestId
    \readonly

    Every intent request in the system gets an unique requestId assigned by the server that will be
    used throughout the life-time of the request in every context (requesting application, handling
    application and intent server).
    \note Since this requestId is generated by the server, any IntentRequest object generated by
          IntentClient::sendIntentRequest() will start with a null requestId. The property will
          be updated asynchronously once the server has assigned a new requestId to the
          incoming request.

    \note Constant for requests received by IntentHandlers, valid on both sent and received requests.
*/

/*! \qmlproperty IntentRequest::Direction IntentRequest::direction
    \readonly

    This property describes if this instance is an outgoing or incoming intent request:

    \list
    \li IntentRequest.ToSystem - The request object was generated by IntentClient::sendIntentRequest(),
                                 i.e. this request is sent out to the system side for handling.
    \li IntentRequest.ToApplication - The request object was received by IntentHandler::requestReceived(),
                                      i.e. this request was sent from the system side to the
                                      application for handling.
    \endlist

    \note Constant, valid on both sent and received requests.
*/

/*! \qmlproperty string IntentRequest::intentId
    \readonly

    The requested intent id.

    \note Constant, valid on both sent and received requests.
*/

/*! \qmlproperty string IntentRequest::applicationId
    \readonly

    The id of the application which should be handling this request. Returns an empty string if
    no specific application was requested.

    \note Constant, valid on both sent and received requests.
*/

/*! \qmlproperty string IntentRequest::requestingApplicationId
    \readonly

    The id of the application which created this intent request. Returns an empty string if called
    from within an application context - only the server side has access to this information in
    IntentServerHandler::requestReceived.

    \note Constant, valid on both sent and received requests.
*/

/*! \qmlproperty var IntentRequest::parameters
    \readonly

    All parameters attached to the request as a JavaScript object.

    \note Constant, valid on both sent and received requests.
*/

/*! \qmlproperty bool IntentRequest::succeeded
    \readonly

    As soon as the replyReceived() signal has been emitted, this property will show if the
    intent request was actually successful.

    \note Valid only on sent requests.
*/

/*! \qmlproperty string IntentRequest::errorMessage
    \readonly

    As soon as the replyReceived() signal has been emitted, this property will hold a potential
    error message in case the request failed.

    \note Valid only on sent requests.

    \sa succeeded
*/

/*! \qmlproperty var IntentRequest::result
    \readonly

    As soon as the replyReceived() signal has been emitted, this property will hold the result in
    form of a JavaScript object in case the request succeeded.

    \note Valid only on sent requests.

    \sa succeeded
*/

/*! \qmlproperty bool IntentRequest::broadcast
    \readonly
    \since 6.5

    Only Set to \c true, if the received request is a broadcast.

    \note Valid only on received requests.
*/

/*! \qmlsignal IntentRequest::replyReceived()

    This signal gets emitted when a reply to an intent request is available. The signal handler
    needs to check the succeeded property to decided whether errorMessage or result are actually
    valid.

    \note This signal will only ever by emitted for request objects created by
          IntentClient::sendIntentRequest().
*/


IntentClientRequest::Direction IntentClientRequest::direction() const
{
    return m_direction;
}

IntentClientRequest::~IntentClientRequest()
{
    // the incoming request was gc'ed on the JavaScript side, but no reply was sent yet
    if ((direction() == Direction::ToApplication) && !m_finished && !m_broadcast)
        sendErrorReply(qSL("Request not handled"));
}

QUuid IntentClientRequest::requestId() const
{
    return m_id;
}

QString IntentClientRequest::intentId() const
{
    return m_intentId;
}

QString IntentClientRequest::applicationId() const
{
    return m_applicationId;
}

QString IntentClientRequest::requestingApplicationId() const
{
    return m_requestingApplicationId;
}

QVariantMap IntentClientRequest::parameters() const
{
    return m_parameters;
}

bool IntentClientRequest::isBroadcast() const
{
    return m_broadcast;
}

bool IntentClientRequest::succeeded() const
{
    return m_succeeded;
}

const QVariantMap IntentClientRequest::result() const
{
    return m_result;
}

QString IntentClientRequest::errorMessage() const
{
    return m_errorMessage;
}

/*! \qmlmethod IntentRequest::sendReply(var result)

    An IntentHandler needs to call this function to send its \a result back to the system in reply
    to an request received via IntentHandler::requestReceived().

    Only either sendReply() or sendErrorReply() can be used on a single IntentRequest.

    \note This function only works for request objects received from IntentHandler::requestReceived().
          It will simply do nothing on requests created by IntentClient::sendIntentRequest().

    \sa sendErrorReply
*/
void IntentClientRequest::sendReply(const QVariantMap &result)
{
    //TODO: check that result only contains basic datatypes. convertFromJSVariant() does most of
    //      this already, but doesn't bail out on unconvertible types (yet)

    if (m_direction == Direction::ToSystem) {
        qmlWarning(this) << "Calling IntentRequest::sendReply on requests originating from this application is a no-op.";
        return;
    }
    if (m_broadcast) {
        qmlWarning(this) << "Calling IntentRequest::sendReply on broadcast requests is a no-op.";
        return;
    }

    IntentClient *ic = IntentClient::instance();

    if (QThread::currentThread() != ic->thread()) {
        QPointer<IntentClientRequest> that(this);

        ic->metaObject()->invokeMethod(ic, [that, ic, result]()
        { if (that) ic->replyFromApplication(that.data(), result); },
        Qt::QueuedConnection);
    } else {
        ic->replyFromApplication(this, result);
    }
}

/*! \qmlmethod IntentRequest::sendErrorReply(string errorMessage)

    IntentHandlers can use this function to indicate that they are unable to handle a request that
    they received via IntentHandler::requestReceived(), stating the reason in \a errorMessage.

    Only either sendReply() or sendErrorReply() can be used on a single IntentRequest.

    \note This function only works for request objects received from IntentHandler::requestReceived().
          It will simply do nothing on requests created by IntentClient::sendIntentRequest().

    \sa sendReply
*/
void IntentClientRequest::sendErrorReply(const QString &errorMessage)
{
    if (m_direction == Direction::ToSystem) {
        qmlWarning(this) << "Calling IntentRequest::sendErrorReply on requests originating from this application is a no-op.";
        return;
    }
    if (m_broadcast) {
        qmlWarning(this) << "Calling IntentRequest::sendErrorReply on broadcast requests is a no-op.";
        return;
    }
    IntentClient *ic = IntentClient::instance();

    if (QThread::currentThread() != ic->thread()) {
        QPointer<IntentClientRequest> that(this);

        ic->metaObject()->invokeMethod(ic, [that, ic, errorMessage]()
        { if (that) ic->errorReplyFromApplication(that.data(), errorMessage); },
        Qt::QueuedConnection);
    } else {
        ic->errorReplyFromApplication(this, errorMessage);
    }
}

void IntentClientRequest::startTimeout(int timeout)
{
    if (timeout <= 0)
        return;

    QTimer::singleShot(timeout, this, [this, timeout]() {
        if (!m_finished) {
            if (direction() == Direction::ToApplication)
                sendErrorReply(qSL("Intent request to application timed out after %1 ms").arg(timeout));
            else
                setErrorMessage(qSL("No reply received from Intent server after %1 ms").arg(timeout));
        }
    });
}

void IntentClientRequest::connectNotify(const QMetaMethod &signal)
{
    // take care of connects happening after the request is already finished:
    // re-emit the finished signal in this case (this shouldn't happen in practice, but better be
    // safe than sorry)
    if (signal == QMetaMethod::fromSignal(&IntentClientRequest::replyReceived)) {
        if (direction() == Direction::ToApplication) {
            qmlWarning(this) << "Connecting to IntentRequest::replyReceived on requests received "
                                "by IntentHandlers is a no-op.";
        } else if (m_finished) {
            QMetaObject::invokeMethod(this, &IntentClientRequest::doFinish, Qt::QueuedConnection);
        }
    }
}

IntentClientRequest::IntentClientRequest(Direction direction, const QString &requestingApplicationId,
                                         const QUuid &id, const QString &intentId,
                                         const QString &applicationId, const QVariantMap &parameters,
                                         bool broadcast)
    : QObject()
    , m_direction(direction)
    , m_id(id)
    , m_intentId(intentId)
    , m_requestingApplicationId(requestingApplicationId)
    , m_applicationId(applicationId)
    , m_parameters(parameters)
    , m_broadcast(broadcast)
{ }

void IntentClientRequest::setRequestId(const QUuid &requestId)
{
    if (m_id != requestId) {
        m_id = requestId;
        emit requestIdChanged();
    }
}

void IntentClientRequest::setResult(const QVariantMap &result)
{
    if (m_result != result)
        m_result = result;
    m_succeeded = true;
    doFinish();
}

void IntentClientRequest::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage)
        m_errorMessage = errorMessage;
    m_succeeded = false;
    doFinish();
}

void IntentClientRequest::doFinish()
{
    m_finished = true;
    emit replyReceived();
    // We need to disconnect all JS handlers now, because otherwise the request object would
    // never be garbage collected (the signal connections increase the use-counter).
    disconnect(this, &IntentClientRequest::replyReceived, nullptr, nullptr);
}

QT_END_NAMESPACE_AM

#include "moc_intentclientrequest.cpp"
