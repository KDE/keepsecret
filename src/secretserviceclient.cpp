/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "secretserviceclient.h"
#include "kwallets_debug.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QEventLoop>
#include <QTimer>

static bool wasErrorFree(GError **error)
{
    if (!*error) {
        return true;
    }
    qCWarning(KWALLETS_LOG) << QString::fromUtf8((*error)->message);
    g_error_free(*error);
    *error = nullptr;
    return false;
}
SecretServiceClient::SecretServiceClient(QObject *parent)
    : QObject(parent)
{
    m_serviceBusName = QStringLiteral("org.freedesktop.secrets");

    m_collectionDirtyTimer = new QTimer(this);
    m_collectionDirtyTimer->setSingleShot(true);
    m_collectionDirtyTimer->setInterval(100);
    connect(m_collectionDirtyTimer, &QTimer::timeout, this, [this]() {
        for (const QString &collection : std::as_const(m_dirtyCollections)) {
            Q_EMIT collectionDirty(collection);
        }
        m_dirtyCollections.clear();
    });

    m_serviceWatcher = new QDBusServiceWatcher(m_serviceBusName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &SecretServiceClient::onServiceOwnerChanged);

    // Unconditionally try to connect to the service without checking it exists:
    // it will try to dbus-activate it if not running
    if (attemptConnection()) {
        bool ok = false;
        for (const QString &collection : listCollections(&ok)) {
            watchCollection(collection, &ok);
        }
    }

    QDBusConnection bus = QDBusConnection::sessionBus();

    // React to collections being created, either by us or somebody else
    bus.connect(m_serviceBusName,
                QStringLiteral("/org/freedesktop/secrets"),
                QStringLiteral("org.freedesktop.Secret.Service"),
                QStringLiteral("CollectionCreated"),
                this,
                SLOT(onCollectionCreated(QDBusObjectPath)));
    bus.connect(m_serviceBusName,
                QStringLiteral("/org/freedesktop/secrets"),
                QStringLiteral("org.freedesktop.Secret.Service"),
                QStringLiteral("CollectionDeleted"),
                this,
                SLOT(onCollectionDeleted(QDBusObjectPath)));
}

// Copied from
// https://github.com/frankosterfeld/qtkeychain/blob/main/qtkeychain/libsecret.cpp
// This is intended to be a data format compatible with QtKeychain
const SecretSchema *SecretServiceClient::qtKeychainSchema(void)
{
    static const SecretSchema schema = {
        "org.qt.keychain",
        SECRET_SCHEMA_DONT_MATCH_NAME,
        {{"user", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"server", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"type", SECRET_SCHEMA_ATTRIBUTE_STRING}}};

    return &schema;
}

QString SecretServiceClient::typeToString(SecretServiceClient::Type type)
{
    // Similar to QtKeychain implementation: adds the "map" datatype
    switch (type) {
    case SecretServiceClient::Base64:
        return QStringLiteral("base64");
    case SecretServiceClient::Binary:
        return QStringLiteral("binary");
    case SecretServiceClient::Map:
        return QStringLiteral("map");
    default:
        return QStringLiteral("plaintext");
    }
}

SecretServiceClient::Type SecretServiceClient::stringToType(const QString &typeName)
{
    if (typeName == QStringLiteral("binary")) {
        return SecretServiceClient::Binary;
    } else if (typeName == QStringLiteral("base64")) {
        return SecretServiceClient::Base64;
    } else if (typeName == QStringLiteral("map")) {
        return SecretServiceClient::Map;
    } else {
        return SecretServiceClient::PlainText;
    }
}

QString SecretServiceClient::collectionLabelForPath(const QDBusObjectPath &path)
{
    if (!attemptConnection()) {
        return {};
    }
    QDBusInterface collectionInterface(m_serviceBusName, path.path(), QStringLiteral("org.freedesktop.Secret.Collection"), QDBusConnection::sessionBus());

    if (!collectionInterface.isValid()) {
        qCWarning(KWALLETS_LOG) << "Failed to connect to the DBus collection object:" << path.path();
        return {};
    }

    QVariant reply = collectionInterface.property("Label");

    if (!reply.isValid()) {
        qCWarning(KWALLETS_LOG) << "Error reading label:" << collectionInterface.lastError();
        return {};
    }

    return reply.toString();
}

SecretCollection *SecretServiceClient::retrieveCollection(const QString &name)
{
    if (!attemptConnection()) {
        return nullptr;
    }

    GListPtr collections = GListPtr(secret_service_get_collections(m_service.get()));

    for (GList *l = collections.get(); l != nullptr; l = l->next) {
        SecretCollection *coll = SECRET_COLLECTION(l->data);
        const gchar *label = secret_collection_get_label(coll);
        if (QString::fromUtf8(label) == name) {
            SecretCollection *collection = coll;
            return collection;
        }
    }

    return nullptr;
}

SecretItemPtr SecretServiceClient::retrieveItem(const QString &dbusPath, const QString &collectionName, bool *ok)
{
    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    GListPtr list = GListPtr(secret_collection_get_items(collection));
    if (list) {
        *ok = true;
        for (GList *l = list.get(); l != nullptr; l = l->next) {
            SecretItem *item = static_cast<SecretItem *>(l->data);
            const QString path = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(item)));

            if (path == dbusPath) {
                return SecretItemPtr(item);
            }
        }
    } else {
        *ok = false;
    }

    return nullptr;
}

static void onServiceGetFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    SecretService *service = secret_service_get_finish(result, &error);

    if (wasErrorFree(&error)) {
        client->attemptConnectionFinished(service);
    } else {
        client->attemptConnectionFinished(nullptr);
    }
}

void SecretServiceClient::attemptConnectionFinished(SecretService *service)
{
    m_service.reset(service);
    if (service) {
        setStatus(SecretServiceClient::Connected);
    } else {
        setStatus(SecretServiceClient::Disconnected);
    }
}

bool SecretServiceClient::attemptConnection()
{
    if (m_service) {
        return true;
    }

    setStatus(Connecting);

    secret_service_get(static_cast<SecretServiceFlags>(SECRET_SERVICE_OPEN_SESSION | SECRET_SERVICE_LOAD_COLLECTIONS), nullptr, onServiceGetFinished, this);
    /*
        if (!ok || !m_service) {
            qCWarning(KWALLETS_LOG) << i18n("Could not connect to Secret Service");
            return false;
        }*/

    return true;
}

void SecretServiceClient::watchCollection(const QString &collectionName, bool *ok)
{
    if (m_watchedCollections.contains(collectionName)) {
        *ok = true;
        return;
    }

    SecretCollection *collection = retrieveCollection(collectionName);

    GObjectPtr<GDBusProxy> proxy = GObjectPtr<GDBusProxy>(G_DBUS_PROXY(collection));
    const QString path = QString::fromUtf8(g_strdup(g_dbus_proxy_get_object_path(proxy.get())));
    if (path.isEmpty()) {
        *ok = false;
        return;
    }

    QDBusConnection::sessionBus().connect(m_serviceBusName,
                                          path,
                                          QStringLiteral("org.freedesktop.Secret.Collection"),
                                          QStringLiteral("ItemChanged"),
                                          this,
                                          SLOT(onSecretItemChanged(QDBusObjectPath)));
    QDBusConnection::sessionBus().connect(m_serviceBusName,
                                          path,
                                          QStringLiteral("org.freedesktop.Secret.Collection"),
                                          QStringLiteral("ItemCreated"),
                                          this,
                                          SLOT(onSecretItemChanged(QDBusObjectPath)));
    QDBusConnection::sessionBus().connect(m_serviceBusName,
                                          path,
                                          QStringLiteral("org.freedesktop.Secret.Collection"),
                                          QStringLiteral("ItemDeleted"),
                                          this,
                                          SLOT(onSecretItemChanged(QDBusObjectPath)));

    m_watchedCollections.insert(collectionName);
    *ok = true;
}

void SecretServiceClient::onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(oldOwner);

    bool available = !newOwner.isEmpty();

    if (available && !m_service) {
        GError *error = nullptr;
        m_service = SecretServicePtr(
            secret_service_get_sync(static_cast<SecretServiceFlags>(SECRET_SERVICE_OPEN_SESSION | SECRET_SERVICE_LOAD_COLLECTIONS), nullptr, &error));

        available = wasErrorFree(&error);
    }

    if (!available) {
        setStatus(Disconnected);
        m_service.reset();
    }

    qDebug() << "Secret Service availability changed:" << (available ? "Available" : "Unavailable");
    Q_EMIT serviceChanged();
}

void SecretServiceClient::onCollectionCreated(const QDBusObjectPath &path)
{
    const QString label = collectionLabelForPath(path);
    if (label.isEmpty()) {
        return;
    }

    Q_EMIT collectionCreated(label);
    Q_EMIT collectionListDirty();
}

void SecretServiceClient::onCollectionDeleted(const QDBusObjectPath &path)
{
    Q_UNUSED(path);
    if (!attemptConnection()) {
        return;
    }

    // Only emitting collectionListDirty here as we can't know the actual label
    // of the collection as is already deleted
    Q_EMIT collectionListDirty();
}

void SecretServiceClient::onSecretItemChanged(const QDBusObjectPath &path)
{
    if (!attemptConnection()) {
        return;
    }

    // Don't trigger a reload if we did changes ourselves
    if (m_updateInProgress) {
        m_updateInProgress = false;
        return;
    }

    QStringList pieces = path.path().split(QStringLiteral("/"), Qt::SkipEmptyParts);

    // 6 items: /org/freedesktop/secrets/collection/collectionName/itemName
    if (pieces.length() != 6) {
        return;
    }
    pieces.pop_back();
    const QString collectionPath = QStringLiteral("/") % pieces.join(QStringLiteral("/"));

    const QString label = collectionLabelForPath(QDBusObjectPath(collectionPath));
    if (label.isEmpty()) {
        return;
    }

    m_dirtyCollections.insert(label);
    m_collectionDirtyTimer->start();
}

void SecretServiceClient::handlePrompt(bool dismissed)
{
    Q_EMIT promptClosed(!dismissed);
}

bool SecretServiceClient::isAvailable() const
{
    return m_service != nullptr;
}

SecretService *SecretServiceClient::service() const
{
    return m_service.get();
}

SecretServiceClient::Status SecretServiceClient::status() const
{
    return m_status;
}

void SecretServiceClient::setStatus(Status status)
{
    if (status == m_status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged(status);
}

bool SecretServiceClient::unlockCollection(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return false;
    }

    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    if (!collection) {
        *ok = false;
        return false;
    }

    watchCollection(collectionName, ok);

    gboolean locked = secret_collection_get_locked(collection);

    if (locked) {
        gboolean success = secret_service_unlock_sync(m_service.get(), g_list_append(nullptr, collection), nullptr, nullptr, &error);
        *ok = wasErrorFree(&error);
        if (!success) {
            qCWarning(KWALLETS_LOG) << i18n("Unable to unlock collectionName %1", collectionName);
        }
        return success;
    }

    return true;
}

bool SecretServiceClient::lockCollection(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return false;
    }

    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    if (!collection) {
        *ok = false;
        return false;
    }

    gboolean locked = secret_collection_get_locked(collection);

    if (!locked) {
        gboolean success = secret_service_lock_sync(m_service.get(), g_list_append(nullptr, collection), nullptr, nullptr, &error);
        *ok = wasErrorFree(&error);
        if (!success) {
            qCWarning(KWALLETS_LOG) << i18n("Unable to lock collectionName %1", collectionName);
        }
        return success;
    }

    return true;
}

QString SecretServiceClient::defaultCollection(bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return QString();
    }

    QString label = QStringLiteral("kdewallet");

    QDBusInterface serviceInterface(m_serviceBusName,
                                    QStringLiteral("/org/freedesktop/secrets"),
                                    QStringLiteral("org.freedesktop.Secret.Service"),
                                    QDBusConnection::sessionBus());

    if (!serviceInterface.isValid()) {
        qCWarning(KWALLETS_LOG) << "Failed to connect to the DBus SecretService object";
        *ok = false;
        return label;
    }

    QDBusReply<QDBusObjectPath> reply = serviceInterface.call(QStringLiteral("ReadAlias"), QStringLiteral("default"));

    if (!reply.isValid()) {
        qCWarning(KWALLETS_LOG) << "Error reading label:" << reply.error().message();
        *ok = false;
        return label;
    }

    label = collectionLabelForPath(reply.value());

    if (label.isEmpty()) {
        *ok = false;
        return QStringLiteral("kdewallet");
    }

    return label;
}

void SecretServiceClient::setDefaultCollection(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    *ok = secret_service_set_alias_sync(m_service.get(), "default", collection, nullptr, &error);

    *ok = *ok && wasErrorFree(&error);
}

QStringList SecretServiceClient::listCollections(bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return QStringList();
    }

    QStringList collections;

    GListPtr glist = GListPtr(secret_service_get_collections(m_service.get()));

    if (glist) {
        for (GList *iter = glist.get(); iter != nullptr; iter = iter->next) {
            SecretCollection *collection = SECRET_COLLECTION(iter->data);
            const gchar *rawLabel = secret_collection_get_label(collection);
            const QString label = QString::fromUtf8(rawLabel);
            if (!label.isEmpty()) {
                collections.append(label);
            }
        }
    } else {
        qCDebug(KWALLETS_LOG) << i18n("No collections");
    }

    *ok = true;
    return collections;
}

QStringList SecretServiceClient::listFolders(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return {};
    }

    QSet<QString> folders;

    SecretCollection *collection = retrieveCollection(collectionName);

    if (!collection) {
        *ok = false;
        return {};
    }
    GListPtr glist = GListPtr(secret_collection_get_items(collection));

    if (glist) {
        for (GList *iter = glist.get(); iter != nullptr; iter = iter->next) {
            SecretItem *item = static_cast<SecretItem *>(iter->data);

            GHashTable *attributes = secret_item_get_attributes(item);
            if (attributes) {
                const gchar *value = (const char *)g_hash_table_lookup(attributes, "server");
                if (value) {
                    folders.insert(QString::fromUtf8(value));
                }
            }
        }
    } else {
        qCDebug(KWALLETS_LOG) << i18n("No entries");
    }
    *ok = true;
    return folders.values();
}

void SecretServiceClient::createCollection(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;

    secret_collection_create_sync(m_service.get(), collectionName.toUtf8().data(), nullptr, SECRET_COLLECTION_CREATE_NONE, nullptr, &error);

    *ok = wasErrorFree(&error);
}

void SecretServiceClient::deleteCollection(const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    *ok = secret_collection_delete_sync(collection, nullptr, &error);
    m_watchedCollections.remove(collectionName);

    *ok = *ok && wasErrorFree(&error);
    if (ok) {
        Q_EMIT collectionDeleted(collectionName);
    }
}

void SecretServiceClient::deleteFolder(const QString &folder, const QString &collectionName, bool *ok)
{
    if (!attemptConnection()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;

    SecretCollection *collection = retrieveCollection(collectionName);

    GHashTablePtr attributes = GHashTablePtr(g_hash_table_new(g_str_hash, g_str_equal));
    g_hash_table_insert(attributes.get(), g_strdup("server"), g_strdup(folder.toUtf8().constData()));

    GListPtr glist = GListPtr(secret_collection_search_sync(collection, qtKeychainSchema(), attributes.get(), SECRET_SEARCH_ALL, nullptr, &error));

    *ok = wasErrorFree(&error);
    if (!*ok) {
        return;
    }

    if (glist) {
        for (GList *iter = glist.get(); iter != nullptr; iter = iter->next) {
            SecretItem *item = static_cast<SecretItem *>(iter->data);
            m_updateInProgress = true;
            secret_item_delete_sync(item, nullptr, &error);
            *ok = wasErrorFree(&error);
            g_object_unref(item);
        }
    } else {
        qCWarning(KWALLETS_LOG) << i18n("No entries");
    }
}

#include <moc_secretserviceclient.cpp>
