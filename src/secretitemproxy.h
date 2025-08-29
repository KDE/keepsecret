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
    QML_UNCREATABLE("Cannot create elements of type SecretItemProxy")

    Q_PROPERTY(QDateTime creationTime READ creationTime NOTIFY creationTimeChanged)
    Q_PROPERTY(QDateTime modificationTime READ modificationTime NOTIFY modificationTimeChanged)
    Q_PROPERTY(QString wallet READ wallet NOTIFY walletChanged)
    Q_PROPERTY(QString itemName READ itemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString secretValue READ secretValue WRITE setSecretValue NOTIFY secretValueChanged)
    Q_PROPERTY(SecretServiceClient::Type type READ type NOTIFY typeChanged)
    Q_PROPERTY(QString formattedBinarySecret READ formattedBinarySecret NOTIFY secretValueChanged)

    Q_PROPERTY(QVariantMap attributes READ attributes NOTIFY attributesChanged)

public:
    explicit SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretItemProxy() override;

    QDateTime creationTime() const;
    QDateTime modificationTime() const;

    QString wallet() const;

    QString itemName() const;

    QString label() const;
    void setLabel(const QString &label);

    QString secretValue() const;
    void setSecretValue(const QString &secretValue);
    void setSecretValue(const QByteArray &secretValue);

    QString formattedBinarySecret() const;

    QVariantMap attributes() const;
    Q_INVOKABLE void setAttribute(const QString &key, const QString &value);
    Q_INVOKABLE void copySecret();

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
    Q_INVOKABLE void revert();
    Q_INVOKABLE void close();
    Q_INVOKABLE void deleteItem();

    SecretItem *secretItem() const;

Q_SIGNALS:
    void itemLoaded();
    void itemSaved();
    void creationTimeChanged(const QDateTime &time);
    void modificationTimeChanged(const QDateTime &time);
    void walletChanged(const QString &wallet);
    void folderChanged(const QString &folder);
    void itemNameChanged(const QString &itemName);
    void labelChanged(const QString &itemName);
    void secretValueChanged();
    void typeChanged(SecretServiceClient::Type type);
    void attributesChanged(const QVariantMap &attribures);

private:
    QString m_dbusPath;
    QString m_collectionPath;
    SecretServiceClient::Type m_type = SecretServiceClient::Unknown;
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
