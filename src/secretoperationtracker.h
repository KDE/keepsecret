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
        Locking = 16,
        Deleting = 32
    };
    Q_ENUM(Operation);
    Q_DECLARE_FLAGS(Operations, Operation);

    enum Error {
        NoError = 0,
        CreationFailed,
        LoadFailed,
        UnlockFailed,
        LockFailed,
        DeleteFailed
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
