/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusObjectPath>
#include <QObject>
#include <QQmlEngine>

#include <libsecret/secret.h>

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

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Operations operations READ operations NOTIFY operationsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
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

    enum Status {
        Disconnected = 0,
        Connected
    };
    Q_ENUM(Status);

    enum Operation {
        OperationNone = 0,
        Connecting = 1,
        LoadingCollections = 2,
        ReadingDefault = 4,
        WritingDefault = 8,
        LockingCollection = 16,
        UnlockingCollection = 32,
        CreatingCollection = 64,
        DeletingCollection = 128
    };
    Q_ENUM(Operation);
    Q_DECLARE_FLAGS(Operations, Operation);

    enum Error {
        NoError = 0,
        ConnectionFailed,
        ReadDefaultFailed,
        SetDefaultFailed,
        LoadCollectionsFailed,
        UnlockCollectionFailed,
        LockCollectionFailed
    };
    Q_ENUM(Error);

    struct CollectionEntry {
        QString name;
        QString dbusPath;
        bool locked;
    };

    SecretServiceClient(QObject *parent = nullptr);

    static const SecretSchema *qtKeychainSchema(void);

    static bool wasErrorFree(GError **error, QString &message);

    bool isAvailable() const;
    SecretService *service() const;

    Status status() const;
    void setStatus(Status status);

    Operations operations() const;
    void setOperations(Operations operations);
    void setOperation(Operation operation);
    void clearOperation(Operation operation);

    Error error() const;
    QString errorMessage() const;
    void setError(Error error, const QString &message);

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
    void statusChanged(Status status);
    void operationsChanged(Operations operations);
    void errorChanged(Error error);
    void errorMessageChanged(const QString &errorMessage);
    void collectionsChanged();
    void promptClosed(bool accepted);
    void collectionListDirty();
    void defaultCollectionChanged(const QString &collection);
    void collectionCreated(const QDBusObjectPath &path);
    void collectionDeleted(const QDBusObjectPath &path);
    void collectionLocked(const QDBusObjectPath &path);
    void collectionUnlocked(const QDBusObjectPath &path);

protected:
    bool attemptConnection();
    void onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

    QString collectionLabelForPath(const QDBusObjectPath &path);

protected Q_SLOTS:
    void handlePrompt(bool dismissed);
    void onCollectionCreated(const QDBusObjectPath &path);
    void onCollectionDeleted(const QDBusObjectPath &path);
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    SecretServicePtr m_service;
    Status m_status = Disconnected;
    Operations m_operations = OperationNone;
    Error m_error = NoError;
    QString m_errorMessage;
    QString m_serviceBusName;
    QDBusServiceWatcher *m_serviceWatcher;

    QString m_defaultCollection;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SecretServiceClient::Operations)
