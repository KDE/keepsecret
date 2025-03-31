// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    title: App.secretItem.itemName

    actions: [
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save"
            tooltip: i18n("Save changes")
            enabled: App.secretItem.needsSave
            onTriggered: App.secretItem.save()
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this entry")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                print("delete")
            }
        }
    ]

    ColumnLayout {
        QQC.Label {
            Layout.fillWidth: true
            text: i18n("Label:")
        }
        QQC.TextField {
            id: labelField
            Layout.fillWidth: true
            text: App.secretItem.label
            onTextEdited: App.secretItem.label = text
        }
        QQC.Label {
            Layout.fillWidth: true
            text: i18n("Password:")
        }
        Kirigami.PasswordField {
            id: passwordField
            Layout.fillWidth: true
            text: App.secretItem.secretValue
            onTextEdited: App.secretItem.secretValue = text
        }

        Repeater {
            model: App.secretItem.attributes.__keys
            delegate: RowLayout {
                //TODO: hide for kwallet-type entries?
                QQC.Label {
                    Layout.fillWidth: true
                    text: modelData
                }
                QQC.Label {
                    text: App.secretItem.attributes[modelData]
                }
            }
        }

        RowLayout {
            QQC.Label {
                Layout.fillWidth: true
                text: i18n("Created")
            }
            QQC.Label {
                text: Qt.formatDateTime(App.secretItem.creationTime, Locale.LongFormat)
            }
        }

        RowLayout {
            QQC.Label {
                Layout.fillWidth: true
                text: i18n("Modified")
            }
            QQC.Label {
                text: Qt.formatDateTime(App.secretItem.modificationTime, Locale.LongFormat)
            }
        }
    }
}
