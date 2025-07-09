// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class StateTracker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot create elements of type StateTracker")

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Operations operations READ operations NOTIFY operationsChanged)
    Q_PROPERTY(QString operationsReadableName READ operationsReadableName NOTIFY operationsReadableNameChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    enum State {
        // Service state
        ServiceDisconnected = 0,
        ServiceConnected = 1 << 1,

        // Item state
        ItemReady = 1 << 2,
        ItemLocked = 1 << 3,
        ItemNeedsSave = 1 << 4,

        // Collection state
        CollectionReady = 1 << 5,
        CollectionLocked = 1 << 6
    };
    Q_ENUM(State);
    Q_DECLARE_FLAGS(Status, State)

    enum Operation {
        OperationNone = 0,
        // Service operations
        ServiceConnecting = 1 << 1,
        ServiceLoadingCollections = 1 << 2,

        // Item operations
        ItemCreating = 1 << 3,
        ItemLoading = 1 << 4,
        ItemLoadingSecret = ItemLoading | 1 << 5,
        ItemUnlocking = ItemLoading | 1 << 6,
        ItemSaving = 1 << 7,
        ItemSavingLabel = ItemSaving | 1 << 8,
        ItemSavingSecret = ItemSaving | 1 << 9,
        ItemSavingAttributes = ItemSaving | 1 << 10,
        ItemDeleting = 1 << 11,

        // Collection operations
        CollectionReadingDefault = 1 << 12,
        CollectionWritingDefault = 1 << 13,
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
        // Service related errors
        ServiceConnectionError,
        ServiceLoadCollectionsError,

        // Item related errors
        ItemCreationError,
        ItemLoadError,
        ItemLoadSecretError,
        ItemUnlockError,
        ItemSaveError,
        ItemDeleteError,

        // Collection related errors
        CollectionReadDefaultError,
        CollectionWriteDefaultError,
        CollectionCreationError,
        CollectionLoadError,
        CollectionUnlockError,
        CollectionLockError,
        CollectionDeleteError
    };
    Q_ENUM(Error);

    StateTracker(QObject *parent = nullptr);
    ~StateTracker();

    static StateTracker *instance();

    Status status() const;
    void setStatus(Status status);
    void setState(State state);
    void clearState(State state);
    // Short hand to see if State has ServiceConnected
    bool isServiceConnected() const;

    Operations operations() const;
    void setOperations(Operations operations);
    void setOperation(Operation operation);
    void clearOperation(Operation operation);
    QString operationsReadableName() const;

    Error error() const;
    QString errorMessage() const;
    void setError(Error error, const QString &message);
    void clearError();

Q_SIGNALS:
    void statusChanged(Status oldStatus, Status newStatus);
    void serviceConnectedChanged(bool connected);
    void operationsChanged(Operations oldOperations, Operations newOperations);
    void operationsReadableNameChanged(const QString &name);
    void errorChanged(Error error, const QString &errorMessage);

private:
    Status m_status = ServiceDisconnected;
    Operations m_operations = OperationNone;
    Error m_error = NoError;
    QString m_errorMessage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(StateTracker::Status)
Q_DECLARE_OPERATORS_FOR_FLAGS(StateTracker::Operations)
