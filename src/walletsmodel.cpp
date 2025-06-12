// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "walletsmodel.h"
#include "secretserviceclient.h"

WalletsModel::WalletsModel(SecretServiceClient *secretServiceClient, QObject *parent)
    : QAbstractListModel(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(m_secretServiceClient, &SecretServiceClient::statusChanged, this, [this](SecretServiceClient::Status status) {
        beginResetModel();
        if (status == SecretServiceClient::Connected) {
            bool ok;
            m_wallets = m_secretServiceClient->listCollections(&ok);
        } else {
            m_wallets.clear();
        }
        endResetModel();
        Q_EMIT currentIndexChanged();
    });
}

WalletsModel::~WalletsModel()
{
}

QString WalletsModel::currentWallet() const
{
    return m_currentWallet;
}

void WalletsModel::setCurrentWallet(const QString &wallet)
{
    if (wallet == m_currentWallet) {
        return;
    }

    m_currentWallet = wallet;

    Q_EMIT currentIndexChanged();
}

int WalletsModel::currentIndex() const
{
    return m_wallets.indexOf(m_currentWallet);
}

int WalletsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_wallets.count();
}

QVariant WalletsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_wallets.count() - 1) {
        return {};
    }

    return m_wallets[index.row()];
}

#include "moc_walletsmodel.cpp"
