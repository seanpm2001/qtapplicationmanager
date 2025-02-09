// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlParserStatus>
#include <QtAppManCommon/global.h>

QT_BEGIN_NAMESPACE_AM

class NotificationImpl;


class Notification : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_CLASSINFO("AM-QmlType", "QtApplicationManager/Notification 2.0")
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(uint notificationId READ notificationId NOTIFY notificationIdChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)

    Q_PROPERTY(QString summary READ summary WRITE setSummary NOTIFY summaryChanged FINAL)
    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged FINAL)
    Q_PROPERTY(QUrl icon READ icon WRITE setIcon NOTIFY iconChanged FINAL)
    Q_PROPERTY(QUrl image READ image WRITE setImage NOTIFY imageChanged FINAL)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged FINAL)
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY priorityChanged FINAL)
    Q_PROPERTY(bool acknowledgeable READ isAcknowledgeable WRITE setAcknowledgeable NOTIFY acknowledgeableChanged FINAL)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged FINAL)
    Q_PROPERTY(bool sticky READ isSticky WRITE setSticky NOTIFY stickyChanged FINAL)
    Q_PROPERTY(bool showProgress READ isShowingProgress WRITE setShowProgress NOTIFY showProgressChanged FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(QVariantList actions READ actions WRITE setActions NOTIFY actionsChanged FINAL)
    Q_PROPERTY(bool showActionsAsIcons READ showActionsAsIcons WRITE setShowActionsAsIcons NOTIFY showActionsAsIconsChanged FINAL)
    Q_PROPERTY(bool dismissOnAction READ dismissOnAction WRITE setDismissOnAction NOTIFY dismissOnActionChanged FINAL)
    Q_PROPERTY(QVariantMap extended READ extended WRITE setExtended NOTIFY extendedChanged FINAL)

public:
    enum Priority { Low, Normal, Critical };
    Q_ENUM(Priority)

    Notification(QObject *parent = nullptr, const QString &applicationId = { });
    ~Notification() override;

    uint notificationId() const;
    QString summary() const;
    QString body() const;
    QUrl icon() const;
    QUrl image() const;
    QString category() const;
    int priority() const;
    bool isAcknowledgeable() const;
    int timeout() const;
    bool isSticky() const;
    bool isShowingProgress() const;
    qreal progress() const;
    QVariantList actions() const;
    bool showActionsAsIcons() const;
    bool dismissOnAction() const;
    QVariantMap extended() const;
    bool isVisible() const;

    Q_INVOKABLE void show();
    Q_INVOKABLE void update();
    Q_INVOKABLE void hide();

public slots:
    void setSummary(const QString &summary);
    void setBody(const QString &boy);
    void setIcon(const QUrl &icon);
    void setImage(const QUrl &image);
    void setCategory(const QString &category);
    void setPriority(int priority);
    void setAcknowledgeable(bool acknowledgeable);
    void setTimeout(int timeout);
    void setSticky(bool sticky);
    void setShowProgress(bool showProgress);
    void setProgress(qreal progress);
    void setActions(const QVariantList &actions);
    void setShowActionsAsIcons(bool showActionsAsIcons);
    void setDismissOnAction(bool dismissOnAction);
    void setExtended(QVariantMap extended);
    void setVisible(bool visible);

signals:
    void notificationIdChanged(uint notificationId);
    void summaryChanged(const QString &summary);
    void bodyChanged(const QString &body);
    void iconChanged(const QUrl &icon);
    void imageChanged(const QUrl &image);
    void categoryChanged(const QString &category);
    void priorityChanged(int priority);
    void acknowledgeableChanged(bool acknowledgeable);
    void timeoutChanged(int timeout);
    void stickyChanged(bool sticky);
    void showProgressChanged(bool showProgress);
    void progressChanged(qreal progress);
    void actionsChanged(const QVariantList &actions);
    void showActionsAsIconsChanged(bool showActionsAsIcons);
    void dismissOnActionChanged(bool dismissOnAction);
    void extendedChanged(QVariantMap extended);
    void visibleChanged(bool visible);

    void acknowledged();
    void actionTriggered(const QString &actionId);

public:
    // not public API, but QQmlParserStatus pure-virtual overrides
    void classBegin() override;
    void componentComplete() override;

    void setId(uint notificationId);
    void close();
    void triggerAction(const QString &actionId);

    QVariantMap libnotifyHints() const;
    QStringList libnotifyActionList() const;

private:
    void updateNotification();

private:
    uint m_id = 0;
    QString m_summary;
    QString m_body;
    QUrl m_icon;
    QUrl m_image;
    QString m_category;
    int m_priority = Normal;
    bool m_acknowledgeable = false;
    int m_timeout = defaultTimeout;
    bool m_showProgress = false;
    qreal m_progress = -1;
    QVariantList m_actions;
    bool m_showActionsAsIcons = false;
    bool m_dismissOnAction = false;
    QVariantMap m_extended;
    bool m_visible = false;

    bool m_usedByQml = false;
    bool m_componentComplete = false;

    static const int defaultTimeout = 2000;

private:
    Q_DISABLE_COPY_MOVE(Notification)

    std::unique_ptr<NotificationImpl> m_impl;
    friend class NotificationImpl;
    friend class ApplicationMain;
};

QT_END_NAMESPACE_AM
