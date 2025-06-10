// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "app.h"
#include "secretserviceclient.h"
#include "walletmodel.h"
#include <KSharedConfig>
#include <KWindowConfig>
#include <QQuickWindow>
#include <qt6/QtCore/qstringliteral.h>

App::App(QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(new SecretServiceClient(this))
{
    m_walletsModel = new WalletsModel(m_secretServiceClient, this);
    m_walletModel = new WalletModel(m_secretServiceClient, this);
    m_secretItemProxy = new SecretItemProxy(m_secretServiceClient, this);
}

App::~App()
{
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

void App::restoreWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);

    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);

    // Sidebars
    window->setProperty("leadingSidebarWidth", windowGroup.readEntry(QStringLiteral("leadingSidebarWidth"), -1));
    window->setProperty("trailingSidebarWidth", windowGroup.readEntry(QStringLiteral("trailingSidebarWidth"), -1));
}

void App::saveWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);
    KWindowConfig::saveWindowPosition(window, windowGroup);
    KWindowConfig::saveWindowSize(window, windowGroup);

    // Sidebars
    const int leadingSidebarWidth = window->property("leadingSidebarWidth").toInt();
    if (leadingSidebarWidth > 0) {
        windowGroup.writeEntry(QStringLiteral("leadingSidebarWidth"), leadingSidebarWidth);
    }

    const int trailingSidebarWidth = window->property("trailingSidebarWidth").toInt();
    if (trailingSidebarWidth > 0) {
        windowGroup.writeEntry(QStringLiteral("trailingSidebarWidth"), trailingSidebarWidth);
    }

    dataResource.sync();
}

QByteArray App::sidebarState() const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    return QByteArray::fromBase64(windowGroup.readEntry(QStringLiteral("sidebarState"), QString()).toUtf8());
}

void App::setSidebarState(const QByteArray &state)
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    windowGroup.writeEntry(QStringLiteral("sidebarState"), state.toBase64());

    dataResource.sync();
}

#include "moc_app.cpp"
