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

    title: App.walletModel.collectionName

    // FIXME: why int?
    property int status: App.walletModel.status

    actions: [
        Kirigami.Action {
            id: newAction
            text: i18n("New Entry")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new entry in this wallet")
            enabled: App.walletModel.status & WalletModel.Connected
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
            onTriggered: deletionDialog.open()
        }
    ]

    header: ColumnLayout {
        spacing: 0
        visible: searchAction.checked || App.walletModel.error !== SecretServiceClient.NoError
        QQC.ToolBar {
            Layout.fillWidth: true
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
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: App.walletModel.error !== WalletModel.NoError
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Error
            text: App.walletModel.errorMessage
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
                Layout.fillWidth: true
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
                Layout.fillWidth: true
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18n("User:")
            }
            QQC.TextField {
                id: userField
                Layout.fillWidth: true
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18n("Server:")
            }
            QQC.TextField {
                id: serverField
                Layout.fillWidth: true
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
        }

        onAccepted: {
            App.secretItem.createItem(labelField.text,
                                passwordField.text,
                                userField.text,
                                serverField.text,
                                App.walletModel.collectionPath);
        }
        onVisibleChanged: {
            labelField.text = ""
            passwordField.text = ""
            userField.text = ""
            serverField.text = ""
        }
    }

    QQC.Dialog {
        id: deletionDialog
        modal: true
        standardButtons: QQC.Dialog.Yes | QQC.Dialog.No

        Component.onCompleted: standardButton(QQC.Dialog.Yes).enabled = false

        contentItem: ColumnLayout {
            QQC.Label {
                text: i18n("Are you sure you want to delete the wallet “%1”?", App.walletModel.collectionName)
                wrapMode: Text.WordWrap
            }
            QQC.CheckBox {
                id: deletionConfirmation
                text: i18n("I understand that all the items will be permanently deleted")
                onCheckedChanged: deletionDialog.standardButton(QQC.Dialog.Yes).enabled = checked
            }
        }

        onAccepted: App.secretService.deleteCollection(App.walletModel.collectionPath)
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
                    source: section
                    fallback: "folder"
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
                App.secretItem.loadItem(App.walletModel.collectionPath, model.dbusPath);
                page.Kirigami.ColumnView.view.currentIndex = 2;
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: view.count === 0 &&
                    (App.walletModel.status === WalletModel.Ready ||
                     App.walletModel.status === WalletModel.Locked ||
                     App.secretService.error === SecretServiceClient.ConnectionFailed)
            icon.name: {
                if (App.secretService.status === SecretServiceClient.Disconnected) {
                    return "action-unavailable-symbolic";
                }
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return "object-locked";
                default:
                    return "wallet-closed";
                }
            }
            text: {
                if (App.secretService.status === SecretServiceClient.Disconnected) {
                    return "";
                }
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return i18n("Wallet is locked")
                case WalletModel.Ready:
                    return i18n("Wallet is empty")
                default:
                    return i18n("Select a wallet to open from the list")
                }
            }
            helpfulAction: {
                switch (App.walletModel.status) {
                case WalletModel.Locked:
                    return lockAction
                case WalletModel.Ready:
                    return newAction
                default:
                    return null
                }
            }
        }
    }
}
