// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "secretitemproxy.h"
#include "secretserviceclient.h"

SecretItemProxy::SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent)
    : QObject(parent)
    , m_secretServiceClient(secretServiceClient)
{
}

SecretItemProxy::~SecretItemProxy()
{
}

QString SecretItemProxy::wallet() const
{
    return m_wallet;
}

void SecretItemProxy::setWallet(const QString &wallet)
{
    if (wallet == m_wallet) {
        return;
    }

    m_wallet = wallet;

    Q_EMIT(walletChanged(wallet));
}

QString SecretItemProxy::itemName() const
{
    return m_itemName;
}

void SecretItemProxy::setItemName(const QString &itemName)
{
    if (itemName == m_itemName) {
        return;
    }

    m_itemName = itemName;

    Q_EMIT(itemNameChanged(itemName));
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

    Q_EMIT(secretValueChanged(secretValue));
}

void SecretItemProxy::loadItem(const QString &wallet, const QString &folder, const QString &itemName)
{
    setWallet(wallet);
    // setFolder(folder);
    setItemName(itemName);
    bool ok;
    setSecretValue(QString::fromUtf8(m_secretServiceClient->readEntry(itemName, SecretServiceClient::PlainText, folder, wallet, &ok)));
}

#include "moc_secretitemproxy.cpp"
