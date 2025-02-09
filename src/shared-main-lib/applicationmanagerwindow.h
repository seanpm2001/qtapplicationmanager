// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <memory>

#include <QtGui/QColor>
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>
#include <QtAppManCommon/global.h>


QT_FORWARD_DECLARE_CLASS(QQmlComponentAttached)
QT_FORWARD_DECLARE_CLASS(QQuickItem)

QT_BEGIN_NAMESPACE_AM

class ApplicationManagerWindowImpl;


class ApplicationManagerWindow : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_CLASSINFO("AM-QmlType", "QtApplicationManager.Application/ApplicationManagerWindow 2.0")
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool inProcess READ isInProcess CONSTANT FINAL)
    Q_PROPERTY(QObject *backingObject READ backingObject CONSTANT FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT FINAL)

    // QWindow properties
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    //Q_PROPERTY(Qt::WindowModality modality READ modality WRITE setModality NOTIFY modalityChanged FINAL)
    //Q_PROPERTY(Qt::WindowFlags flags READ flags WRITE setFlags FINAL)
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(int minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(int minimumHeight READ minimumHeight WRITE setMinimumHeight NOTIFY minimumHeightChanged FINAL)
    Q_PROPERTY(int maximumWidth READ maximumWidth WRITE setMaximumWidth NOTIFY maximumWidthChanged FINAL)
    Q_PROPERTY(int maximumHeight READ maximumHeight WRITE setMaximumHeight NOTIFY maximumHeightChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    //Q_PROPERTY(Visibility visibility READ visibility WRITE setVisibility NOTIFY visibilityChanged FINAL)
    //Q_PROPERTY(Qt::ScreenOrientation contentOrientation READ contentOrientation WRITE reportContentOrientationChange NOTIFY contentOrientationChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)

    Q_PROPERTY(QQmlListProperty<QObject> data READ data NOTIFY dataChanged FINAL)

    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit ApplicationManagerWindow(QObject *parent = nullptr);
    ~ApplicationManagerWindow() override;

    bool isInProcess() const;
    QObject *backingObject() const;

    QQmlListProperty<QObject> data();
    Q_SIGNAL void dataChanged();

    QQuickItem *contentItem();

    void classBegin() override;
    void componentComplete() override;

    QString title() const;
    void setTitle(const QString &title);
    Q_SIGNAL void titleChanged();
    int x() const;
    void setX(int x);
    Q_SIGNAL void xChanged();
    int y() const;
    void setY(int y);
    Q_SIGNAL void yChanged();
    int width() const;
    void setWidth(int w);
    Q_SIGNAL void widthChanged();
    int height() const;
    void setHeight(int h);
    Q_SIGNAL void heightChanged();
    int minimumWidth() const;
    void setMinimumWidth(int minw);
    Q_SIGNAL void minimumWidthChanged();
    int minimumHeight() const;
    void setMinimumHeight(int minh);
    Q_SIGNAL void minimumHeightChanged();
    int maximumWidth() const;
    void setMaximumWidth(int maxw);
    Q_SIGNAL void maximumWidthChanged();
    int maximumHeight() const;
    void setMaximumHeight(int maxh);
    Q_SIGNAL void maximumHeightChanged();
    bool isVisible() const;
    void setVisible(bool visible);
    Q_SIGNAL void visibleChanged();
    qreal opacity() const;
    void setOpacity(qreal opacity);
    Q_SIGNAL void opacityChanged();
    QColor color() const;
    void setColor(const QColor &c);
    Q_SIGNAL void colorChanged();
    bool isActive() const;
    Q_SIGNAL void activeChanged();

    Q_INVOKABLE bool setWindowProperty(const QString &name, const QVariant &value);
    Q_INVOKABLE QVariant windowProperty(const QString &name) const;
    Q_INVOKABLE QVariantMap windowProperties() const;
    Q_SIGNAL void windowPropertyChanged(const QString &name, const QVariant &value);

    Q_INVOKABLE void close();
    Q_INVOKABLE void showFullScreen();
    Q_INVOKABLE void showMaximized();
    Q_INVOKABLE void showNormal();

    // following QWindow slots aren't implemented yet:
    //    void hide()
    //    void lower()
    //    void raise()
    //    void show()
    //    void showMinimized()

    ApplicationManagerWindowImpl *implementation();

protected:
    std::unique_ptr<ApplicationManagerWindowImpl> m_impl;

private:
    static void data_append(QQmlListProperty<QObject> *property, QObject *object);
    static qsizetype data_count(QQmlListProperty<QObject> *property);
    static QObject *data_at(QQmlListProperty<QObject> *property, qsizetype index);
    static void data_clear(QQmlListProperty<QObject> *property);
};

QT_END_NAMESPACE_AM
