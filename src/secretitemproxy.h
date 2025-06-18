// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QDateTime>
#include <QObject>
#include <qqmlregistration.h>

class SecretServiceClient;

class SecretItemProxy : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Operations operations READ operations NOTIFY operationsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QDateTime creationTime READ creationTime NOTIFY creationTimeChanged)
    Q_PROPERTY(QDateTime modificationTime READ modificationTime NOTIFY modificationTimeChanged)
    Q_PROPERTY(QString wallet READ wallet NOTIFY walletChanged)
    Q_PROPERTY(QString itemName READ itemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString secretValue READ secretValue WRITE setSecretValue NOTIFY secretValueChanged)

    Q_PROPERTY(QVariantMap attributes READ attributes NOTIFY attributesChanged)

public:
    enum StatusItem {
        Disconnected = 0,
        Connected = 1,
        Ready = Connected | 2,
        Locked = Connected | 4,
        NeedsSave = Ready | 8,
    };
    Q_ENUM(StatusItem);
    Q_DECLARE_FLAGS(Status, StatusItem)

    enum Operation {
        OperationNone = 0,
        Creating = 1,
        Loading = 2,
        LoadingSecret = Loading | 4,
        Unlocking = Loading | 8,
        Saving = 16,
        SavingLabel = Saving | 32,
        SavingSecret = Saving | 64,
        SavingAttributes = Saving | 128,
        Deleting = 256
    };
    Q_ENUM(Operation);
    Q_DECLARE_FLAGS(Operations, Operation);

    enum Error {
        NoError = 0,
        CreationFailed,
        LoadFailed,
        LoadSecretFailed,
        UnlockFailed,
        SaveFailed,
        DeleteFailed
    };
    Q_ENUM(Error);

    SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretItemProxy();

    Status status() const;
    void setStatus(Status status);

    Operations operations() const;
    void setOperations(Operations operations);
    void setOperation(Operation operation);
    void clearOperation(Operation operation);

    Error error() const;
    QString errorMessage() const;
    void setError(Error error, const QString &message);

    QDateTime creationTime() const;
    QDateTime modificationTime() const;

    QString wallet() const;

    QString itemName() const;

    QString label() const;
    void setLabel(const QString &label);

    QString secretValue() const;
    void setSecretValue(const QString &secretValue);
    void setSecretValue(const QByteArray &secretValue);

    QVariantMap attributes() const;

    SecretServiceClient::Type type() const;

    Q_INVOKABLE void createItem(const QString &label,
                                const QByteArray &secret,
                                // const SecretServiceClient::Type type,
                                const QString &user,
                                const QString &server,
                                const QString &collectionPath);
    Q_INVOKABLE void loadItem(const QString &collectionPath, const QString &itemPath);
    Q_INVOKABLE void unlock();
    Q_INVOKABLE void save();
    Q_INVOKABLE void close();
    Q_INVOKABLE void deleteItem();

    SecretItem *secretItem() const;

Q_SIGNALS:
    void statusChanged(Status status);
    void operationsChanged(Operations operations);
    void errorChanged(Error error);
    void errorMessageChanged(const QString &errorMessage);
    void creationTimeChanged(const QDateTime &time);
    void modificationTimeChanged(const QDateTime &time);
    void walletChanged(const QString &wallet);
    void folderChanged(const QString &folder);
    void itemNameChanged(const QString &itemName);
    void labelChanged(const QString &itemName);
    void secretValueChanged();
    void attributesChanged(const QVariantMap &attribures);

private:
    Status m_status = Disconnected;
    Operations m_operations = OperationNone;
    Error m_error = NoError;
    QString m_errorMessage;
    SecretServiceClient::Type m_type = SecretServiceClient::Unknown;
    QString m_dbusPath;
    QDateTime m_creationTime;
    QDateTime m_modificationTime;
    QString m_wallet;
    QString m_folder;
    QString m_itemName;
    QString m_label;
    QByteArray m_secretValue;
    QVariantMap m_attributes;

    SecretItemPtr m_secretItem;
    SecretServiceClient *m_secretServiceClient = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SecretItemProxy::Status)
Q_DECLARE_OPERATORS_FOR_FLAGS(SecretItemProxy::Operations)
