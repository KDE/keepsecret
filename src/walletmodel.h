// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QAbstractListModel>
#include <QQmlEngine>

class SecretServiceClient;

class WalletModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Operations operations READ operations NOTIFY operationsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString currentWallet READ currentWallet WRITE setCurrentWallet NOTIFY currentWalletChanged)

public:
    enum StatusItem {
        Disconnected = 0,
        Connected = 1,
        Ready = Connected | 2,
        Locked = Connected | 4
    };
    Q_ENUM(StatusItem);
    Q_DECLARE_FLAGS(Status, StatusItem)

    enum Operation {
        OperationNone = 0,
        Creating = 1,
        Loading = 2,
        Unlocking = Loading | 4,
        Deleting = 16
    };
    Q_ENUM(Operation);
    Q_DECLARE_FLAGS(Operations, Operation);

    enum Error {
        NoError = 0,
        CreationFailed,
        LoadFailed,
        UnlockFailed,
        DeleteFailed
    };
    Q_ENUM(Error);

    enum Roles {
        FolderRole = Qt::UserRole + 1,
        DbusPathRole
    };
    Q_ENUM(Roles)

    WalletModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~WalletModel();

    Status status() const;
    void setStatus(Status status);

    Operations operations() const;
    void setOperations(Operations operations);
    void setOperation(Operation operation);
    void clearOperation(Operation operation);

    Error error() const;
    QString errorMessage() const;
    void setError(Error error, const QString &message);

    QString currentWallet() const;
    void setCurrentWallet(const QString &wallet);

    void refreshWallet();

    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void statusChanged(Status status);
    void operationsChanged(Operations operations);
    void errorChanged(Error error);
    void errorMessageChanged(const QString &errorMessage);
    void currentWalletChanged(const QString &currentWallet);
    bool lockedChanged(bool locked);

protected:
    void loadWallet();

private:
    Status m_status = Disconnected;
    Operations m_operations = OperationNone;
    Error m_error = NoError;
    QString m_errorMessage;
    struct Entry {
        QString label;
        QString dbusPath;
        QString folder;
    };
    QString m_currentWallet;
    QList<Entry> m_items;
    SecretCollectionPtr m_secretCollection;
    SecretServiceClient *m_secretServiceClient = nullptr;
    ulong m_notifyHandlerId = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WalletModel::Status)
Q_DECLARE_OPERATORS_FOR_FLAGS(WalletModel::Operations)
