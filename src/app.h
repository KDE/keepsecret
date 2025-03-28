// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "itemsmodel.h"
#include "secretserviceclient.h"
#include "walletsmodel.h"

class QQuickWindow;

class SecretServiceClient;

class App : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(WalletsModel *walletsModel READ walletsModel CONSTANT)
    Q_PROPERTY(ItemsModel *itemsModel READ itemsModel CONSTANT)

public:
    App(QObject *parent = nullptr);
    ~App();

    WalletsModel *walletsModel() const;
    ItemsModel *itemsModel() const;

    // Restore current window geometry
    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QStringLiteral("main")) const;
    // Save current window geometry
    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QStringLiteral("main")) const;

private:
    SecretServiceClient *m_secretServiceClient = nullptr;
    WalletsModel *m_walletsModel = nullptr;
    ItemsModel *m_itemsModel = nullptr;
};
