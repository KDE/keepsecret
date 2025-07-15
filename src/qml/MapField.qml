// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kirigami as Kirigami
import org.kde.keepsecret


FormItem {


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

    contentItem: ColumnLayout {
        id: fieldContentsRoot
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
            if (App.secretItem.type !== SecretServiceClient.Map) {
                return;
            }
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
                fieldContentsRoot.reload();
            }
            function onItemSaved() {
                fieldContentsRoot.reload();
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
            onTriggered: App.secretItem.secretValue = fieldContentsRoot.generateJsonMap();
        }
        Repeater {
            model: fieldContentsRoot.secretValueMap ? Object.keys(fieldContentsRoot.secretValueMap) : 0
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
                    text: fieldContentsRoot.secretValueMap[modelData]
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
            Component.onCompleted: fieldContentsRoot.dynamicFields.push(this)
        }
    }
}
