// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QObject>

class SecretServiceClient;

class SecretItemProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString wallet READ wallet WRITE setWallet NOTIFY walletChanged)
    Q_PROPERTY(QString itemName READ itemName WRITE setItemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString secretValue READ secretValue WRITE setSecretValue NOTIFY secretValueChanged)

public:
    SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretItemProxy();

    QString wallet() const;
    void setWallet(const QString &wallet);

    QString itemName() const;
    void setItemName(const QString &itemName);

    QString secretValue() const;
    void setSecretValue(const QString &secretValue);

    Q_INVOKABLE void loadItem(const QString &wallet, const QString &folder, const QString &item);

Q_SIGNALS:
    void walletChanged(const QString &wallet);
    void itemNameChanged(const QString &itemName);
    void secretValueChanged(const QString &secretValue);

private:
    QString m_wallet;
    QString m_folder;
    QString m_itemName;
    QString m_secretValue;

    SecretServiceClient *m_secretServiceClient = nullptr;
};
