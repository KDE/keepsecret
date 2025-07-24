// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "app.h"
#include "collectionmodel.h"
#include "secretserviceclient.h"
#include <KSharedConfig>
#include <KWindowConfig>
#include <QQuickWindow>

App::App(QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(new SecretServiceClient(this))
{
    m_collectionsModel = new CollectionsModel(m_secretServiceClient, this);
    m_collectionModel = new CollectionModel(m_secretServiceClient, this);
    m_secretItemProxy = new SecretItemProxy(m_secretServiceClient, this);
    m_secretItemForContextMenu = new SecretItemProxy(m_secretServiceClient, this);

    m_collectionsModel->setCollectionPath(m_collectionModel->collectionPath());

    connect(m_collectionModel, &CollectionModel::collectionPathChanged, m_collectionsModel, &CollectionsModel::setCollectionPath);
}

App::~App()
{
}

SecretServiceClient *App::secretService() const
{
    return m_secretServiceClient;
}

StateTracker *App::stateTracker() const
{
    return StateTracker::instance();
}

CollectionsModel *App::collectionsModel() const
{
    return m_collectionsModel;
}

CollectionModel *App::collectionModel() const
{
    return m_collectionModel;
}

SecretItemProxy *App::secretItem() const
{
    return m_secretItemProxy;
}

SecretItemProxy *App::secretItemForContextMenu() const
{
    return m_secretItemForContextMenu;
}

QString App::sidebarState() const
{
    KConfigGroup windowGroup(KSharedConfig::openStateConfig(), QStringLiteral("Window"));
    return windowGroup.readEntry(QStringLiteral("sidebarState"), QString());
}

void App::setSidebarState(const QString &state)
{
    KConfigGroup windowGroup(KSharedConfig::openStateConfig(), QStringLiteral("Window"));
    windowGroup.writeEntry(QStringLiteral("sidebarState"), state);

    Q_EMIT sidebarStateChanged();
}

#include "moc_app.cpp"
