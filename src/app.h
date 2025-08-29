// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include <QObject>

#include "collectionmodel.h"
#include "collectionsmodel.h"
#include "secretitemproxy.h"
#include "secretserviceclient.h"
#include "statetracker.h"

class SecretServiceClient;

class App : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SecretServiceClient *secretService READ secretService CONSTANT)
    Q_PROPERTY(StateTracker *stateTracker READ stateTracker CONSTANT)
    Q_PROPERTY(CollectionsModel *collectionsModel READ collectionsModel CONSTANT)
    Q_PROPERTY(CollectionModel *collectionModel READ collectionModel CONSTANT)
    Q_PROPERTY(SecretItemProxy *secretItem READ secretItem CONSTANT)
    Q_PROPERTY(SecretItemProxy *secretItemForContextMenu READ secretItemForContextMenu CONSTANT)
    Q_PROPERTY(QString sidebarState READ sidebarState WRITE setSidebarState NOTIFY sidebarStateChanged)

public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

    SecretServiceClient *secretService() const;
    StateTracker *stateTracker() const;
    CollectionsModel *collectionsModel() const;
    CollectionModel *collectionModel() const;
    SecretItemProxy *secretItem() const;
    SecretItemProxy *secretItemForContextMenu() const;

    QString sidebarState() const;
    void setSidebarState(const QString &state);

Q_SIGNALS:
    void sidebarStateChanged();

private:
    SecretServiceClient *m_secretServiceClient = nullptr;
    CollectionsModel *m_collectionsModel = nullptr;
    CollectionModel *m_collectionModel = nullptr;
    SecretItemProxy *m_secretItemProxy = nullptr;
    SecretItemProxy *m_secretItemForContextMenu = nullptr;
};
