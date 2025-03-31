// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QAbstractListModel>

class SecretServiceClient;

class WalletModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString currentWallet READ currentWallet WRITE setCurrentWallet NOTIFY currentWalletChanged)

public:
    enum Roles {
        FolderRole = Qt::UserRole + 1
    };
    Q_ENUM(Roles)

    WalletModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~WalletModel();

    QString currentWallet() const;
    void setCurrentWallet(const QString &wallet);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void currentWalletChanged(const QString &currentWallet);

private:
    QString m_currentWallet;
    QList<QPair<QString, QString>> m_items;
    SecretServiceClient *m_secretServiceClient = nullptr;
};
