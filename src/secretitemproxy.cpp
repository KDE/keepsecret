// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "secretitemproxy.h"
#include "kwallets_debug.h"
#include "secretserviceclient.h"

#include <KLocalizedString>

SecretItemProxy::SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(m_secretServiceClient, &SecretServiceClient::serviceChanged, this, [this] {
        if (m_dbusPath.isEmpty()) {
            setStatus(Disconnected);
            return;
        }
        if (m_secretServiceClient->isAvailable()) {
            setStatus(Connected);
            loadItem(m_wallet, m_dbusPath);
        } else {
            setStatus(Disconnected);
            close();
        }
    });
}

SecretItemProxy::~SecretItemProxy()
{
}

SecretItemProxy::Status SecretItemProxy::status() const
{
    return m_status;
}

void SecretItemProxy::setStatus(Status status)
{
    if (status == m_status) {
        return;
    }

    qWarning() << "Setting status" << status;

    m_status = status;
    Q_EMIT statusChanged(status);
}

SecretItemProxy::Operations SecretItemProxy::operations() const
{
    return m_operations;
}

void SecretItemProxy::setOperations(SecretItemProxy::Operations operations)
{
    if (operations == m_operations) {
        return;
    }

    if (m_operations & Saving && !(operations & Saving)) {
        m_modificationTime = QDateTime::currentDateTime();
        Q_EMIT modificationTimeChanged(m_modificationTime);
    }

    qWarning() << "Setting operations" << operations;

    m_operations = operations;
    Q_EMIT operationsChanged(operations);
}

void SecretItemProxy::setOperation(SecretItemProxy::Operation operation)
{
    setOperations(m_operations | operation);
}

void SecretItemProxy::clearOperation(SecretItemProxy::Operation operation)
{
    // Keep the Loading or Saving flags if we still have another load/save operation
    Operations result = m_operations & ~operation;
    if (result & (SavingLabel | SavingSecret | SavingAttributes)) {
        result |= Saving;
    }
    if (result & (LoadingSecret | Unlocking)) {
        result |= Loading;
    }
    setOperations(result);
}

SecretItemProxy::Error SecretItemProxy::error() const
{
    return m_error;
}

QString SecretItemProxy::errorMessage() const
{
    return m_errorMessage;
}

void SecretItemProxy::setError(SecretItemProxy::Error error, const QString &errorMessage)
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

QDateTime SecretItemProxy::creationTime() const
{
    return m_creationTime;
}

QDateTime SecretItemProxy::modificationTime() const
{
    return m_modificationTime;
}

QString SecretItemProxy::wallet() const
{
    return m_wallet;
}

QString SecretItemProxy::itemName() const
{
    return m_itemName;
}

QString SecretItemProxy::label() const
{
    return m_label;
}

void SecretItemProxy::setLabel(const QString &label)
{
    if (label == m_label) {
        return;
    }

    m_label = label;

    Q_EMIT labelChanged(label);
    setStatus(NeedsSave);
}

QString SecretItemProxy::secretValue() const
{
    return QString::fromUtf8(m_secretValue);
}

void SecretItemProxy::setSecretValue(const QString &secretValue)
{
    const QString stringValue = QString::fromUtf8(m_secretValue);
    if (secretValue == stringValue) {
        return;
    }

    m_secretValue = secretValue.toUtf8();

    Q_EMIT secretValueChanged();
    setStatus(NeedsSave);
}

void SecretItemProxy::setSecretValue(const QByteArray &secretValue)
{
    if (secretValue == m_secretValue) {
        return;
    }

    m_secretValue = secretValue;

    Q_EMIT secretValueChanged();
    setStatus(NeedsSave);
}

QVariantMap SecretItemProxy::attributes() const
{
    return m_attributes;
}

SecretServiceClient::Type SecretItemProxy::type() const
{
    return m_type;
}

static void onLoadSecretFinish(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_load_secret_finish((SecretItem *)source, result, &error);

    QString message;

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        SecretValuePtr secretValue = SecretValuePtr(secret_item_get_secret(proxy->secretItem()));

        if (secretValue) {
            if (proxy->type() == SecretServiceClient::Binary) {
                gsize length = 0;
                const gchar *password = secret_value_get(secretValue.get(), &length);
                proxy->setSecretValue(QByteArray(password, length));
            } else {
                const gchar *password = secret_value_get_text(secretValue.get());
                if (proxy->type() == SecretServiceClient::Base64) {
                    proxy->setSecretValue(QByteArray::fromBase64(QByteArray(password)));
                } else {
                    proxy->setSecretValue(QByteArray(password));
                }
            }
            proxy->setStatus(SecretItemProxy::Ready);
        } else {
            proxy->setError(SecretItemProxy::LoadSecretFailed, i18n("Couldn't retrieve the secret value"));
        }
    } else {
        proxy->setError(SecretItemProxy::LoadSecretFailed, message);
    }
    proxy->clearOperation(SecretItemProxy::LoadingSecret);
}

static void onItemCreateFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(source);
    // qWarning()<<"AAAAAAA"<<g_dbus_proxy_get_object_path((SecretItem *)source)
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_create_finish(result, &error);

    if (!SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setError(SecretItemProxy::CreationFailed, message);
    }
    proxy->clearOperation(SecretItemProxy::Creating);
}

void SecretItemProxy::createItem(const QString &label,
                                 const QByteArray &secret,
                                 // const SecretServiceClient::Type type,
                                 const QString &user,
                                 const QString &server,
                                 const QString &wallet)
{
    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    // TODO: make it a paramenter?
    const SecretServiceClient::Type type = SecretServiceClient::PlainText;

    SecretCollection *collection = m_secretServiceClient->retrieveCollection(wallet);

    QByteArray data;
    if (type == SecretServiceClient::Base64) {
        data = secret.toBase64();
    } else {
        data = secret;
    }

    QString mimeType;
    if (type == SecretServiceClient::Binary) {
        mimeType = QStringLiteral("application/octet-stream");
    } else {
        mimeType = QStringLiteral("text/plain");
    }

    SecretValuePtr secretValue = SecretValuePtr(secret_value_new(data.constData(), -1, mimeType.toLatin1().constData()));
    if (!secretValue) {
        setError(CreationFailed, i18n("Failed to create SecretValue"));
        return;
    }

    GHashTablePtr attributes = GHashTablePtr(g_hash_table_new(g_str_hash, g_str_equal));
    g_hash_table_insert(attributes.get(), g_strdup("user"), g_strdup(user.toUtf8().constData()));
    g_hash_table_insert(attributes.get(), g_strdup("type"), g_strdup(m_secretServiceClient->typeToString(type).toUtf8().constData()));
    g_hash_table_insert(attributes.get(), g_strdup("server"), g_strdup(server.toUtf8().constData()));

    secret_item_create(collection,
                       m_secretServiceClient->qtKeychainSchema(),
                       attributes.get(),
                       label.toUtf8().constData(),
                       secretValue.get(),
                       SECRET_ITEM_CREATE_REPLACE,
                       nullptr,
                       onItemCreateFinished,
                       this);

    setOperation(Creating);
}

void SecretItemProxy::loadItem(const QString &wallet, const QString &dbusPath)
{
    m_dbusPath = dbusPath;

    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    bool ok;

    m_secretItem = m_secretServiceClient->retrieveItem(dbusPath, wallet, &ok);

    if (ok) {
        if (secret_item_get_locked(m_secretItem.get())) {
            setStatus(Locked);
        }
        m_creationTime = QDateTime::fromSecsSinceEpoch(secret_item_get_created(m_secretItem.get()));
        m_modificationTime = QDateTime::fromSecsSinceEpoch(secret_item_get_modified(m_secretItem.get()));
        m_label = QString::fromUtf8(secret_item_get_label(m_secretItem.get()));

        m_attributes.clear();
        GHashTablePtr attributes = GHashTablePtr(secret_item_get_attributes(m_secretItem.get()));

        if (attributes) {
            const char *schema = static_cast<gchar *>(g_hash_table_lookup(attributes.get(), "xdg:schema"));
            if (schema && g_strcmp0(schema, "org.qt.keychain") == 0) {
                // Retrieve "server" value
                const char *server = static_cast<gchar *>(g_hash_table_lookup(attributes.get(), "server"));
                if (server) {
                    m_folder = QString::fromUtf8(server);
                }
                const char *user = static_cast<gchar *>(g_hash_table_lookup(attributes.get(), "user"));
                if (user) {
                    m_itemName = QString::fromUtf8(server);
                }
            }

            GHashTableIter attrIter;
            gpointer key, value;
            g_hash_table_iter_init(&attrIter, attributes.get());
            QStringList keys;
            while (g_hash_table_iter_next(&attrIter, &key, &value)) {
                QString keyString = QString::fromUtf8(static_cast<gchar *>(key));
                QString valueString = QString::fromUtf8(static_cast<gchar *>(value));
                m_attributes.insert(keyString, valueString);
                keys << keyString;
                if (keyString == QStringLiteral("type")) {
                    m_type = m_secretServiceClient->stringToType(valueString);
                }
            }
            m_attributes[QStringLiteral("__keys")] = keys;
        }
        if (m_status == Locked) {
            unlock();
        } else {
            setOperation(LoadingSecret);
            secret_item_load_secret(m_secretItem.get(), nullptr, onLoadSecretFinish, this);
        }

    } else {
        m_creationTime = {};
        m_modificationTime = {};
        m_wallet = QString();
        m_folder = QString();
        m_itemName = QString();
        m_label = QString();
        m_secretValue = QByteArray();
        m_attributes.clear();
        m_attributes[QStringLiteral("__keys")] = QStringList();
        setError(LoadFailed, QStringLiteral("Failed to load the secret item"));
    }

    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT attributesChanged(m_attributes);
}

static void onItemUnlockFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_service_unlock_finish((SecretService *)source, result, nullptr, &error);

    if (!SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setError(SecretItemProxy::UnlockFailed, message);
    }

    proxy->clearOperation(SecretItemProxy::Unlocking);
}

void SecretItemProxy::unlock()
{
    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    setOperation(Unlocking);
    secret_service_unlock(m_secretServiceClient->service(), g_list_append(nullptr, m_secretItem.get()), nullptr, onItemUnlockFinished, this);
}

static void onSetLabelFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_set_label_finish((SecretItem *)source, result, &error);

    if (!SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setError(SecretItemProxy::SaveFailed, message);
    }

    proxy->clearOperation(SecretItemProxy::SavingLabel);
}

static void onSetAttributesFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_set_attributes_finish((SecretItem *)source, result, &error);

    if (!SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setError(SecretItemProxy::SaveFailed, message);
    }

    proxy->clearOperation(SecretItemProxy::SavingAttributes);
}

static void onSetSecretFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_set_secret_finish((SecretItem *)source, result, &error);

    if (!SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setError(SecretItemProxy::SaveFailed, message);
    }

    proxy->clearOperation(SecretItemProxy::SavingSecret);
}

void SecretItemProxy::save()
{
    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    secret_item_set_label(m_secretItem.get(), m_label.toUtf8().data(), nullptr, onSetLabelFinished, this);
    setOperation(SavingLabel);

    if (m_attributes.contains(QStringLiteral("xdg:schema")) && m_attributes[QStringLiteral("xdg:schema")] == QStringLiteral("org.qt.keychain")) {
        GHashTable *attributes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        for (auto it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it) {
            if (it.key() == QStringLiteral("__keys")) {
                continue;
            }
            QByteArray keyBytes = it.key().toUtf8();
            gchar *key = g_strdup(keyBytes.constData());

            QByteArray valueBytes = it.value().toString().toUtf8();
            gchar *value = g_strdup(valueBytes.constData());

            g_hash_table_insert(attributes, key, value);
        }

        secret_item_set_attributes(m_secretItem.get(), SecretServiceClient::qtKeychainSchema(), attributes, nullptr, onSetAttributesFinished, this);
        setOperation(SavingAttributes);

        SecretValuePtr secretValue;
        if (m_type == SecretServiceClient::Binary) {
            secretValue.reset(secret_value_new(m_secretValue.data(), m_secretValue.length(), "application/octet-stream"));
        } else if (m_type == SecretServiceClient::Base64) {
            secretValue.reset(secret_value_new(m_secretValue.toBase64().data(), m_secretValue.length(), "text/plain"));
        } else {
            secretValue.reset(secret_value_new(m_secretValue.data(), m_secretValue.length(), "text/plain"));
        }

        secret_item_set_secret(m_secretItem.get(), secretValue.get(), nullptr, onSetSecretFinished, this);
        setOperation(SavingSecret);
    }
}

void SecretItemProxy::close()
{
    m_creationTime = {};
    m_modificationTime = {};
    m_wallet = QString();
    m_folder = QString();
    m_itemName = QString();
    m_label = QString();
    m_secretValue = QByteArray();
    m_folder = QString();
    m_attributes.clear();
    m_attributes[QStringLiteral("__keys")] = QStringList();

    m_secretItem.reset();

    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT attributesChanged(m_attributes);

    setStatus(Connected);
}

static void onDeleteFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_delete_finish((SecretItem *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        proxy->setStatus(SecretItemProxy::Connected);
    } else {
        proxy->setError(SecretItemProxy::DeleteFailed, message);
    }
    proxy->close();
    proxy->clearOperation(SecretItemProxy::Deleting);
}

void SecretItemProxy::deleteItem()
{
    if (m_status <= Connected) {
        return;
    }

    setOperation(Deleting);
    secret_item_delete(m_secretItem.get(), nullptr, onDeleteFinished, this);
}

SecretItem *SecretItemProxy::secretItem() const
{
    return m_secretItem.get();
}

#include "moc_secretitemproxy.cpp"
