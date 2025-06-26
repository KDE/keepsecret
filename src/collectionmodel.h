// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#pragma once

#include "secretserviceclient.h"
#include <QAbstractListModel>
#include <QQmlEngine>

class SecretServiceClient;

class CollectionModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot create elements of type CollectionModel")

    Q_PROPERTY(QString collectionName READ collectionName NOTIFY collectionNameChanged)
    Q_PROPERTY(QString collectionPath READ collectionPath WRITE setCollectionPath NOTIFY collectionPathChanged)

public:
    enum Roles {
        FolderRole = Qt::UserRole + 1,
        DbusPathRole
    };
    Q_ENUM(Roles)

    CollectionModel(SecretServiceClient *secretServiceClient, QObject *parent = nullptr);
    ~CollectionModel();

    QString collectionName() const;

    QString collectionPath() const;
    void setCollectionPath(const QString &collectionPath);

    void refreshWallet();

    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void collectionNameChanged(const QString &name);
    void collectionPathChanged(const QString &collectionPath);
    bool lockedChanged(bool locked);

protected:
    void loadWallet();

private:
    struct Entry {
        QString label;
        QString dbusPath;
        QString folder;
    };
    QString m_currentCollectionPath;
    QList<Entry> m_items;
    SecretCollectionPtr m_secretCollection;
    SecretServiceClient *m_secretServiceClient = nullptr;
    ulong m_notifyHandlerId = 0;
};
