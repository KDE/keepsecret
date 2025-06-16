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
        setError(NoError, QString());
        if (status == SecretServiceClient::Connected) {
            setStatus(Connected);
            loadWallet();
        } else {
            setStatus(Disconnected);
            beginResetModel();
            m_items.clear();
            endResetModel();
        }
    });

    // FIXME: needed the timer?
    QTimer::singleShot(0, this, [this]() {
        KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
        KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
        setCurrentWallet(windowGroup.readEntry(QStringLiteral("lastOpenWallet"), QString()));
    });
}

WalletModel::~WalletModel()
{
}

WalletModel::Status WalletModel::status() const
{
    return m_status;
}

void WalletModel::setStatus(WalletModel::Status status)
{
    if (status == m_status) {
        return;
    }

    qWarning() << "WalletModel: Setting status" << status;

    m_status = status;
    Q_EMIT statusChanged(status);
}

WalletModel::Operations WalletModel::operations() const
{
    return m_operations;
}

void WalletModel::setOperations(WalletModel::Operations operations)
{
    if (operations == m_operations) {
        return;
    }

    qWarning() << "WalletModel: Setting operations" << operations;

    m_operations = operations;
    Q_EMIT operationsChanged(operations);
}

void WalletModel::setOperation(WalletModel::Operation operation)
{
    setOperations(m_operations | operation);
}

void WalletModel::clearOperation(WalletModel::Operation operation)
{
    setOperations(m_operations & ~operation);
}

WalletModel::Error WalletModel::error() const
{
    return m_error;
}

QString WalletModel::errorMessage() const
{
    return m_errorMessage;
}

void WalletModel::setError(WalletModel::Error error, const QString &errorMessage)
{
    if (error != m_error) {
        m_error = error;
        Q_EMIT errorChanged(error);
    }

    if (errorMessage != m_errorMessage) {
        m_errorMessage = errorMessage;
        Q_EMIT errorMessageChanged(errorMessage);
    }
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

    if (m_notifyHandlerId > 0) {
        g_signal_handler_disconnect(m_secretCollection.get(), m_notifyHandlerId);
    }

    m_currentWallet = wallet;

    if (m_secretServiceClient->status() == SecretServiceClient::Connected) {
        loadWallet();
    }

    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-main"));
    windowGroup.writeEntry(QStringLiteral("lastOpenWallet"), wallet);

    dataResource.sync();

    Q_EMIT(currentWalletChanged(wallet));
}

void WalletModel::lock()
{
    bool ok;
    m_secretServiceClient->lockCollection(m_currentWallet, &ok);
    if (ok) {
        setStatus(Locked);
    }
    refreshWallet();
}

void WalletModel::unlock()
{
    bool ok;
    m_secretServiceClient->unlockCollection(m_currentWallet, &ok);
    if (ok) {
        setStatus(m_status & (~Locked | Connected));
    }
    refreshWallet();
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

static void onCollectionNotify(SecretCollection *collection, GParamSpec *pspec, gpointer inst)
{
    if (g_strcmp0(pspec->name, "items") != 0) {
        return;
    }
    WalletModel *walletModel = (WalletModel *)inst;
    walletModel->refreshWallet();
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

    refreshWallet();
}

void WalletModel::refreshWallet()
{
    if (!m_secretCollection) {
        return;
    }

    if (m_notifyHandlerId > 0) {
        g_signal_handler_disconnect(m_secretCollection.get(), m_notifyHandlerId);
    }

    setError(NoError, QString());

    setStatus(Connected);
    beginResetModel();
    bool ok;
    m_items.clear();

    if (secret_collection_get_locked(m_secretCollection.get())) {
        setStatus(Locked);
        endResetModel();
        return;
    }

    GError *error = nullptr;
    secret_collection_load_items_sync(m_secretCollection.get(), nullptr, &error);

    if (error) {
        setError(LoadFailed, QString::fromUtf8(error->message));
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
        if (m_items.length() > 0) {
            setStatus(Ready);
        }
    } else {
        setError(LoadFailed, i18n("Unable to load Entries."));
    }

    endResetModel();
    m_notifyHandlerId = g_signal_connect(m_secretCollection.get(), "notify", G_CALLBACK(onCollectionNotify), this);
}

#include "moc_walletmodel.cpp"
