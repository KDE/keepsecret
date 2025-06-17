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

SecretServiceClient::SecretServiceClient(QObject *parent)
    : QObject(parent)
{
    m_serviceBusName = QStringLiteral("org.freedesktop.secrets");

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
        {{"user", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"server", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"type", SECRET_SCHEMA_ATTRIBUTE_STRING}}};

    return &schema;
}

bool SecretServiceClient::wasErrorFree(GError **error, QString &message)
{
    if (!*error) {
        return true;
    }
    message = QString::fromUtf8((*error)->message);
    qCWarning(KWALLETS_LOG) << message;
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
    if (!isAvailable()) {
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
    if (!isAvailable()) {
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
    QString message;
    SecretServiceClient *client = (SecretServiceClient *)inst;

    SecretService *service = secret_service_get_finish(result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
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
        clearOperation(Connecting);
        readDefaultCollection();
    } else {
        setStatus(SecretServiceClient::Disconnected);
    }
}

bool SecretServiceClient::attemptConnection()
{
    if (m_service) {
        return true;
    }

    setOperation(Connecting);

    secret_service_get(static_cast<SecretServiceFlags>(SECRET_SERVICE_OPEN_SESSION | SECRET_SERVICE_LOAD_COLLECTIONS), nullptr, onServiceGetFinished, this);

    return true;
}

void SecretServiceClient::onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(oldOwner);

    bool available = !newOwner.isEmpty();

    setStatus(Disconnected);
    m_service.reset();

    qDebug() << "Secret Service availability changed:" << (available ? "Available" : "Unavailable");
    Q_EMIT serviceChanged();

    if (available) {
        attemptConnection();
    }
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
    if (!isAvailable()) {
        return;
    }

    // Only emitting collectionListDirty here as we can't know the actual label
    // of the collection as is already deleted
    Q_EMIT collectionListDirty();
}

void SecretServiceClient::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)

    if (interface == QStringLiteral("org.freedesktop.Secret.Service")) {
        readDefaultCollection();
        Q_EMIT collectionsChanged();
    }
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

SecretServiceClient::Operations SecretServiceClient::operations() const
{
    return m_operations;
}

void SecretServiceClient::setOperations(SecretServiceClient::Operations operations)
{
    if (operations == m_operations) {
        return;
    }

    qWarning() << "SecretServiceClient: Setting operations" << operations;

    m_operations = operations;
    Q_EMIT operationsChanged(operations);
}

void SecretServiceClient::setOperation(SecretServiceClient::Operation operation)
{
    setOperations(m_operations | operation);
}

void SecretServiceClient::clearOperation(SecretServiceClient::Operation operation)
{
    setOperations(m_operations & ~operation);
}

SecretServiceClient::Error SecretServiceClient::error() const
{
    return m_error;
}

QString SecretServiceClient::errorMessage() const
{
    return m_errorMessage;
}

void SecretServiceClient::setError(SecretServiceClient::Error error, const QString &errorMessage)
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

QString SecretServiceClient::defaultCollection()
{
    return m_defaultCollection;
}

void SecretServiceClient::readDefaultCollection()
{
    if (!isAvailable()) {
        m_defaultCollection.clear();
        Q_EMIT defaultCollectionChanged(m_defaultCollection);
        return;
    }

    QString label = QStringLiteral("kdewallet");

    QDBusInterface serviceInterface(m_serviceBusName,
                                    QStringLiteral("/org/freedesktop/secrets"),
                                    QStringLiteral("org.freedesktop.Secret.Service"),
                                    QDBusConnection::sessionBus());

    if (!serviceInterface.isValid()) {
        setError(ConnectionFailed, i18n("Failed to connect to the DBus SecretService object"));
        m_defaultCollection.clear();
        Q_EMIT defaultCollectionChanged(m_defaultCollection);
        return;
    }

    setOperation(ReadingDefault);
    QDBusPendingCall call = serviceInterface.asyncCall(QStringLiteral("ReadAlias"), QStringLiteral("default"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QDBusObjectPath> reply = *call;

        const QString oldDefaultCollection = m_defaultCollection;

        if (reply.isError()) {
            setError(ReadDefaultFailed, reply.error().message());
            m_defaultCollection.clear();
        } else {
            m_defaultCollection = collectionLabelForPath(reply.value());
        }

        call->deleteLater();
        clearOperation(ReadingDefault);
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
        client->setError(SecretServiceClient::SetDefaultFailed, message);
    }
    client->clearOperation(SecretServiceClient::WritingDefault);
    client->readDefaultCollection();
}

void SecretServiceClient::setDefaultCollection(const QString &collectionName)
{
    if (!isAvailable()) {
        return;
    }

    SecretCollection *collection = retrieveCollection(collectionName);

    setOperation(WritingDefault);
    secret_service_set_alias(m_service.get(), "default", collection, nullptr, onSetDefaultCollectionFinished, this);
}

QStringList SecretServiceClient::listCollections(bool *ok)
{
    if (!isAvailable()) {
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

void SecretServiceClient::createCollection(const QString &collectionName, bool *ok)
{
    if (!isAvailable()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;
    QString message;

    secret_collection_create_sync(m_service.get(), collectionName.toUtf8().data(), nullptr, SECRET_COLLECTION_CREATE_NONE, nullptr, &error);

    *ok = wasErrorFree(&error, message);
}

void SecretServiceClient::deleteCollection(const QString &collectionName, bool *ok)
{
    if (!isAvailable()) {
        *ok = false;
        return;
    }

    GError *error = nullptr;
    QString message;

    SecretCollection *collection = retrieveCollection(collectionName);

    *ok = secret_collection_delete_sync(collection, nullptr, &error);

    *ok = *ok && wasErrorFree(&error, message);
    if (ok) {
        Q_EMIT collectionDeleted(collectionName);
    }
}

#include <moc_secretserviceclient.cpp>
