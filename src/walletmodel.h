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

    Q_PROPERTY(bool locked READ isLocked NOTIFY lockedChanged)

public:
    enum Roles {
        FolderRole = Qt::UserRole + 1,
        DbusPathRole
    };
    Q_ENUM(Roles)

    WalletModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~WalletModel();

    QString currentWallet() const;
    void setCurrentWallet(const QString &wallet);

    bool isLocked() const;
    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    void loadWallet();

Q_SIGNALS:
    void currentWalletChanged(const QString &currentWallet);
    bool lockedChanged(bool locked);
    void error(const QString &error);

private:
    struct Entry {
        QString label;
        QString dbusPath;
        QString folder;
    };
    QString m_currentWallet;
    QList<Entry> m_items;
    SecretCollectionPtr m_secretCollection;
    SecretServiceClient *m_secretServiceClient = nullptr;
};
