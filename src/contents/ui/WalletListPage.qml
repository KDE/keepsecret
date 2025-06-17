// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    title: Kirigami.Settings.isMobile ? i18n("Wallets") : ""

    readonly property string currentWallet: App.walletModel.currentWallet

    actions: [
        Kirigami.Action {
            text: i18n("New Wallet")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new wallet")
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

    QQC.Dialog {
        id: deletionDialog
        modal: true
        standardButtons: QQC.Dialog.Yes | QQC.Dialog.No
        property string collection

        Component.onCompleted: standardButton(QQC.Dialog.Yes).enabled = false

        contentItem: ColumnLayout {
            QQC.Label {
                text: i18n("Are you sure you want to delete the wallet “%1”?", deletionDialog.collection)
                wrapMode: Text.WordWrap
            }
            QQC.CheckBox {
                id: deletionConfirmation
                text: i18n("I understand that all the items will be permanently deleted")
                onCheckedChanged: deletionDialog.standardButton(QQC.Dialog.Yes).enabled = checked
            }
        }

        onAccepted: App.secretService.deleteCollection(deletionDialog.collection)
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
            enabled: App.secretService.defaultCollection !== contextMenu.model.display
            onClicked: App.secretService.defaultCollection = contextMenu.model.display
        }
        QQC.MenuItem {
            text: contextMenu.model.locked ? i18n("Unlock") : i18n("Lock")
            icon.name: contextMenu.model.locked ? "unlock-symbolic" : "lock-symbolic"
            onClicked: {
                if (contextMenu.model.locked) {
                    App.secretService.unlockCollection(contextMenu.model.display)
                } else {
                    App.secretService.lockCollection(contextMenu.model.display)
                }
            }
        }
        QQC.MenuItem {
            text: i18n("Delete")
            icon.name: "usermenu-delete-symbolic"
            onClicked: {
                deletionDialog.collection = contextMenu.model.display;
                deletionDialog.open();
            }
        }
    }

    ListView {
        id: view
        currentIndex: App.walletsModel.currentIndex
        model: App.walletsModel
        delegate: QQC.ItemDelegate {
            id: delegate
            required property var model
            required property int index
            readonly property bool current: App.walletModel.currentWallet === model.display
            width: view.width
            icon.name: highlighted && !App.walletModel.locked ? "wallet-open" : "wallet-closed"
            text: model.display
            highlighted: view.currentIndex == index
            font.bold: App.secretService.defaultCollection === model.display

            onClicked: {
                App.walletModel.currentWallet = model.display
                page.Kirigami.ColumnView.view.currentIndex = 1
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onPressedChanged: {
                    if (pressed) {
                        contextMenu.model = model
                        contextMenu.popup(delegate)
                    }
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
                    onTapped: App.secretService.unlockCollection(model.display)
                }
            }
        }
    }
}
