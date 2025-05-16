// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QDateTime>
#include <QObject>

class SecretServiceClient;

class SecretItemProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    Q_PROPERTY(bool needsSave READ needsSave NOTIFY needsSaveChanged)
    Q_PROPERTY(bool locked READ isLocked NOTIFY lockedChanged)
    Q_PROPERTY(QDateTime creationTime READ creationTime NOTIFY creationTimeChanged)
    Q_PROPERTY(QDateTime modificationTime READ modificationTime NOTIFY modificationTimeChanged)
    Q_PROPERTY(QString wallet READ wallet NOTIFY walletChanged)
    Q_PROPERTY(QString itemName READ itemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString secretValue READ secretValue WRITE setSecretValue NOTIFY secretValueChanged)

    Q_PROPERTY(QVariantMap attributes READ attributes NOTIFY attributesChanged)

public:
    SecretItemProxy(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~SecretItemProxy();

    bool isValid() const;
    bool needsSave() const;
    bool isLocked() const;
    QDateTime creationTime() const;
    QDateTime modificationTime() const;

    QString wallet() const;

    QString itemName() const;

    QString label() const;
    void setLabel(const QString &label);

    QString secretValue() const;
    void setSecretValue(const QString &secretValue);

    QVariantMap attributes() const;

    Q_INVOKABLE void loadItem(const QString &wallet, const QString &dbusPath);
    Q_INVOKABLE void save();
    Q_INVOKABLE void close();

Q_SIGNALS:
    void validChanged(bool valid);
    void needsSaveChanged(bool needsSave);
    void lockedChanged(bool locked);
    void creationTimeChanged(const QDateTime &time);
    void modificationTimeChanged(const QDateTime &time);
    void walletChanged(const QString &wallet);
    void folderChanged(const QString &folder);
    void itemNameChanged(const QString &itemName);
    void labelChanged(const QString &itemName);
    void secretValueChanged(const QString &secretValue);
    void attributesChanged(const QVariantMap &attribures);

private:
    bool m_needsSave = false;
    bool m_locked = false;
    QString m_dbusPath;
    QDateTime m_creationTime;
    QDateTime m_modificationTime;
    QString m_wallet;
    QString m_folder;
    QString m_itemName;
    QString m_label;
    QString m_secretValue;
    QVariantMap m_attributes;

    SecretItemPtr m_secretItem;
    SecretServiceClient *m_secretServiceClient = nullptr;
};
