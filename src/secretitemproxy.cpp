// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "secretitemproxy.h"
#include <qt6/QtCore/qstringliteral.h>

SecretItemProxy::SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(secretServiceClient)
{
}

SecretItemProxy::~SecretItemProxy()
{
}

bool SecretItemProxy::isValid() const
{
    return m_secretItem.get() != nullptr;
}

bool SecretItemProxy::needsSave() const
{
    return m_needsSave;
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
    m_needsSave = true;

    Q_EMIT labelChanged(label);
    Q_EMIT needsSaveChanged(m_needsSave);
}

QString SecretItemProxy::secretValue() const
{
    return m_secretValue;
}

void SecretItemProxy::setSecretValue(const QString &secretValue)
{
    if (secretValue == m_secretValue) {
        return;
    }

    m_secretValue = secretValue;
    m_needsSave = true;

    Q_EMIT secretValueChanged(secretValue);
    Q_EMIT needsSaveChanged(m_needsSave);
}

QVariantMap SecretItemProxy::attributes() const
{
    return m_attributes;
}

QString SecretItemProxy::schemaName() const
{
    return m_schemaName;
}

void SecretItemProxy::loadItem(const QString &wallet, const QString &folder, const QString &itemName)
{
    bool wasValid = isValid();
    bool ok;

    m_secretItem = m_secretServiceClient->retrieveItem(itemName, SecretServiceClient::Unknown, folder, wallet, &ok);

    if (ok) {
        m_needsSave = false;
        m_locked = secret_item_get_locked(m_secretItem.get());
        m_creationTime = QDateTime::fromSecsSinceEpoch(secret_item_get_created(m_secretItem.get()));
        m_modificationTime = QDateTime::fromSecsSinceEpoch(secret_item_get_modified(m_secretItem.get()));
        m_wallet = wallet;
        m_folder = folder;
        m_itemName = itemName;
        m_label = QString::fromUtf8(secret_item_get_label(m_secretItem.get()));
        // TODO: text vs binary vs map
        m_secretValue = QString::fromUtf8(m_secretServiceClient->readEntry(itemName, SecretServiceClient::Unknown, folder, wallet, &ok));
        m_schemaName = QString::fromUtf8(secret_item_get_schema_name(m_secretItem.get()));

        m_attributes.clear();
        GHashTablePtr attributes = GHashTablePtr(secret_item_get_attributes(m_secretItem.get()));

        if (attributes) {
            GHashTableIter attrIter;
            gpointer key, value;
            g_hash_table_iter_init(&attrIter, attributes.get());
            QStringList keys;
            while (g_hash_table_iter_next(&attrIter, &key, &value)) {
                QString keyString = QString::fromUtf8(static_cast<gchar *>(key));
                QString valueString = QString::fromUtf8(static_cast<gchar *>(value));
                m_attributes.insert(keyString, valueString);
                keys << keyString;
            }
            m_attributes[QStringLiteral("__keys")] = keys;
        }

    } else {
        m_needsSave = false;
        m_locked = false;
        m_creationTime = {};
        m_modificationTime = {};
        m_wallet = QString();
        m_folder = QString();
        m_itemName = QString();
        m_label = QString();
        m_secretValue = QString();
        m_schemaName = QString();
        m_attributes.clear();
        m_attributes[QStringLiteral("__keys")] = QStringList();
    }

    Q_EMIT needsSaveChanged(m_needsSave);
    Q_EMIT lockedChanged(m_locked);
    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged(m_secretValue);
    Q_EMIT attributesChanged(m_attributes);
    Q_EMIT schemaNameChanged(m_schemaName);

    if (wasValid != ok) {
        Q_EMIT validChanged(ok);
    }
}

void SecretItemProxy::save()
{
    bool ok;
    m_secretServiceClient->writeEntry(m_label, m_itemName, m_secretValue.toUtf8(), SecretServiceClient::PlainText, m_folder, m_wallet, &ok);

    m_needsSave = !ok;
    if (ok) {
        m_modificationTime = QDateTime::currentDateTime();
        Q_EMIT modificationTimeChanged(m_modificationTime);
    }
    Q_EMIT needsSaveChanged(m_needsSave);
}

void SecretItemProxy::close()
{
    m_needsSave = false;
    m_locked = false;
    m_creationTime = {};
    m_modificationTime = {};
    m_wallet = QString();
    m_folder = QString();
    m_itemName = QString();
    m_label = QString();
    m_secretValue = QString();
    m_folder = QString();
    m_schemaName = QString();
    m_attributes.clear();
    m_attributes[QStringLiteral("__keys")] = QStringList();

    m_secretItem.reset();

    Q_EMIT needsSaveChanged(m_needsSave);
    Q_EMIT lockedChanged(m_locked);
    Q_EMIT creationTimeChanged(m_creationTime);
    Q_EMIT modificationTimeChanged(m_modificationTime);
    Q_EMIT walletChanged(m_wallet);
    Q_EMIT folderChanged(m_folder);
    Q_EMIT itemNameChanged(m_itemName);
    Q_EMIT labelChanged(m_label);
    Q_EMIT secretValueChanged(m_secretValue);
    Q_EMIT attributesChanged(m_attributes);
    Q_EMIT schemaNameChanged(m_schemaName);
    Q_EMIT validChanged(false);
}

#include "moc_secretitemproxy.cpp"
