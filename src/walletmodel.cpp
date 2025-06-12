// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "walletmodel.h"
#include "secretserviceclient.h"

#include <QTimer>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

WalletModel::WalletModel(SecretServiceClient *secretServiceClient, QObject *parent)
    : QAbstractListModel(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(m_secretServiceClient, &SecretServiceClient::statusChanged, this, [this](SecretServiceClient::Status status) {
        if (status == SecretServiceClient::Connected) {
            loadWallet();
        } else {
            beginResetModel();
            m_items.clear();
            endResetModel();
        }
    });

    QTimer::singleShot(0, this, [this]() {
        KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
        KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
        setCurrentWallet(windowGroup.readEntry(QStringLiteral("lastOpenWallet"), QString()));
    });
}

WalletModel::~WalletModel()
{
}

QString WalletModel::currentWallet() const
{
    return m_currentWallet;
}

void WalletModel::setCurrentWallet(const QString &wallet)
{
    if (wallet == m_currentWallet) {
        return;
    }

    m_currentWallet = wallet;

    // loadWallet();
    if (m_secretServiceClient->status() == SecretServiceClient::Connected) {
        loadWallet();
    }

    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    windowGroup.writeEntry(QStringLiteral("lastOpenWallet"), wallet);

    dataResource.sync();

    Q_EMIT(currentWalletChanged(wallet));
}

bool WalletModel::isLocked() const
{
    if (!m_secretCollection) {
        return false;
    }
    return secret_collection_get_locked(m_secretCollection.get());
}

void WalletModel::lock()
{
    bool ok;
    m_secretServiceClient->lockCollection(m_currentWallet, &ok);
    loadWallet();
    Q_EMIT lockedChanged(isLocked());
}

void WalletModel::unlock()
{
    bool ok;
    m_secretServiceClient->unlockCollection(m_currentWallet, &ok);
    loadWallet();
    Q_EMIT lockedChanged(isLocked());
}

QHash<int, QByteArray> WalletModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[FolderRole] = "folder";
    roleNames[DbusPathRole] = "dbusPath";

    return roleNames;
}

int WalletModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_items.count();
}

QVariant WalletModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_items.count() - 1) {
        return {};
    }

    const auto item = m_items[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return item.label;
    case FolderRole:
        return item.folder;
    case DbusPathRole:
        return item.dbusPath;
    }

    return {};
}

void WalletModel::loadWallet()
{
    if (m_secretServiceClient->status() != SecretServiceClient::Connected) {
        return;
    }

    m_secretCollection = SecretCollectionPtr(m_secretServiceClient->retrieveCollection(m_currentWallet));

    if (!m_secretCollection) {
        return;
    }

    beginResetModel();
    bool ok;
    const QStringList folders = m_secretServiceClient->listFolders(m_currentWallet, &ok);
    m_items.clear();

    GError *error = nullptr;
    secret_collection_load_items_sync(m_secretCollection.get(), nullptr, &error);

    if (error) {
        Q_EMIT WalletModel::error(QString::fromUtf8(error->message));
        g_error_free(error);
        endResetModel();
        return;
    }

    GListPtr list = GListPtr(secret_collection_get_items(m_secretCollection.get()));
    if (list) {
        for (GList *l = list.get(); l != nullptr; l = l->next) {
            SecretItemPtr item = SecretItemPtr(SECRET_ITEM(l->data));
            Entry entry;
            entry.label = QString::fromUtf8(secret_item_get_label(item.get()));
            entry.dbusPath = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(item.get())));
            entry.folder = QString();

            GHashTablePtr attributes = GHashTablePtr(secret_item_get_attributes(item.get()));

            // Retrieve "server" value
            const char *server = static_cast<gchar *>(g_hash_table_lookup(attributes.get(), "server"));
            if (server) {
                entry.folder = QString::fromUtf8(server);
            } else {
                // If there is no "server", try with "service"
                const char *service = static_cast<gchar *>(g_hash_table_lookup(attributes.get(), "service"));
                if (service) {
                    entry.folder = QString::fromUtf8(service);
                }
            }
            if (entry.folder.isEmpty()) {
                entry.folder = i18n("Other");
            }

            m_items << entry;
        }
    }

    endResetModel();
}

#include "moc_walletmodel.cpp"
