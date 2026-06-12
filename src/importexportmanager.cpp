// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Roshani Kumari <roshnikumariii098@gmail.com>

#include "importexportmanager.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>

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

    QMap<QString, QList<QVariantMap>> folders;
    for (const QVariant &item : items) {
        QVariantMap map = item.toMap();
        QString folder = map.value(QStringLiteral("folder"), QStringLiteral("Passwords")).toString();
        folders[folder].append(map);
    }

    for (auto it = folders.begin(); it != folders.end(); ++it) {
        QDomElement folderEl = doc.createElement(QStringLiteral("folder"));
        folderEl.setAttribute(QStringLiteral("name"), it.key());
        root.appendChild(folderEl);

        for (const QVariantMap &item : it.value()) {
            QString label = item.value(QStringLiteral("label")).toString();
            QString type  = item.value(QStringLiteral("type")).toString();

            if (type == QStringLiteral("Map")) {
                QDomElement mapEl = doc.createElement(QStringLiteral("map"));
                mapEl.setAttribute(QStringLiteral("name"), label);
                QVariantMap attrs = item.value(QStringLiteral("attributes")).toMap();
                for (auto a = attrs.begin(); a != attrs.end(); ++a) {
                    QDomElement entry = doc.createElement(QStringLiteral("mapentry"));
                    entry.setAttribute(QStringLiteral("name"), a.key());
                    entry.appendChild(doc.createTextNode(a.value().toString()));
                    mapEl.appendChild(entry);
                }
                folderEl.appendChild(mapEl);

            } else if (type == QStringLiteral("Binary") || type == QStringLiteral("Base64")) {
                QDomElement streamEl = doc.createElement(QStringLiteral("stream"));
                streamEl.setAttribute(QStringLiteral("name"), label);
                QByteArray raw = item.value(QStringLiteral("secretValue")).toString().toUtf8();
                streamEl.appendChild(doc.createTextNode(
                    QString::fromLatin1(raw.toBase64())));
                folderEl.appendChild(streamEl);

            } else {
                QDomElement passEl = doc.createElement(QStringLiteral("password"));
                passEl.setAttribute(QStringLiteral("name"), label);
                passEl.appendChild(doc.createTextNode(
                    item.value(QStringLiteral("secretValue")).toString()));
                folderEl.appendChild(passEl);
            }
        }
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
                item[QStringLiteral("secretValue")] = e.text();
                item[QStringLiteral("type")]        = QStringLiteral("PlainText");
                item[QStringLiteral("folder")]      = folderName;
                items.append(item);

            } else if (tag == QStringLiteral("stream")) {
                QVariantMap item;
                item[QStringLiteral("label")]       = ename;
                item[QStringLiteral("secretValue")] = QString::fromUtf8(
                    QByteArray::fromBase64(e.text().toLatin1()));
                item[QStringLiteral("type")]        = QStringLiteral("Binary");
                item[QStringLiteral("folder")]      = folderName;
                items.append(item);

            } else if (tag == QStringLiteral("map")) {
                QVariantMap item;
                item[QStringLiteral("label")]  = ename;
                item[QStringLiteral("type")]   = QStringLiteral("Map");
                item[QStringLiteral("folder")] = folderName;
                QVariantMap attrs;
                QDomNode mapChild = e.firstChild();
                while (!mapChild.isNull()) {
                    QDomElement mapEntry = mapChild.toElement();
                    if (mapEntry.tagName().toLower() == QStringLiteral("mapentry")) {
                        attrs[mapEntry.attribute(QStringLiteral("name"))] =
                            mapEntry.text();
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