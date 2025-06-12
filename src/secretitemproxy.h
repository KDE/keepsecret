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

    Q_PROPERTY(SecretItemProxy::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    Q_PROPERTY(bool locked READ isLocked NOTIFY lockedChanged)
    Q_PROPERTY(QDateTime creationTime READ creationTime NOTIFY creationTimeChanged)
    Q_PROPERTY(QDateTime modificationTime READ modificationTime NOTIFY modificationTimeChanged)
    Q_PROPERTY(QString wallet READ wallet NOTIFY walletChanged)
    Q_PROPERTY(QString itemName READ itemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString secretValue READ secretValue WRITE setSecretValue NOTIFY secretValueChanged)

    Q_PROPERTY(QVariantMap attributes READ attributes NOTIFY attributesChanged)

public:
    enum Status {
        Disconnected,
        Empty,
        Loading,
        LoadingSecret,
        Locked,
        Ready,
        NeedsSave,
        Unlocking,
        Saving,
        LoadFailed,
        LoadSecretFailed,
        UnlockFailed,
        SaveFailed,
        Deleting,
        DeleteFailed
    };
    Q_ENUM(Status);

    enum SaveOperation {
        SaveOperationNone = 0,
        SavingLabel = 1,
        SavingSecret = 2,
        SavingAttributes = 4
    };
    Q_ENUM(SaveOperation);
    Q_DECLARE_FLAGS(SaveOperations, SaveOperation)

    SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretItemProxy();

    bool isValid() const;

    Status status() const;
    void setStatus(Status status);

    SaveOperations saveOperations() const;
    void setSaveOperations(SaveOperations saveOperations);

    // TODO: collapse those in Status
    bool isLocked() const;
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

    Q_INVOKABLE void loadItem(const QString &wallet, const QString &dbusPath);
    Q_INVOKABLE void unlock();
    Q_INVOKABLE void save();
    Q_INVOKABLE void close();
    Q_INVOKABLE void deleteItem();

    SecretItem *secretItem() const;

Q_SIGNALS:
    void statusChanged(Status status);
    void saveOperationsChanged(SaveOperations saveOperations);
    void validChanged(bool valid);
    void lockedChanged(bool locked);
    void creationTimeChanged(const QDateTime &time);
    void modificationTimeChanged(const QDateTime &time);
    void walletChanged(const QString &wallet);
    void folderChanged(const QString &folder);
    void itemNameChanged(const QString &itemName);
    void labelChanged(const QString &itemName);
    void secretValueChanged();
    void attributesChanged(const QVariantMap &attribures);

private:
    bool m_locked = false;
    Status m_status = Disconnected;
    SaveOperations m_saveOperations = SaveOperationNone;
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

Q_DECLARE_OPERATORS_FOR_FLAGS(SecretItemProxy::SaveOperations)
