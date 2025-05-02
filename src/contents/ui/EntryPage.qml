// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
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
        FormCard.FormCard {
            FormCard.FormTextFieldDelegate {
                text: App.secretItem.label
                label: i18n("Label:")
                onTextEdited: App.secretItem.label = text
            }
            FormCard.FormTextFieldDelegate {
                text: App.secretItem.secretValue
                label: i18n("Password:")
                echoMode: TextInput.Password
                onTextEdited: App.secretItem.secretValue = text
            }
        }

        FormCard.FormCard {
            Repeater {
                model: App.secretItem.attributes.__keys
                delegate: FormCard.FormTextDelegate {
                    text: modelData
                    description: App.secretItem.attributes[modelData]
                }
            }
        }

        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18n("Created")
                description: Qt.formatDateTime(App.secretItem.creationTime, Locale.LongFormat)
            }
            FormCard.FormTextDelegate {
                text: i18n("Modified")
                description: Qt.formatDateTime(App.secretItem.modificationTime, Locale.LongFormat)
            }
        }
    }
}
