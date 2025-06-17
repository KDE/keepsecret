// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include <QObject>

class SecretServiceClient;

class SecretOperationTracker : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Operations operations READ operations NOTIFY operationsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    enum StatusItem {
        ServiceDisconnected = 0,
        ServiceConnected = 1 << 1,

        ItemReady = 1 << 2,
        ItemLocked = 1 << 3,
        ItemNeedsSave = 1 << 4,

        CollectionReady = 1 << 5,
        CollectionLocked = 1 << 6
    };
    Q_ENUM(StatusItem);
    Q_DECLARE_FLAGS(Status, StatusItem)

    enum Operation {
        OperationNone = 0,
        ServiceConnecting = 1 << 1,
        ServiceLoadingCollections = 1 << 2,
        ServiceReadingDefaultCollection = 1 << 3,
        ServiceWritingDefaultCollection = 1 << 4,

        ItemCreating = 1 << 5,
        ItemLoading = 1 << 6,
        ItemLoadingSecret = 1 << 7,
        ItemUnlocking = ItemLoading | 1 << 8,
        ItemSaving = 1 << 9,
        ItemSavingLabel = ItemSaving | 1 << 10,
        ItemSavingSecret = ItemSaving | 1 << 11,
        ItemSavingAttributes = ItemSaving | 1 << 12,
        ItemDeleting = 1 << 13

            CollectionCreating = 1 << 14,
        CollectionLoading = 1 << 15,
        CollectionUnlocking = 1 << 16,
        CollectionLocking = 1 << 17,
        CollectionDeleting = 1 << 18
    };
    Q_ENUM(Operation);
    Q_DECLARE_FLAGS(Operations, Operation);

    enum Error {
        NoError = 0,
        ServiceConnectionFailed,
        ServiceReadDefaultCollectionFailed,
        ServiceSetDefaultCollectionFailed,

        ItemCreationFailed,
        ItemLoadFailed,
        ItemLoadSecretFailed,
        ItemUnlockFailed,
        ItemSaveFailed,
        ItemDeleteFailed,

        CollectionCreationFailed,
        CollectionLoadFailed,
        CollectionUnlockFailed,
        CollectionLockFailed,
        CollectionDeleteFailed
    };
    Q_ENUM(Error);

    SecretOperationTracker(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretOperationTracker();

    Status status() const;
    void setStatus(Status status);

    Operations operations() const;
    void setOperations(Operations operations);
    void setOperation(Operation operation);
    void clearOperation(Operation operation);

    Error error() const;
    QString errorMessage() const;
    void setError(Error error, const QString &message);

Q_SIGNALS:
    void statusChanged(Status status);
    void operationsChanged(Operations operations);
    void errorChanged(Error error);
    void errorMessageChanged(const QString &errorMessage);

private:
    Status m_status = Disconnected;
    Operations m_operations = OperationNone;
    Error m_error = NoError;
    QString m_errorMessage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SecretOperationTracker::Status)
Q_DECLARE_OPERATORS_FOR_FLAGS(SecretOperationTracker::Operations)
