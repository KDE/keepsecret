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
            enabled: App.secretItem.status === SecretItemProxy.NeedsSave
            onTriggered: App.secretItem.save()
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this entry")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: deleteDialog.open()
        }
    ]

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

    // TODO: Future Form layout things are needed like that, as can't even have a show password button in FormTextFieldDelegate
    component FormItem: Kirigami.HeaderFooterLayout {
        id: layout
        property string label
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        Layout.topMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        Layout.bottomMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.mediumSpacing
        header: QQC.Label {
            visible: text.length > 0
            text: layout.label
        }
    }


    ColumnLayout {
        spacing: Kirigami.Units.gridUnit
        FormCard.FormCard {
            Text { //TODO: remove
                text: "Type" + App.secretItem.type
            }
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
                visible: App.secretItem.type !== SecretServiceClient.Binary
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
                        text: App.secretItem.formattedBinarySecret
                        font.family: "monospace"
                        wrapMode: Text.WordWrap
                    }
                }
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
