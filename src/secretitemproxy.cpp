// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "secretitemproxy.h"
#include "kwallets_debug.h"

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
            setStatus(Empty);
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

bool SecretItemProxy::isValid() const
{
    return m_secretServiceClient->isAvailable() && m_secretItem.get() != nullptr;
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

    if (m_status == Saving && status == Ready) {
        m_modificationTime = QDateTime::currentDateTime();
        Q_EMIT modificationTimeChanged(m_modificationTime);
    }

    qWarning() << "Setting status" << status;

    m_status = status;
    Q_EMIT statusChanged(status);
}

SecretItemProxy::SaveOperations SecretItemProxy::saveOperations() const
{
    return m_saveOperations;
}

void SecretItemProxy::setSaveOperations(SecretItemProxy::SaveOperations saveOperations)
{
    if (saveOperations == m_saveOperations) {
        return;
    }

    m_saveOperations = saveOperations;
    Q_EMIT saveOperationsChanged(saveOperations);
}

bool SecretItemProxy::isLocked() const
{
    return m_locked;
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

    if (wasErrorFree(&error)) {
        SecretValuePtr secretValue = SecretValuePtr(secret_item_get_secret(proxy->secretItem()));
        if (secretValue) {
            if (proxy->type() == SecretServiceClient::Binary) {
                gsize length = 0;
                const gchar *password = secret_value_get(secretValue.get(), &length);
                proxy->setSecretValue(QByteArray(password, length));
            }

            const gchar *password = secret_value_get_text(secretValue.get());
            if (proxy->type() == SecretServiceClient::Base64) {
                proxy->setSecretValue(QByteArray::fromBase64(QByteArray(password)));
            } else {
                proxy->setSecretValue(QByteArray(password));
            }
            proxy->setStatus(SecretItemProxy::Ready);
        } else {
            proxy->setStatus(SecretItemProxy::LoadSecretFailed);
        }
    } else {
        proxy->setStatus(SecretItemProxy::LoadSecretFailed);
    }
}

void SecretItemProxy::loadItem(const QString &wallet, const QString &dbusPath)
{
    setStatus(Loading);
    m_dbusPath = dbusPath;

    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    bool wasValid = isValid();
    bool ok;

    m_secretItem = m_secretServiceClient->retrieveItem(dbusPath, wallet, &ok);

    if (ok) {
        m_locked = secret_item_get_locked(m_secretItem.get());
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
        if (secret_item_get_locked(m_secretItem.get())) {
            unlock();
        } else {
            setStatus(LoadingSecret);
            secret_item_load_secret(m_secretItem.get(), nullptr, onLoadSecretFinish, this);
        }

    } else {
        m_locked = false;
        m_creationTime = {};
        m_modificationTime = {};
        m_wallet = QString();
        m_folder = QString();
        m_itemName = QString();
        m_label = QString();
        m_secretValue = QByteArray();
        m_attributes.clear();
        m_attributes[QStringLiteral("__keys")] = QStringList();
        setStatus(LoadFailed);
    }

    Q_EMIT lockedChanged(m_locked);
    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT attributesChanged(m_attributes);

    if (wasValid != ok) {
        Q_EMIT validChanged(ok);
    }
}

static void onItemUnlocked(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_service_unlock_finish((SecretService *)source, result, nullptr, &error);

    if (wasErrorFree(&error)) {
        proxy->setStatus(SecretItemProxy::Ready);
    } else {
        proxy->setStatus(SecretItemProxy::UnlockFailed);
    }
}

void SecretItemProxy::unlock()
{
    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    setStatus(Unlocking);
    secret_service_unlock(m_secretServiceClient->service(), g_list_append(nullptr, m_secretItem.get()), nullptr, onItemUnlocked, this);
}

static void onSetLabelFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_set_label_finish((SecretItem *)source, result, &error);

    if (wasErrorFree(&error)) {
        proxy->setSaveOperations(proxy->saveOperations() & !SecretItemProxy::SavingLabel);
        if (proxy->saveOperations() == SecretItemProxy::SaveOperationNone) {
            proxy->setStatus(SecretItemProxy::Ready);
        }
    } else {
        proxy->setStatus(SecretItemProxy::SaveFailed);
    }
}

static void onSetAttributesFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_set_attributes_finish((SecretItem *)source, result, &error);

    if (wasErrorFree(&error)) {
        proxy->setSaveOperations(proxy->saveOperations() & !SecretItemProxy::SavingAttributes);
        if (proxy->saveOperations() == SecretItemProxy::SaveOperationNone) {
            proxy->setStatus(SecretItemProxy::Ready);
        }
    } else {
        proxy->setStatus(SecretItemProxy::SaveFailed);
    }
}

void SecretItemProxy::save()
{
    if (!m_secretServiceClient->isAvailable()) {
        return;
    }

    secret_item_set_label(m_secretItem.get(), m_label.toUtf8().data(), nullptr, onSetLabelFinished, this);
    m_saveOperations |= SavingLabel;

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
        m_saveOperations |= SavingAttributes;

        // TODO: save secret
    }
}

void SecretItemProxy::close()
{
    m_locked = false;
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

    Q_EMIT lockedChanged(m_locked);
    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT attributesChanged(m_attributes);
    Q_EMIT validChanged(false);

    setStatus(Empty);
}

static void onDeleteFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_delete_finish((SecretItem *)source, result, &error);

    if (wasErrorFree(&error)) {
        proxy->setStatus(SecretItemProxy::Empty);
    } else {
        proxy->setStatus(SecretItemProxy::DeleteFailed);
    }
    proxy->close();
}

void SecretItemProxy::deleteItem()
{
    if (!isValid()) {
        return;
    }

    setStatus(Deleting);
    secret_item_delete(m_secretItem.get(), nullptr, onDeleteFinished, this);
}

SecretItem *SecretItemProxy::secretItem() const
{
    return m_secretItem.get();
}

#include "moc_secretitemproxy.cpp"
