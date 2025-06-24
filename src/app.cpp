// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "app.h"
#include "secretserviceclient.h"
#include "walletmodel.h"
#include <KSharedConfig>
#include <KWindowConfig>
#include <QQuickWindow>

App::App(QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(new SecretServiceClient(this))
{
    m_walletsModel = new WalletsModel(m_secretServiceClient, this);
    m_walletModel = new WalletModel(m_secretServiceClient, this);
    m_secretItemProxy = new SecretItemProxy(m_secretServiceClient, this);
    m_secretItemForContextMenu = new SecretItemProxy(m_secretServiceClient, this);
    connect(m_walletModel, &WalletModel::collectionPathChanged, m_walletsModel, &WalletsModel::setCollectionPath);
    m_walletsModel->setCollectionPath(m_walletModel->collectionPath());
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
    return m_secretServiceClient->stateTracker();
}

WalletsModel *App::walletsModel() const
{
    return m_walletsModel;
}

WalletModel *App::walletModel() const
{
    return m_walletModel;
}

SecretItemProxy *App::secretItem() const
{
    return m_secretItemProxy;
}

SecretItemProxy *App::secretItemForContextMenu() const
{
    return m_secretItemForContextMenu;
}

void App::restoreWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);

    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);
}

void App::saveWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);
    KWindowConfig::saveWindowPosition(window, windowGroup);
    KWindowConfig::saveWindowSize(window, windowGroup);

    dataResource.sync();
}

QString App::sidebarState() const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    return windowGroup.readEntry(QStringLiteral("sidebarState"), QString());
}

void App::setSidebarState(const QString &state)
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    windowGroup.writeEntry(QStringLiteral("sidebarState"), state);

    dataResource.sync();
    Q_EMIT sidebarStateChanged();
}

#include "moc_app.cpp"
