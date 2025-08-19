/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusObjectPath>
#include <QObject>
#include <QQmlEngine>

#include <libsecret/secret.h>
#include <memory>

class QDBusServiceWatcher;
class QTimer;

// To allow gobject derived things with std::unique_ptr
struct GObjectDeleter {
    template<typename T>
    void operator()(T *obj) const
    {
        if (obj) {
            g_object_unref(obj);
        }
    }
};

struct GHashTableDeleter {
    void operator()(GHashTable *table) const
    {
        if (table) {
            g_hash_table_destroy(table);
        }
    }
};

struct GListDeleter {
    void operator()(GList *list) const
    {
        if (list) {
            g_list_free(list);
        }
    }
};

struct SecretValueDeleter {
    void operator()(SecretValue *value) const
    {
        if (value) {
            secret_value_unref(value);
        }
    }
};

template<typename T>
using GObjectPtr = std::unique_ptr<T, GObjectDeleter>;
using SecretServicePtr = GObjectPtr<SecretService>;
using SecretCollectionPtr = GObjectPtr<SecretCollection>;
using SecretItemPtr = GObjectPtr<SecretItem>;
using GHashTablePtr = std::unique_ptr<GHashTable, GHashTableDeleter>;
using GListPtr = std::unique_ptr<GList, GListDeleter>;
using SecretValuePtr = std::unique_ptr<SecretValue, SecretValueDeleter>;

class SecretServiceClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot create elements of type SecretServiceClient")

    Q_PROPERTY(QString defaultCollection READ defaultCollection WRITE setDefaultCollection NOTIFY defaultCollectionChanged)

public:
    enum Type {
        PlainText = 0,
        Base64,
        Binary,
        Map,
        Unknown
    };
    Q_ENUM(Type);

    struct CollectionEntry {
        QString name;
        QString dbusPath;
        bool locked;
    };

    SecretServiceClient(QObject *parent = nullptr);

    static const SecretSchema *qtKeychainSchema(void);

    static bool wasErrorFree(GError **error, QString &message);

    SecretService *service() const;

    SecretCollection *retrieveCollection(const QString &collectionPath);
    // TODO: move in secretitemproxy?
    SecretItemPtr retrieveItem(const QString &dbusPath, const QString &collectionPath, bool *ok);

    QString defaultCollection();
    void setDefaultCollection(const QString &collectionPath);

    QList<CollectionEntry> listCollections();
    void loadCollections();

    // collectionPath is the dbus path of the collection
    Q_INVOKABLE void lockCollection(const QString &collectionPath);
    // collectionPath is the dbus path of the collection
    Q_INVOKABLE void unlockCollection(const QString &collectionPath);

    // collectionName is the user-visible label of the collection, they might be non unique
    Q_INVOKABLE void createCollection(const QString &collectionName);
    // collectionPath is the dbus path of the collection
    Q_INVOKABLE void deleteCollection(const QString &collectionPath);

    static QString typeToString(SecretServiceClient::Type type);
    static Type stringToType(const QString &typeName);

    // Functions for the static libsecret handlers
    void readDefaultCollection();
    void attemptConnectionFinished(SecretService *service);

Q_SIGNALS:
    // Emitted when the service availability changed, or the service owner of secretservice has changed to a new one
    void serviceChanged();
    void collectionsChanged();
    void promptClosed(bool accepted);
    void collectionListDirty();
    void defaultCollectionChanged(const QString &collection);
    void collectionCreated(const QDBusObjectPath &path);
    void collectionDeleted(const QDBusObjectPath &path);
    void collectionLocked(const QDBusObjectPath &path);
    void collectionUnlocked(const QDBusObjectPath &path);

protected:
    void attemptConnection();
    void onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

    QString collectionLabelForPath(const QDBusObjectPath &path);

protected Q_SLOTS:
    void handlePrompt(bool dismissed);
    void onCollectionCreated(const QDBusObjectPath &path);
    void onCollectionDeleted(const QDBusObjectPath &path);
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    SecretServicePtr m_service;
    QString m_serviceBusName;
    QDBusServiceWatcher *m_serviceWatcher;

    QString m_defaultCollection;
};
