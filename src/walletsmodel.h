// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QAbstractListModel>

class SecretServiceClient;

class WalletsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    WalletsModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~WalletsModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    SecretServiceClient *m_secretServiceClient = nullptr;
    QStringList m_wallets;
};
