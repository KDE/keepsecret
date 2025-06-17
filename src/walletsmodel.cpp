// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "walletsmodel.h"
#include "secretserviceclient.h"

WalletsModel::WalletsModel(SecretServiceClient *secretServiceClient, QObject *parent)
    : QAbstractListModel(parent)
    , m_secretServiceClient(secretServiceClient)
{
    connect(m_secretServiceClient, &SecretServiceClient::statusChanged, this, &WalletsModel::reloadWallets);
    connect(m_secretServiceClient, &SecretServiceClient::collectionListDirty, this, &WalletsModel::reloadWallets);
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
    int i = 0;
    for (const SecretServiceClient::CollectionEntry &entry : m_wallets) {
        if (entry.name == m_currentWallet) {
            return i;
        }
        ++i;
    }

    return -1;
}

QHash<int, QByteArray> WalletsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[DbusPathRole] = "dbusPath";
    roleNames[LockedRole] = "locked";

    return roleNames;
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

    const SecretServiceClient::CollectionEntry &entry = m_wallets[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return entry.name;
    case DbusPathRole:
        return entry.dbusPath;
    case LockedRole:
        return entry.locked;
    default:
        return QVariant();
    }
}

void WalletsModel::reloadWallets()
{
    beginResetModel();
    m_wallets.clear();
    if (m_secretServiceClient->status() == SecretServiceClient::Connected) {
        m_wallets = m_secretServiceClient->listCollections();
    }
    endResetModel();
    Q_EMIT currentIndexChanged();
}

#include "moc_walletsmodel.cpp"
