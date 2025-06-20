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

    title: App.secretItem.label

    actions: [
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save"
            tooltip: i18n("Save changes")
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: App.secretItem.status === SecretItemProxy.NeedsSave
            onTriggered: App.secretItem.save()
        },
        Kirigami.Action {
            text: i18n("Revert")
            icon.name: "document-revert-symbolic"
            tooltip: i18n("Revert changes")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: App.secretItem.status === SecretItemProxy.NeedsSave
            onTriggered: App.secretItem.revert()
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this entry")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: deleteDialog.open()
        }
    ]

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: App.secretItem.error !== WalletModel.NoError
        position: Kirigami.InlineMessage.Header
        type: Kirigami.MessageType.Error
        text: App.secretItem.errorMessage
    }

    QQC.Dialog {
        id: deleteDialog
        modal: true
        header: null
        standardButtons: QQC.Dialog.Yes | QQC.Dialog.No
        contentItem: QQC.Label {
            text: i18n("Are you sure you want to delete the entry “%1”?", App.secretItem.label)
            wrapMode: Text.WordWrap
        }
        onAccepted: App.secretItem.deleteItem()
    }


    ColumnLayout {
        spacing: Kirigami.Units.gridUnit
        FormCard.FormCard {
            FormItem {
                label: i18n("Label:")
                contentItem: Kirigami.ActionTextField {
                    id: labelField
                    text: App.secretItem.label
                    rightActions: Kirigami.Action {
                        icon.name: "edit-clear"
                        visible: labelField.text.length > 0
                        onTriggered: labelField.clear()
                    }
                    onTextEdited: App.secretItem.label = text
                }
            }
            FormItem {
                visible: App.secretItem.type !== SecretServiceClient.Binary && App.secretItem.type !== SecretServiceClient.Map
                label: i18n("Password:")
                contentItem: Kirigami.PasswordField {
                    id: passwordField
                    text: App.secretItem.secretValue
                    onTextEdited: App.secretItem.secretValue = text
                }
            }
            FormItem {
                visible: App.secretItem.type === SecretServiceClient.Binary
                contentItem: ColumnLayout {
                    QQC.CheckBox {
                        id: showBinaryCheck
                        Layout.fillWidth: true
                        text: i18n("Show Binary Secret")
                    }
                    QQC.Label {
                        Layout.fillWidth: true
                        visible: showBinaryCheck.checked
                        text: visible ? App.secretItem.formattedBinarySecret : ""
                        font.family: "monospace"
                        wrapMode: Text.Wrap
                    }
                }
            }
            MapField {
                visible: App.secretItem.type === SecretServiceClient.Map
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
