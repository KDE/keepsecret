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

    title: QQC.ApplicationWindow.window.pageStack.wideMode ? "" : App.secretItem.label

    actions: [
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save"
            tooltip: i18n("Save changes")
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: App.secretItem.status === SecretItemProxy.NeedsSave
            onTriggered: App.secretItem.save()
        },Kirigami.Action {
            text: i18n("Revert")
            icon.name: "document-revert-symbolic"
            tooltip: i18n("Revert changes")
            //displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: App.secretItem.status === SecretItemProxy.NeedsSave
            onTriggered: App.secretItem.revert()
        },
        Kirigami.Action {
            text: i18n("Copy")
            icon.name: "edit-copy-symbolic"
            tooltip: i18n("Copy the secret password in the clipboard")
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: App.secretItem.secretValue.length > 0
            onTriggered: App.secretItem.copySecret()
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this entry")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                showDeleteDialog(
                    i18n("Are you sure you want to delete the item “%1”?", App.secretItem.label),
                         i18n("I understand that the item will be permanently deleted"),
                         () => {
                             App.secretItem.deleteItem()
                         });
            }
        }
    ]

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: App.secretItem.error !== WalletModel.NoError
        position: Kirigami.InlineMessage.Header
        type: Kirigami.MessageType.Error
        text: App.secretItem.errorMessage
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
                model: Object.keys(App.secretItem.attributes)
                delegate: FormItem {
                    label: modelData + ":"
                    contentItem: QQC.TextField {
                        text: App.secretItem.attributes[modelData]
                        readOnly: App.secretItem.attributes["xdg:schema"] !== "org.qt.keychain" ||
                            (modelData !== "server" && modelData !== "user")
                        background.visible: !readOnly
                        onTextEdited: App.secretItem.setAttribute(modelData, text)
                    }
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
