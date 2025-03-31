// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "walletmodel.h"
#include "secretserviceclient.h"

WalletModel::WalletModel(SecretServiceClient *secretServiceClient, QObject *parent)
    : QAbstractListModel(parent)
    , m_secretServiceClient(secretServiceClient)
{
}

WalletModel::~WalletModel()
{
}

QString WalletModel::currentWallet() const
{
    return m_currentWallet;
}

void WalletModel::setCurrentWallet(const QString &wallet)
{
    if (wallet == m_currentWallet) {
        return;
    }

    m_currentWallet = wallet;

    bool ok;

    beginResetModel();
    // m_items = m_secretServiceClient->listFolders(wallet, &ok);
    const QStringList folders = m_secretServiceClient->listFolders(wallet, &ok);
    m_items.clear();
    for (const QString &folder : folders) {
        const QStringList items = m_secretServiceClient->listEntries(folder, wallet, &ok);
        for (const QString &item : items) {
            m_items << QPair<QString, QString>(folder, item);
        }
    }
    endResetModel();

    Q_EMIT(currentWalletChanged(wallet));
}

QHash<int, QByteArray> WalletModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[FolderRole] = "folder";

    return roleNames;
}

int WalletModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_items.count();
}

QVariant WalletModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_items.count() - 1) {
        return {};
    }

    const auto item = m_items[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return item.second;
    case FolderRole:
        return item.first;
    }

    return {};
}

#include "moc_walletmodel.cpp"
