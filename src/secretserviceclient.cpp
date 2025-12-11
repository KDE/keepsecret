/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "secretserviceclient.h"
#include "keepsecret_debug.h"
#include "statetracker.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QEventLoop>
#include <QTimer>
#include <memory>

SecretServiceClient::SecretServiceClient(QObject *parent)
    : QObject(parent)
    , m_serviceBusName(QStringLiteral("org.freedesktop.secrets"))
{
    m_serviceWatcher = new QDBusServiceWatcher(m_serviceBusName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &SecretServiceClient::onServiceOwnerChanged);

    // Unconditionally try to connect to the service without checking it exists:
    // it will try to dbus-activate it if not running
    attemptConnection();

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

    // React to properties change
    bus.connect(m_serviceBusName,
                QStringLiteral("/org/freedesktop/secrets"),
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"),
                this, // receiver
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
}

// Copied from
// https://github.com/frankosterfeld/qtkeychain/blob/main/qtkeychain/libsecret.cpp
// This is intended to be a data format compatible with QtKeychain
const SecretSchema *SecretServiceClient::qtKeychainSchema(void)
{
    static const SecretSchema schema = {
        "org.qt.keychain",
        SECRET_SCHEMA_DONT_MATCH_NAME,
        {{"user", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"server", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"type", SECRET_SCHEMA_ATTRIBUTE_STRING}},
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL};

    return &schema;
}

bool SecretServiceClient::wasErrorFree(GError **error, QString &message)
{
    if (!*error) {
        return true;
    }
    message = QString::fromUtf8((*error)->message);
    qCWarning(KEEPSECRET_LOG) << message;
    g_error_free(*error);
    *error = nullptr;
    return false;
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
    if (!StateTracker::instance()->isServiceConnected()) {
        return {};
    }
    QDBusInterface collectionInterface(m_serviceBusName, path.path(), QStringLiteral("org.freedesktop.Secret.Collection"), QDBusConnection::sessionBus());

    if (!collectionInterface.isValid()) {
        qCWarning(KEEPSECRET_LOG) << "Failed to connect to the DBus collection object:" << path.path();
        return {};
    }

    QVariant reply = collectionInterface.property("Label");

    if (!reply.isValid()) {
        qCWarning(KEEPSECRET_LOG) << "Error reading label:" << collectionInterface.lastError();
        return {};
    }

    return reply.toString();
}

SecretCollection *SecretServiceClient::retrieveCollection(const QString &collectionPath)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return nullptr;
    }

    GListPtr collections = GListPtr(secret_service_get_collections(m_service.get()));

    for (GList *l = collections.get(); l != nullptr; l = l->next) {
        SecretCollection *coll = SECRET_COLLECTION(l->data);
        const QString path = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(coll)));
        if (path == collectionPath) {
            SecretCollection *collection = coll;
            return collection;
        }
    }

    return nullptr;
}

SecretItemPtr SecretServiceClient::retrieveItem(const QString &itemPath, const QString &collectionPath, bool *ok)
{
    SecretCollection *collection = retrieveCollection(collectionPath);

    GListPtr list = GListPtr(secret_collection_get_items(collection));
    if (list) {
        *ok = true;
        for (GList *l = list.get(); l != nullptr; l = l->next) {
            SecretItem *item = static_cast<SecretItem *>(l->data);
            const QString path = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(item)));

            if (path == itemPath) {
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
    Q_UNUSED(source);
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    SecretService *service = secret_service_get_finish(result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        client->attemptConnectionFinished(service);
    } else {
        StateTracker::instance()->setError(StateTracker::ServiceConnectionError, message);
        client->attemptConnectionFinished(nullptr);
    }
}

void SecretServiceClient::attemptConnectionFinished(SecretService *service)
{
    m_service.reset(service);
    if (service) {
        StateTracker::instance()->setState(StateTracker::ServiceConnected);
        StateTracker::instance()->clearOperation(StateTracker::ServiceConnecting);
        readDefaultCollection();
    } else {
        // Use setStatus as it will reset any other state
        StateTracker::instance()->setStatus(StateTracker::ServiceDisconnected);
    }
}

void SecretServiceClient::attemptConnection()
{
    if (m_service) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::ServiceConnecting);

    // Since glib/libsecret doesn't have something like QFlags, this line
    // will always do a warning
    // clang-format off
    secret_service_get(static_cast<SecretServiceFlags>(SECRET_SERVICE_OPEN_SESSION | SECRET_SERVICE_LOAD_COLLECTIONS), nullptr, onServiceGetFinished, this); // NOLINT
    // clang-format on
}

void SecretServiceClient::onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(oldOwner);

    bool available = !newOwner.isEmpty();

    StateTracker::instance()->setStatus(StateTracker::ServiceDisconnected);
    m_service.reset();

    qCWarning(KEEPSECRET_LOG) << "Secret Service availability changed:" << (available ? "Available" : "Unavailable");
    Q_EMIT serviceChanged();

    if (!available) {
        StateTracker::instance()->setError(StateTracker::ServiceConnectionError, i18nc("@info:status", "Secret Service provider unavailable."));
    }

    // Unconditionally attempt a connection, as the service might be DBus-activated
    attemptConnection();
}

void SecretServiceClient::onCollectionCreated(const QDBusObjectPath &path)
{
    const QString label = collectionLabelForPath(path);
    if (label.isEmpty()) {
        return;
    }

    // libsecret is connected too to this signal, and we have a race condition
    // on who handles this before. We need this to be handled after
    // libsecret did, otherwise when we do secret_service_load_collections
    // it will still return an old cached version
    QTimer::singleShot(0, this, &SecretServiceClient::loadCollections);
    Q_EMIT collectionCreated(path);
}

void SecretServiceClient::onCollectionDeleted(const QDBusObjectPath &path)
{
    Q_UNUSED(path);
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    QTimer::singleShot(0, this, &SecretServiceClient::loadCollections);
    Q_EMIT collectionDeleted(path);
}

void SecretServiceClient::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(changedProperties);
    Q_UNUSED(invalidatedProperties)

    if (interface == QStringLiteral("org.freedesktop.Secret.Service")) {
        readDefaultCollection();
    }
}

void SecretServiceClient::handlePrompt(bool dismissed)
{
    Q_EMIT promptClosed(!dismissed);
}

SecretService *SecretServiceClient::service() const
{
    return m_service.get();
}

QString SecretServiceClient::defaultCollection()
{
    return m_defaultCollection;
}

void SecretServiceClient::readDefaultCollection()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        m_defaultCollection.clear();
        Q_EMIT defaultCollectionChanged(m_defaultCollection);
        return;
    }

    QDBusInterface serviceInterface(m_serviceBusName,
                                    QStringLiteral("/org/freedesktop/secrets"),
                                    QStringLiteral("org.freedesktop.Secret.Service"),
                                    QDBusConnection::sessionBus());

    if (!serviceInterface.isValid()) {
        StateTracker::instance()->setError(StateTracker::ServiceConnectionError, i18nc("@info:status", "Failed to connect to the DBus SecretService object"));
        m_defaultCollection.clear();
        Q_EMIT defaultCollectionChanged(m_defaultCollection);
        return;
    } else {
        StateTracker::instance()->clearError();
    }

    StateTracker::instance()->setOperation(StateTracker::CollectionReadingDefault);
    QDBusPendingCall call = serviceInterface.asyncCall(QStringLiteral("ReadAlias"), QStringLiteral("default"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QDBusObjectPath> reply = *call;

        const QString oldDefaultCollection = m_defaultCollection;

        if (reply.isError()) {
            StateTracker::instance()->setError(StateTracker::CollectionReadDefaultError, reply.error().message());
            m_defaultCollection.clear();
        } else {
            StateTracker::instance()->clearError();
            m_defaultCollection = reply.value().path();
        }

        call->deleteLater();
        StateTracker::instance()->clearOperation(StateTracker::CollectionReadingDefault);
        if (oldDefaultCollection != m_defaultCollection) {
            Q_EMIT defaultCollectionChanged(m_defaultCollection);
        }
    });
}

static void onSetDefaultCollectionFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    secret_service_set_alias_finish((SecretService *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::CollectionWriteDefaultError, message);
    }
    StateTracker::instance()->clearOperation(StateTracker::CollectionWritingDefault);
    client->readDefaultCollection();
}

void SecretServiceClient::setDefaultCollection(const QString &collectionPath)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    SecretCollection *collection = retrieveCollection(collectionPath);

    StateTracker::instance()->setOperation(StateTracker::CollectionWritingDefault);
    secret_service_set_alias(m_service.get(), "default", collection, nullptr, onSetDefaultCollectionFinished, this);
}

QList<SecretServiceClient::CollectionEntry> SecretServiceClient::listCollections()
{
    QList<CollectionEntry> collections;

    if (!StateTracker::instance()->isServiceConnected()) {
        return collections;
    }

    GListPtr glist = GListPtr(secret_service_get_collections(m_service.get()));

    if (glist) {
        for (GList *iter = glist.get(); iter != nullptr; iter = iter->next) {
            CollectionEntry entry;
            SecretCollection *collection = SECRET_COLLECTION(iter->data);
            const gchar *rawLabel = secret_collection_get_label(collection);
            // Skip empty names: gnome-keyring uses an empty label as an insernal session collection that the user shouldn't touch
            if (strlen(rawLabel) == 0) {
                g_object_unref(collection);
                continue;
            }

            entry.name = QString::fromUtf8(rawLabel);
            entry.dbusPath = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(collection)));
            entry.locked = secret_collection_get_locked(collection);

            collections << entry;
        }
    }

    return collections;
}

static void onLoadCollectionsFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    secret_service_load_collections_finish((SecretService *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::ServiceLoadCollectionsError, message);
    }

    StateTracker::instance()->clearOperation(StateTracker::ServiceLoadingCollections);
    Q_EMIT client->collectionListDirty();
}

void SecretServiceClient::loadCollections()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::ServiceLoadingCollections);
    secret_service_load_collections(m_service.get(), nullptr, onLoadCollectionsFinished, this);
}

static void onLockCollectionFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;
    GList *locked = nullptr;

    secret_service_lock_finish((SecretService *)source, result, &locked, &error);

    QString path;
    // FIXME: can there me more than one unlocked at once?
    for (GList *l = locked; l != nullptr; l = l->next) {
        SecretCollection *coll = SECRET_COLLECTION(l->data);
        path = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(coll)));
        break;
    }
    g_list_free(locked);

    StateTracker::instance()->clearOperation(StateTracker::CollectionLocking);
    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        client->loadCollections();
        Q_EMIT client->collectionLocked(QDBusObjectPath(path));
    } else {
        StateTracker::instance()->setError(StateTracker::CollectionLockError, message);
    }
}

void SecretServiceClient::lockCollection(const QString &collectionPath)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    SecretCollectionPtr collection;
    // TODO dbus path
    collection.reset(retrieveCollection(collectionPath));

    if (!collection) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::CollectionLocking);
    secret_service_lock(m_service.get(), g_list_append(nullptr, collection.get()), nullptr, onLockCollectionFinished, this);
}

static void onUnlockCollectionFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;
    GList *unlocked = nullptr;

    secret_service_unlock_finish((SecretService *)source, result, &unlocked, &error);

    QString path;
    // FIXME: can there me more than one unlocked at once?
    for (GList *l = unlocked; l != nullptr; l = l->next) {
        SecretCollection *coll = SECRET_COLLECTION(l->data);
        path = QString::fromUtf8(g_dbus_proxy_get_object_path(G_DBUS_PROXY(coll)));
        break;
    }
    g_list_free(unlocked);

    StateTracker::instance()->clearOperation(StateTracker::CollectionUnlocking);
    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        client->loadCollections();
        Q_EMIT client->collectionUnlocked(QDBusObjectPath(path));
    } else {
        StateTracker::instance()->setError(StateTracker::CollectionUnlockError, message);
    }
}

void SecretServiceClient::unlockCollection(const QString &collectionPath)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    SecretCollectionPtr collection;
    // TODO dbus path
    collection.reset(retrieveCollection(collectionPath));

    if (!collection) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::CollectionUnlocking);
    secret_service_unlock(m_service.get(), g_list_append(nullptr, collection.get()), nullptr, onUnlockCollectionFinished, this);
}

static void onCreateCollectionFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(source);
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    secret_collection_create_finish(result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::CollectionCreationError, message);
    }
    StateTracker::instance()->clearOperation(StateTracker::CollectionCreating);
    client->readDefaultCollection();
    client->loadCollections();
}

void SecretServiceClient::createCollection(const QString &collectionName)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::CollectionCreating);
    secret_collection_create(m_service.get(),
                             collectionName.toUtf8().data(),
                             nullptr,
                             SECRET_COLLECTION_CREATE_NONE,
                             nullptr,
                             onCreateCollectionFinished,
                             this);
}

static void onDeleteCollectionFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    secret_collection_delete_finish((SecretCollection *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::CollectionDeleteError, message);
    }
    StateTracker::instance()->clearOperation(StateTracker::CollectionDeleting);
    client->readDefaultCollection();
}

void SecretServiceClient::deleteCollection(const QString &collectionPath)
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    SecretCollection *collection = retrieveCollection(collectionPath);

    StateTracker::instance()->setOperation(StateTracker::CollectionDeleting);
    secret_collection_delete(collection, nullptr, onDeleteCollectionFinished, this);
}

#include <moc_secretserviceclient.cpp>
