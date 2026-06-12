#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqmlregistration.h>

class ImportExportManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot create elements of type ImportExportManager")
public:
    explicit ImportExportManager(QObject *parent = nullptr);
    Q_INVOKABLE void exportToFile(const QString &filePath, const QString &walletName, const QVariantList &items);
    Q_INVOKABLE void importFromFile(const QString &filePath);
Q_SIGNALS:
    void exportSucceeded(const QString &filePath);
    void importSucceeded(const QVariantList &items);
    void errorOccurred(const QString &message);
};