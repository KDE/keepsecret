// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Roshani Kumari <roshnikumarii098@gmail.com>

#include "importexportmanager.h"

#include <QDomDocument>
#include <QDomElement>
#include <QFile>

ImportExportManager::ImportExportManager(QObject *parent)
    : QObject(parent)
{
}

void ImportExportManager::exportToFile(const QString &filePath, const QString &walletName, const QVariantList &items)
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("wallet"));
    root.setAttribute(QStringLiteral("name"), walletName);
    doc.appendChild(root);

    for (const QVariant &item : items) {
        QVariantMap map = item.toMap();
        QString label = map.value(QStringLiteral("label")).toString();
        QByteArray secret = map.value(QStringLiteral("secret")).toByteArray();
        QString contentType = map.value(QStringLiteral("contentType")).toString();
        QVariantMap attributes = map.value(QStringLiteral("attributes")).toMap();

        QDomElement itemEl = doc.createElement(QStringLiteral("item"));
        itemEl.setAttribute(QStringLiteral("label"), label);
        itemEl.setAttribute(QStringLiteral("contentType"), contentType);
        root.appendChild(itemEl);

        // Secret as base64
        QDomElement secretEl = doc.createElement(QStringLiteral("secret"));
        secretEl.appendChild(doc.createTextNode(
            QString::fromLatin1(secret.toBase64())));
        itemEl.appendChild(secretEl);

        // All attributes
        QDomElement attrsEl = doc.createElement(QStringLiteral("attributes"));
        for (auto a = attributes.begin(); a != attributes.end(); ++a) {
            QDomElement attr = doc.createElement(QStringLiteral("attribute"));
            attr.setAttribute(QStringLiteral("name"), a.key());
            attr.appendChild(doc.createTextNode(a.value().toString()));
            attrsEl.appendChild(attr);
        }
        itemEl.appendChild(attrsEl);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Q_EMIT errorOccurred(QStringLiteral("Cannot open file for writing: ") + filePath);
        return;
    }
    file.write(doc.toByteArray(2));
    file.close();
    Q_EMIT exportSucceeded(filePath);
}

void ImportExportManager::importFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Q_EMIT errorOccurred(QStringLiteral("Cannot open file for reading: ") + filePath);
        return;
    }

    QDomDocument doc;
    auto result = doc.setContent(&file);
    file.close();
    if (!result) {
        Q_EMIT errorOccurred(QStringLiteral("XML parse error: ") + result.errorMessage);
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName().toLower() != QStringLiteral("wallet")) {
        Q_EMIT errorOccurred(QStringLiteral("Not a valid KeepSecret XML file."));
        return;
    }

    QVariantList items;
    QDomNodeList itemNodes = root.elementsByTagName(QStringLiteral("item"));

    for (int i = 0; i < itemNodes.count(); ++i) {
        QDomElement e = itemNodes.at(i).toElement();
        QVariantMap item;
        item[QStringLiteral("label")]       = e.attribute(QStringLiteral("label"));
        item[QStringLiteral("contentType")] = e.attribute(QStringLiteral("contentType"));

        // Secret from base64
        QString secretB64 = e.firstChildElement(QStringLiteral("secret")).text();
        item[QStringLiteral("secret")] = QByteArray::fromBase64(secretB64.toLatin1());

        // All attributes
        QVariantMap attrs;
        QDomElement attrsEl = e.firstChildElement(QStringLiteral("attributes"));
        QDomNode attrChild = attrsEl.firstChild();
        while (!attrChild.isNull()) {
            QDomElement attr = attrChild.toElement();
            if (attr.tagName().toLower() == QStringLiteral("attribute")) {
                attrs[attr.attribute(QStringLiteral("name"))] = attr.text();
            }
            attrChild = attrChild.nextSibling();
        }
        item[QStringLiteral("attributes")] = attrs;
        items.append(item);
    }

    Q_EMIT importSucceeded(items);
}

void ImportExportManager::importFromKWalletXml(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Q_EMIT errorOccurred(QStringLiteral("Cannot open file for reading: ") + filePath);
        return;
    }

    QDomDocument doc;
    auto result = doc.setContent(&file);
    file.close();
    if (!result) {
        Q_EMIT errorOccurred(QStringLiteral("XML parse error: ") + result.errorMessage);
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName().toLower() != QStringLiteral("wallet")) {
        Q_EMIT errorOccurred(QStringLiteral("Not a valid KWallet XML file."));
        return;
    }

    QVariantList items;
    QDomNodeList folders = root.elementsByTagName(QStringLiteral("folder"));

    for (int i = 0; i < folders.count(); ++i) {
        QDomElement folder = folders.at(i).toElement();
        QString folderName = folder.attribute(QStringLiteral("name"));

        QDomNode child = folder.firstChild();
        while (!child.isNull()) {
            QDomElement e = child.toElement();
            QString tag = e.tagName().toLower();
            QString ename = e.attribute(QStringLiteral("name"));

            if (tag == QStringLiteral("password")) {
                QVariantMap item;
                item[QStringLiteral("label")]       = ename;
                item[QStringLiteral("secret")]      = e.text().toUtf8();
                item[QStringLiteral("contentType")] = QStringLiteral("text/plain");
                QVariantMap attrs;
                attrs[QStringLiteral("server")] = folderName;
                item[QStringLiteral("attributes")] = attrs;
                items.append(item);

            } else if (tag == QStringLiteral("stream")) {
                QVariantMap item;
                item[QStringLiteral("label")]       = ename;
                item[QStringLiteral("secret")]      = QByteArray::fromBase64(e.text().toLatin1());
                item[QStringLiteral("contentType")] = QStringLiteral("application/octet-stream");
                QVariantMap attrs;
                attrs[QStringLiteral("server")] = folderName;
                item[QStringLiteral("attributes")] = attrs;
                items.append(item);

            } else if (tag == QStringLiteral("map")) {
                QVariantMap item;
                item[QStringLiteral("label")]       = ename;
                item[QStringLiteral("secret")]      = QByteArray();
                item[QStringLiteral("contentType")] = QStringLiteral("text/plain");

                QVariantMap attrs;
                attrs[QStringLiteral("server")] = folderName;
                QDomNode mapChild = e.firstChild();
                while (!mapChild.isNull()) {
                    QDomElement mapEntry = mapChild.toElement();
                    if (mapEntry.tagName().toLower() == QStringLiteral("mapentry")) {
                        attrs[mapEntry.attribute(QStringLiteral("name"))] = mapEntry.text();
                    }
                    mapChild = mapChild.nextSibling();
                }
                item[QStringLiteral("attributes")] = attrs;
                items.append(item);
            }

            child = child.nextSibling();
        }
    }

    Q_EMIT importSucceeded(items);
}
