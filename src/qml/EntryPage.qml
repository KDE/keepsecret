// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kirigami as Kirigami
import org.kde.keepsecret

Kirigami.ScrollablePage {
    id: page

    title: QQC.ApplicationWindow.window.pageStack.wideMode ? "" : App.secretItem.label

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button Save changes made to this secret", "Save")
            icon.name: "document-save"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: App.stateTracker.status & StateTracker.ItemNeedsSave
            onTriggered: App.secretItem.save()
        },Kirigami.Action {
            text: i18nc("@action:button Revert changes made to this secret", "Revert")
            icon.name: "document-revert-symbolic"
            enabled: App.stateTracker.status & StateTracker.ItemNeedsSave
            onTriggered: App.secretItem.revert()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Copy Password")
            icon.name: "password-copy-symbolic"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: App.secretItem.secretValue.length > 0
            onTriggered: App.secretItem.copySecret()
        },
        Kirigami.Action {
            text: i18nc("@action:button Delete this secret", "Delete")
            icon.name: "delete-symbolic"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                showDeleteDialog(
                    i18nc("@label", "Are you sure you want to delete the item “%1”?", App.secretItem.label),
                         i18nc("@option:check", "I understand that the item will be permanently deleted"),
                         () => {
                             App.secretItem.deleteItem()
                         });
            }
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.gridUnit
        FormCard.FormCard {
            FormItem {
                label: i18nc("@label:textbox Name of this secret", "Label:")
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
                label: i18nc("@label:textbox Password for this secret","Password:")
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
                        text: i18nc("@option:check", "Show binary secret")
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
                text: i18nc("@label The time this secret was created", "Created:")
                description: Qt.formatDateTime(App.secretItem.creationTime, Locale.LongFormat)
            }
            FormCard.FormTextDelegate {
                text: i18nc("@label The time this secret was modified", "Modified:")
                description: Qt.formatDateTime(App.secretItem.modificationTime, Locale.LongFormat)
            }
        }
    }
}
