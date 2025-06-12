// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QAbstractListModel>

class SecretServiceClient;

class WalletsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)

public:
    WalletsModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~WalletsModel();

    QString currentWallet() const;
    void setCurrentWallet(const QString &wallet);

    int currentIndex() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void currentIndexChanged();

private:
    SecretServiceClient *m_secretServiceClient = nullptr;
    QStringList m_wallets;
    QString m_currentWallet;
};
