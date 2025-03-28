// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "app.h"
#include "itemsmodel.h"
#include "secretserviceclient.h"
#include <KSharedConfig>
#include <KWindowConfig>
#include <QQuickWindow>

App::App(QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(new SecretServiceClient(this))
{
    m_walletsModel = new WalletsModel(m_secretServiceClient, this);
    m_itemsModel = new ItemsModel(m_secretServiceClient, this);
    m_secretItemProxy = new SecretItemProxy(m_secretServiceClient, this);
}

App::~App()
{
}

WalletsModel *App::walletsModel() const
{
    return m_walletsModel;
}

ItemsModel *App::itemsModel() const
{
    return m_itemsModel;
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
    const qreal leadingSidebarWidth = window->property("leadingSidebarWidth").toReal();
    if (leadingSidebarWidth > 0) {
        windowGroup.writeEntry(QStringLiteral("leadingSidebarWidth"), leadingSidebarWidth);
    }
    windowGroup.writeEntry(QStringLiteral("trailingSidebarWidth"), window->property("trailingSidebarWidth").toString());

    dataResource.sync();
}

#include "moc_app.cpp"
