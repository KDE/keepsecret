// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "secretitemproxy.h"
#include "kwallets_debug.h"
#include "secretserviceclient.h"
#include "statetracker.h"

#include <KLocalizedString>
#include <QClipboard>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>

SecretItemProxy::SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(StateTracker::instance(), &StateTracker::serviceConnectedChanged, this, [this](bool connected) {
        if (connected) {
            loadItem(m_wallet, m_dbusPath);
        } else {
            close();
        }
    });

    connect(StateTracker::instance(),
            &StateTracker::operationsChanged,
            this,
            [this](StateTracker::Operations oldOperations, StateTracker::Operations newOperations) {
                if (oldOperations & StateTracker::ItemSaving && !(newOperations & StateTracker::ItemSaving)) {
                    m_modificationTime = QDateTime::currentDateTime();
                    Q_EMIT modificationTimeChanged(m_modificationTime);
                }
            });
}

SecretItemProxy::~SecretItemProxy()
{
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
    StateTracker::instance()->setState(StateTracker::ItemNeedsSave);
}

QString SecretItemProxy::secretValue() const
{
    return QString::fromUtf8(m_secretValue);
}

// FIXME: remove this method
void SecretItemProxy::setSecretValue(const QString &secretValue)
{
    const QString stringValue = QString::fromUtf8(m_secretValue);
    if (secretValue == stringValue) {
        return;
    }

    m_secretValue = secretValue.toUtf8();

    Q_EMIT secretValueChanged();
    StateTracker::instance()->setState(StateTracker::ItemNeedsSave);
}

void SecretItemProxy::setSecretValue(const QByteArray &secretValue)
{
    if (secretValue == m_secretValue) {
        return;
    }

    m_secretValue = secretValue;

    Q_EMIT secretValueChanged();
    StateTracker::instance()->setState(StateTracker::ItemNeedsSave);
}

QString SecretItemProxy::formattedBinarySecret() const
{
    QString formatted;
    int i = 0;
    const QString hex = QString::fromUtf8(m_secretValue.toHex());

    for (auto &c : hex) {
        formatted.append(c);
        if ((i + 1) % 8 == 0) {
            formatted.append(QLatin1Char(' '));
        }
        ++i;
    }

    formatted.append(QStringLiteral(" \n\n"));
    formatted.append(QString::fromLatin1(m_secretValue));

    return formatted;
}

QVariantMap SecretItemProxy::attributes() const
{
    return m_attributes;
}

void SecretItemProxy::setAttribute(const QString &key, const QString &value)
{
    if (!m_attributes.contains(key)) {
        return;
    }

    m_attributes[key] = value;
    StateTracker::instance()->setState(StateTracker::ItemNeedsSave);
}

void SecretItemProxy::copySecret()
{
    qApp->clipboard()->setText(QString::fromUtf8(m_secretValue));
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
            StateTracker::instance()->clearError();
            StateTracker::instance()->setState(StateTracker::ItemReady);
        } else {
            StateTracker::instance()->setError(StateTracker::ItemLoadSecretError, i18n("Couldn't retrieve the secret value"));
        }
    } else {
        StateTracker::instance()->setError(StateTracker::ItemLoadSecretError, message);
    }
    StateTracker::instance()->clearOperation(StateTracker::ItemLoadingSecret);
}

static void onItemCreateFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(source);
    Q_UNUSED(inst);
    GError *error = nullptr;
    QString message;

    secret_item_create_finish(result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::ItemCreationError, message);
    }
    StateTracker::instance()->clearOperation(StateTracker::ItemCreating);
}

void SecretItemProxy::createItem(const QString &label,
                                 const QByteArray &secret,
                                 // const SecretServiceClient::Type type,
                                 const QString &user,
                                 const QString &server,
                                 const QString &collectionPath)
{
    qWarning() << label << secret << user << server << collectionPath;
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    // TODO: make it a paramenter?
    const SecretServiceClient::Type type = SecretServiceClient::PlainText;

    SecretCollection *collection = m_secretServiceClient->retrieveCollection(collectionPath);

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
        StateTracker::instance()->setError(StateTracker::ItemCreationError, i18n("Failed to create SecretValue"));
        return;
    } else {
        StateTracker::instance()->clearError();
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

    StateTracker::instance()->setOperation(StateTracker::ItemCreating);
}

void SecretItemProxy::loadItem(const QString &collectionPath, const QString &itemPath)
{
    m_dbusPath = itemPath;
    m_collectionPath = collectionPath;

    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    bool ok;

    m_secretItem = m_secretServiceClient->retrieveItem(itemPath, collectionPath, &ok);

    if (ok) {
        if (secret_item_get_locked(m_secretItem.get())) {
            StateTracker::instance()->setState(StateTracker::ItemLocked);
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
            while (g_hash_table_iter_next(&attrIter, &key, &value)) {
                QString keyString = QString::fromUtf8(static_cast<gchar *>(key));
                QString valueString = QString::fromUtf8(static_cast<gchar *>(value));
                m_attributes.insert(keyString, valueString);
                if (keyString == QStringLiteral("type")) {
                    m_type = m_secretServiceClient->stringToType(valueString);
                }
            }
        }
        if (StateTracker::instance()->status() & StateTracker::ItemLocked) {
            unlock();
        } else {
            StateTracker::instance()->setOperation(StateTracker::ItemLoadingSecret);
            secret_item_load_secret(m_secretItem.get(), nullptr, onLoadSecretFinish, this);
        }

        StateTracker::instance()->clearError();

    } else {
        m_creationTime = {};
        m_modificationTime = {};
        m_wallet = QString();
        m_folder = QString();
        m_itemName = QString();
        m_label = QString();
        m_secretValue = QByteArray();
        m_type = SecretServiceClient::PlainText;
        m_attributes.clear();
        StateTracker::instance()->setError(StateTracker::ItemLoadError, QStringLiteral("Failed to load the secret item"));
    }

    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT typeChanged(m_type);
    Q_EMIT attributesChanged(m_attributes);
}

static void onItemUnlockFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(inst);
    GError *error = nullptr;
    QString message;

    secret_service_unlock_finish((SecretService *)source, result, nullptr, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::ItemUnlockError, message);
    }

    StateTracker::instance()->clearOperation(StateTracker::ItemUnlocking);
}

void SecretItemProxy::unlock()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::ItemUnlocking);
    secret_service_unlock(m_secretServiceClient->service(), g_list_append(nullptr, m_secretItem.get()), nullptr, onItemUnlockFinished, this);
}

static void onSetLabelFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(inst);
    GError *error = nullptr;
    QString message;

    secret_item_set_label_finish((SecretItem *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        StateTracker::instance()->clearOperation(StateTracker::ItemSavingLabel);
    } else {
        StateTracker::instance()->setError(StateTracker::ItemSaveError, message);
    }
}

static void onSetAttributesFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(inst);
    GError *error = nullptr;
    QString message;

    secret_item_set_attributes_finish((SecretItem *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        StateTracker::instance()->clearOperation(StateTracker::ItemSavingAttributes);
    } else {
        StateTracker::instance()->setError(StateTracker::ItemSaveError, message);
    }
}

static void onSetSecretFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    Q_UNUSED(inst);
    GError *error = nullptr;
    QString message;

    secret_item_set_secret_finish((SecretItem *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
        StateTracker::instance()->clearOperation(StateTracker::ItemSavingSecret);
    } else {
        StateTracker::instance()->setError(StateTracker::ItemSaveError, message);
    }
}

void SecretItemProxy::save()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    secret_item_set_label(m_secretItem.get(), m_label.toUtf8().data(), nullptr, onSetLabelFinished, this);
    StateTracker::instance()->setOperation(StateTracker::ItemSavingLabel);

    // Only attributes of type org.qt.keychain can be saved
    if (m_attributes.contains(QStringLiteral("xdg:schema")) && m_attributes[QStringLiteral("xdg:schema")] == QStringLiteral("org.qt.keychain")) {
        GHashTable *attributes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        for (auto it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it) {
            QByteArray keyBytes = it.key().toUtf8();
            gchar *key = g_strdup(keyBytes.constData());

            QByteArray valueBytes = it.value().toString().toUtf8();
            gchar *value = g_strdup(valueBytes.constData());

            g_hash_table_insert(attributes, key, value);
        }

        secret_item_set_attributes(m_secretItem.get(), SecretServiceClient::qtKeychainSchema(), attributes, nullptr, onSetAttributesFinished, this);
        StateTracker::instance()->setOperation(StateTracker::ItemSavingAttributes);

        SecretValuePtr secretValue;
        if (m_type == SecretServiceClient::Base64) {
            secretValue.reset(secret_value_new(m_secretValue.toBase64().data(), m_secretValue.length(), "text/plain"));
        } else {
            secretValue.reset(secret_value_new(m_secretValue.data(), m_secretValue.length(), "text/plain"));
        }

        // Saving binary secrets not supported yet
        if (m_type != SecretServiceClient::Binary) {
            secret_item_set_secret(m_secretItem.get(), secretValue.get(), nullptr, onSetSecretFinished, this);
            StateTracker::instance()->setOperation(StateTracker::ItemSavingSecret);
        }
    }
}

void SecretItemProxy::revert()
{
    if (!StateTracker::instance()->isServiceConnected()) {
        return;
    }

    loadItem(m_collectionPath, m_dbusPath);
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
    m_type = SecretServiceClient::PlainText;
    m_folder = QString();
    m_attributes.clear();

    m_secretItem.reset();

    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged();
    Q_EMIT typeChanged(m_type);
    Q_EMIT attributesChanged(m_attributes);

    StateTracker::instance()->setStatus(StateTracker::instance()->status()
                                        & ~(StateTracker::ItemReady | StateTracker::ItemLocked | StateTracker::ItemNeedsSave));
}

static void onDeleteFinished(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QString message;
    SecretItemProxy *proxy = (SecretItemProxy *)inst;

    secret_item_delete_finish((SecretItem *)source, result, &error);

    if (SecretServiceClient::wasErrorFree(&error, message)) {
        StateTracker::instance()->clearError();
    } else {
        StateTracker::instance()->setError(StateTracker::ItemDeleteError, message);
    }
    proxy->close();
    StateTracker::instance()->clearOperation(StateTracker::ItemDeleting);
}

void SecretItemProxy::deleteItem()
{
    if (!(StateTracker::instance()->status() & StateTracker::ServiceConnected)) {
        return;
    }

    StateTracker::instance()->setOperation(StateTracker::ItemDeleting);
    secret_item_delete(m_secretItem.get(), nullptr, onDeleteFinished, this);
}

SecretItem *SecretItemProxy::secretItem() const
{
    return m_secretItem.get();
}

#include "moc_secretitemproxy.cpp"
