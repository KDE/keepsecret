// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KAC
import org.kde.keepsecret

Kirigami.ScrollablePage {
    id: page

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    title: QQC.ApplicationWindow.window.pageStack.wideMode ? "" : i18n("Wallets")

    readonly property string collectionPath: App.collectionModel.collectionPath

    actions: [
        Kirigami.Action {
            id: createAction
            text: i18n("New Wallet")
            icon.name: "list-add-symbolic"
            onTriggered: creationDialog.open()
        }
    ]

    QQC.Dialog {
        id: creationDialog
        modal: true
        title: i18n("Create a new Wallet")
        standardButtons: QQC.Dialog.Save | QQC.Dialog.Cancel

        function checkSaveEnabled() {
            let button = standardButton(QQC.Dialog.Save);
            button.enabled = collectionNameField.text.length > 0;
        }

        function maybeAccept() {
            let button = standardButton(QQC.Dialog.Save);
            if (button.enabled) {
                accept();
            }
        }

        Component.onCompleted: standardButton(QQC.Dialog.Save).enabled = false

        contentItem: ColumnLayout {
            QQC.Label {
                text: i18n("Wallet name:")
            }
            QQC.TextField {
                id: collectionNameField
                Layout.fillWidth: true
                onVisibleChanged: {
                    if (visible) {
                        forceActiveFocus();
                    }
                }
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
        }

        onAccepted: App.secretService.createCollection(collectionNameField.text)
        onVisibleChanged: {
            console.log("resetting")
            collectionNameField.text = ""
        }
    }

    QQC.Menu {
        id: contextMenu
        property var model: {
            "display": "",
            "dbusPath": "",
            "locked": false
        }
        QQC.MenuItem {
            text: i18n("Set as Default")
            enabled: App.secretService.defaultCollection !== contextMenu.model.dbusPath
            onClicked: App.secretService.defaultCollection = contextMenu.model.dbusPath
        }
        QQC.MenuItem {
            text: contextMenu.model.locked ? i18n("Unlock") : i18n("Lock")
            icon.name: contextMenu.model.locked ? "unlock-symbolic" : "lock-symbolic"
            onClicked: {
                if (contextMenu.model.locked) {
                    App.secretService.unlockCollection(contextMenu.model.dbusPath)
                } else {
                    App.secretService.lockCollection(contextMenu.model.dbusPath)
                }
            }
        }
        QQC.MenuItem {
            text: i18n("Delete")
            icon.name: "usermenu-delete-symbolic"
            onClicked: {
                showDeleteDialog(
                    i18n("Are you sure you want to delete the wallet “%1”?", contextMenu.model.display),
                    i18n("I understand that all the items will be permanently deleted"),
                    () => {
                        App.secretService.deleteCollection(contextMenu.model.dbusPath)
                    });
            }
        }
    }

    ListView {
        id: view
        currentIndex: App.collectionsModel.currentIndex
        model: App.collectionsModel
        delegate: QQC.ItemDelegate {
            id: delegate
            required property var model
            required property int index
            width: view.width
            icon.name: highlighted && !App.collectionModel.locked ? "wallet-open" : "wallet-closed"
            text: model.display
            highlighted: view.currentIndex == index
            font.bold: App.secretService.defaultCollection === model.dbusPath

            onClicked: {
                if (contextMenu.visible) {
                    return;
                }
                App.collectionModel.collectionPath = model.dbusPath
                page.Kirigami.ColumnView.view.currentIndex = 1
            }

            TapHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                acceptedButtons: Qt.RightButton
                onPressedChanged: {
                    if (pressed) {
                        contextMenu.model = model
                        contextMenu.popup(delegate)
                    }
                }
            }
            TapHandler {
                acceptedDevices: PointerDevice.TouchScreen
                onLongPressed: {
                    contextMenu.model = model
                    contextMenu.popup(delegate)
                }
            }

            Kirigami.Icon {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    margins: Kirigami.Units.mediumSpacing
                }
                color: delegate.highlighted || delegate.down
                        ? Kirigami.Theme.highlightedTextColor
                        : (delegate.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor)
                width: Kirigami.Units.iconSizes.small
                visible: model.locked
                source: "object-locked-symbolic"
                TapHandler {
                    onTapped: App.secretService.unlockCollection(model.dbusPath)
                }
            }
        }
        Image {
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            visible: !QQC.ApplicationWindow.window.pageStack.wideMode
            width: Math.round(Math.min(parent.width, parent.height) * 0.8)
            height: width
            sourceSize.width: width
            sourceSize.height: height
            source: visible ? "qrc:/watermark.svg" : ""
        }
    }
    KAC.FloatingButton {
        parent: page
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        margins: Kirigami.Units.gridUnit
        visible: Kirigami.Settings.isMobile

        action: createAction
    }
}
