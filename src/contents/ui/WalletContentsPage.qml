// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    property alias currentEntry: view.currentIndex

    title: App.walletModel.currentWallet

    actions: [
        Kirigami.Action {
            id: newAction
            text: i18n("New Entry")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new entry in this wallet")
            enabled: App.walletModel.status === WalletModel.Ready
            onTriggered: creationDialog.open()
        },
        Kirigami.Action {
            id: searchAction
            text: i18n("Search")
            icon.name: "search-symbolic"
            tooltip: i18n("Search entries in this wallet")
            enabled: App.walletModel.status === WalletModel.Ready
            checkable: true
        },
        Kirigami.Action {
            id: lockAction
            text: App.walletModel.status === WalletModel.Locked? i18n("Unlock") : i18n("Lock")
            icon.name: App.walletModel.status === WalletModel.Locked? "unlock-symbolic" : "lock-symbolic"
            tooltip: App.walletModel.status === WalletModel.Locked? i18n("Unlock this wallet") : i18n("Lock this wallet")
            enabled: App.walletModel.status !== WalletModel.Disconnected
            onTriggered: {
                if (App.walletModel.status === WalletModel.Locked) {
                    App.walletModel.unlock()
                } else {
                    App.walletModel.lock()
                }
            }
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this wallet")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: root.counter += 1
        }
    ]

    header: QQC.ToolBar {
        visible: searchAction.checked
        contentItem: Kirigami.SearchField {
            id: searchField
            onVisibleChanged: {
                if (visible) {
                    forceActiveFocus()
                } else {
                    text = ""
                }
            }
        }
    }

    QQC.Dialog {
        id: creationDialog
        modal: true
        title: i18n("Create a new Item")
        standardButtons: QQC.Dialog.Save | QQC.Dialog.Cancel

        function checkSaveEnabled() {
            let button = standardButton(QQC.Dialog.Save);
            button.enabled = (labelField.text.length > 0 && passwordField.text.length > 0 &&
                              userField.text.length > 0 && serverField.text.length > 0);
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
                text: i18n("Label:")
            }
            QQC.TextField {
                id: labelField
                onVisibleChanged: {
                    if (visible) {
                        forceActiveFocus();
                    }
                }
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18n("Password:")
            }
            Kirigami.PasswordField {
                id: passwordField
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18n("User:")
            }
            QQC.TextField {
                id: userField
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18n("Server:")
            }
            QQC.TextField {
                id: serverField
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
        }

        onAccepted: {
            console.log("Ok clicked")
            App.secretItem.createItem(labelField.text,
                                passwordField.text,
                                userField.text,
                                serverField.text,
                                App.walletModel.currentWallet);
        }
        onVisibleChanged: {
            console.log("resetting")
            labelField.text = ""
            passwordField.text = ""
            userField.text = ""
            serverField.text = ""
        }
    }

    ListView {
        id: view
        currentIndex: -1
        model: KSortFilterProxyModel {
            sourceModel: App.walletModel
            sortRoleName: "folder"
            sortCaseSensitivity: Qt.CaseInsensitive
            filterRole: Qt.DisplayRole
            filterString: searchField.text
            filterCaseSensitivity: Qt.CaseInsensitive
        }
        onModelChanged: currentIndex = -1
        section.property: "folder"
        section.delegate: QQC.Control {
            width: view.width
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "folder"
                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                    implicitHeight: implicitWidth
                }
                QQC.Label {
                    text: section
                    Layout.fillWidth: false
                }
                Kirigami.Separator {
                    Layout.alignment: Qt.AlignCenter
                    Layout.fillWidth: true
                }
            }
        }
        section.criteria: ViewSection.FullString
        delegate: QQC.ItemDelegate {
            required property var model
            required property int index
            width: view.width
            // FIXME: this imitates an item with the space for the icon even if there is none, there should be something to do that more cleanly
            leftPadding: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.mediumSpacing * 2
            implicitHeight: Kirigami.Units.iconSizes.smallMedium + padding * 2
            text: model.display
            highlighted: view.currentIndex == index
            onClicked: {
                view.currentIndex = index
                App.secretItem.loadItem(App.walletModel.currentWallet, model.dbusPath);
                page.Kirigami.ColumnView.view.currentIndex = 2;
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: view.count === 0
            icon.name: {
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return "folder-locked"
                case WalletModel.Connected:
                    return "wallet-closed"
                default:
                    return ""
                }
            }
            text: {
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return i18n("Wallet is locked")
                case WalletModel.Connected:
                    return i18n("Wallet is empty")
                default:
                    return ""
                }
            }
            helpfulAction: {
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return lockAction
                case WalletModel.Connected:
                    return newAction
                default:
                    return null
                }
            }
        }
    }
}
