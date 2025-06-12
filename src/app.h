// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "secretitemproxy.h"
#include "secretserviceclient.h"
#include "walletmodel.h"
#include "walletsmodel.h"

class QQuickWindow;

class SecretServiceClient;

class App : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(WalletsModel *walletsModel READ walletsModel CONSTANT)
    Q_PROPERTY(WalletModel *walletModel READ walletModel CONSTANT)
    Q_PROPERTY(SecretItemProxy *secretItem READ secretItem CONSTANT)
    Q_PROPERTY(QString sidebarState READ sidebarState WRITE setSidebarState NOTIFY sidebarStateChanged)

public:
    App(QObject *parent = nullptr);
    ~App();

    WalletsModel *walletsModel() const;
    WalletModel *walletModel() const;
    SecretItemProxy *secretItem() const;

    QString sidebarState() const;
    void setSidebarState(const QString &state);

    // Restore current window geometry
    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QStringLiteral("main")) const;
    // Save current window geometry
    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QStringLiteral("main")) const;

Q_SIGNALS:
    void sidebarStateChanged();

private:
    SecretServiceClient *m_secretServiceClient = nullptr;
    WalletsModel *m_walletsModel = nullptr;
    WalletModel *m_walletModel = nullptr;
    SecretItemProxy *m_secretItemProxy = nullptr;
};
