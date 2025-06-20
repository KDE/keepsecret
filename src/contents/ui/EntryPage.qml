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



    component MapNewField: RowLayout {
        id: tableRow
        property alias key: keyField.text
        property alias value: valueField.text
        visible: showMapValuesCheck.checked
        QQC.TextField {
            id: keyField
            Layout.fillWidth: true
            onTextEdited: {
                let index = tableRow.parent.dynamicFields.indexOf(tableRow);
                if (text.length > 0 && index >= 0 && index == tableRow.parent.dynamicFields.length - 1) {
                    print(mapNewFieldComponent)
                    let field = mapNewFieldComponent.createObject(tableRow.parent)
                    tableRow.parent.dynamicFields.push(field)
                    tableRow.parent.dynamicFieldsChanged()
                }
                saveJsonTimer.restart();
            }
        }
        QQC.TextField {
            id: valueField
            visible: showMapValuesCheck.checked
            Layout.fillWidth: true
            onTextEdited: saveJsonTimer.restart()
        }
        QQC.Button {
            enabled: tableRow.parent.dynamicFields.indexOf(tableRow) < tableRow.parent.dynamicFields.length - 1
            icon.name: "delete-symbolic"
            QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC.ToolTip.visible: hovered
            QQC.ToolTip.text: i18n("Remove Row")
            onClicked: {
                tableRow.destroy();
                saveJsonTimer.restart();
            }
        }
        Component.onDestruction: {
            let index = tableRow.parent.dynamicFields.indexOf(tableRow);
            if (index < 0) {
                return;
            }
            tableRow.parent.dynamicFields.splice(index, 1);
            tableRow.parent.dynamicFieldsChanged()
        }
    }

    property Component mapNewFieldComponent: Component { MapNewField {} }

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
            FormItem {
                visible: App.secretItem.type === SecretServiceClient.Map
                contentItem: ColumnLayout {
                    id: mapFormItem
                    property var secretValueMap: {}
                    property var secretValueMapKeys: []
                    property var dynamicFields: []

                    function generateJsonMap() {
                        let json = {}
                        children.forEach((child) => {
                            if (child instanceof RowLayout && child.visible === true && child.key.length > 0) {
                                json[child.key] = child.value
                            }
                        });
                        return JSON.stringify(json);
                    }

                    function reload() {
                        for (let i in dynamicFields) {
                            if (typeof dynamicFields[i] === "undefined") {
                                continue;
                            }
                            if (i == 0) {
                                dynamicFields[0].key = "";
                                dynamicFields[0].value = "";
                                continue;
                            }
                            dynamicFields[i].destroy();
                        }
                        dynamicFields = [dynamicFields[0]];

                        secretValueMap = JSON.parse(App.secretItem.secretValue);
                    }

                    Component.onCompleted: reload()

                    Connections {
                        target: App.secretItem
                        function onItemLoaded() {
                            mapFormItem.reload();
                        }
                        function onItemSaved() {
                            mapFormItem.reload();
                        }
                    }

                    QQC.CheckBox {
                        id: showMapValuesCheck
                        Layout.fillWidth: true
                        text: i18n("Show Secret Values")
                    }
                    Timer {
                        id: saveJsonTimer
                        interval: 400
                        onTriggered: App.secretItem.secretValue = mapFormItem.generateJsonMap();
                    }
                    Repeater {
                        model: Object.keys(mapFormItem.secretValueMap)
                        RowLayout {
                            id: tableRow
                            property alias key: keyField.text
                            property alias value: valueField.text
                            QQC.TextField {
                                id: keyField
                                Layout.fillWidth: true
                                text: modelData
                                onTextEdited: saveJsonTimer.restart()
                            }
                            QQC.TextField {
                                id: valueField
                                visible: showMapValuesCheck.checked
                                Layout.fillWidth: true
                                text: mapFormItem.secretValueMap[modelData]
                                onTextEdited: saveJsonTimer.restart()
                            }
                            QQC.Button {
                                icon.name: "delete-symbolic"
                                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                                QQC.ToolTip.visible: hovered
                                QQC.ToolTip.text: i18n("Remove Row")
                                onClicked: {
                                    tableRow.visible = false;
                                    saveJsonTimer.restart();
                                }
                            }
                        }
                    }
                    MapNewField {
                        Component.onCompleted: firstDynamicField.parent.dynamicFields.push(this)
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
