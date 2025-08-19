// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "collectionmodel.h"
#include "secretserviceclient.h"
#include "statetracker.h"

#include <QTimer>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

CollectionModel::CollectionModel(SecretServiceClient *secretServiceClient, QObject *parent)
    : QAbstractListModel(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(StateTracker::instance(), &StateTracker::serviceConnectedChanged, this, [this](bool connected) {
        if (m_currentCollectionPath.isEmpty()) {
            KConfigGroup windowGroup(KSharedConfig::openStateConfig(), QStringLiteral("MainWindow"));
            setCollectionPath(windowGroup.readEntry(QStringLiteral("CurrentCollectionPath"), QString()));
        }
        if (m_currentCollectionPath.isEmpty()) {
            const auto &collections = m_secretServiceClient->listCollections();
            setCollectionPath(collections.first().dbusPath);
        }

        if (connected) {
            loadWallet();
        } else {
            beginResetModel();
            m_items.clear();
            endResetModel();
        }
        Q_EMIT collectionNameChanged(collectionName());
        Q_EMIT collectionPathChanged(collectionPath());
    });

    connect(m_secretServiceClient, &SecretServiceClient::collectionDeleted, this, [this](const QDBusObjectPath &path) {
        if (path.path() == m_currentCollectionPath) {
            setCollectionPath(QString());
        }
    });

    connect(m_secretServiceClient, &SecretServiceClient::collectionLocked, this, [this](const QDBusObjectPath &path) {
        if (path.path() == m_currentCollectionPath) {
            StateTracker::instance()->setState(StateTracker::CollectionLocked);
        }
    });

    connect(m_secretServiceClient, &SecretServiceClient::collectionUnlocked, this, [this](const QDBusObjectPath &path) {
        if (path.path() == m_currentCollectionPath) {
            StateTracker::instance()->setState(StateTracker::CollectionReady);
            loadWallet();
        }
    });

    if (StateTracker::instance()->status() & StateTracker::ServiceConnected) {
        KConfigGroup windowGroup(KSharedConfig::openStateConfig(), QStringLiteral("MainWindow"));
        setCollectionPath(windowGroup.readEntry(QStringLiteral("CurrentCollectionPath"), QString()));
    }
}

CollectionModel::~CollectionModel()
{
}

QString CollectionModel::collectionName() const
{
    if (!StateTracker::instance()->isServiceConnected() || !m_secretCollection) {
        return QString();
    }

    return QString::fromUtf8(secret_collection_get_label(m_secretCollection.get()));
}

QString CollectionModel::collectionPath() const
{
    return m_currentCollectionPath;
}

void CollectionModel::setCollectionPath(const QString &collectionPath)
{
    if (collectionPath == m_currentCollectionPath) {
        return;
    }

    if (m_notifyHandlerId > 0) {
        g_signal_handler_disconnect(m_secretCollection.get(), m_notifyHandlerId);
    }

    m_currentCollectionPath = collectionPath;

    if (collectionPath.isEmpty()) {
        beginResetModel();
        m_items.clear();
        endResetModel();
    } else if (StateTracker::instance()->isServiceConnected()) {
        loadWallet();
    }

    KConfigGroup windowGroup(KSharedConfig::openStateConfig(), QStringLiteral("MainWindow"));
    windowGroup.writeEntry(QStringLiteral("CurrentCollectionPath"), collectionPath);

    Q_EMIT collectionPathChanged(m_currentCollectionPath);
    Q_EMIT collectionNameChanged(collectionName());
}

void CollectionModel::lock()
{
    if (StateTracker::instance()->status() & StateTracker::CollectionLocked) {
        return;
    }

    if (!StateTracker::instance()->isServiceConnected() || !m_secretCollection) {
        return;
    }

    m_secretServiceClient->lockCollection(m_currentCollectionPath);
}

void CollectionModel::unlock()
{
    if (!(StateTracker::instance()->status() & StateTracker::CollectionLocked)) {
        return;
    }

    if (!StateTracker::instance()->isServiceConnected() || !m_secretCollection) {
        return;
    }

    m_secretServiceClient->unlockCollection(m_currentCollectionPath);
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[FolderRole] = "folder";
    roleNames[DbusPathRole] = "dbusPath";

    return roleNames;
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_items.count();
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
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
    Q_UNUSED(collection)
    if (g_strcmp0(pspec->name, "items") != 0) {
        return;
    }

    CollectionModel *collectionModel = (CollectionModel *)inst;
    collectionModel->refreshWallet();
}

void CollectionModel::loadWallet()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    m_secretCollection = SecretCollectionPtr(m_secretServiceClient->retrieveCollection(m_currentCollectionPath));

    if (!m_secretCollection) {
        return;
    }

    refreshWallet();
}

void CollectionModel::refreshWallet()
{
    if (!m_secretCollection) {
        return;
    }

    if (m_notifyHandlerId > 0) {
        g_signal_handler_disconnect(m_secretCollection.get(), m_notifyHandlerId);
    }

    StateTracker::instance()->clearError();

    StateTracker::instance()->clearState(StateTracker::CollectionLocked);
    StateTracker::instance()->clearState(StateTracker::CollectionReady);
    beginResetModel();
    m_items.clear();

    if (secret_collection_get_locked(m_secretCollection.get())) {
        StateTracker::instance()->setState(StateTracker::CollectionLocked);
        endResetModel();
        return;
    }

    GError *error = nullptr;
    secret_collection_load_items_sync(m_secretCollection.get(), nullptr, &error);

    if (error) {
        StateTracker::instance()->setError(StateTracker::CollectionLoadError, QString::fromUtf8(error->message));
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
                entry.folder = i18nc("@info Other type of secret", "Other");
            }

            m_items << entry;
        }
    }

    StateTracker::instance()->setState(StateTracker::CollectionReady);

    endResetModel();

    m_notifyHandlerId = g_signal_connect(m_secretCollection.get(), "notify", G_CALLBACK(onCollectionNotify), this);
}

#include "moc_collectionmodel.cpp"
