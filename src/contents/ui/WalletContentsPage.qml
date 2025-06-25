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
    property int status: App.stateTracker.status

    actions: [
        Kirigami.Action {
            id: newAction
            text: i18n("New Entry")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new entry in this wallet")
            enabled: App.stateTracker.status & StateTracker.ServiceConnected
            onTriggered: creationDialog.open()
        },
        Kirigami.Action {
            id: searchAction
            text: i18n("Search")
            icon.name: "search-symbolic"
            tooltip: i18n("Search entries in this wallet")
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            checkable: true
        },
        Kirigami.Action {
            id: lockAction
            readonly property bool locked: App.stateTracker.status & StateTracker.CollectionLocked
            text: locked ? i18n("Unlock") : i18n("Lock")
            icon.name: locked ? "unlock-symbolic" : "lock-symbolic"
            tooltip: locked ? i18n("Unlock this wallet") : i18n("Lock this wallet")
            enabled: App.stateTracker.status !== StateTracker.ServiceDisconnected
            onTriggered: {
                if (locked) {
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
            onTriggered: {
                showDeleteDialog(
                    i18n("Are you sure you want to delete the wallet “%1”?", App.walletModel.collectionName),
                    i18n("I understand that all the items will be permanently deleted"),
                    () => {
                        App.secretService.deleteCollection(App.walletModel.collectionPath)
                    });
            }
        }
    ]

    header: Item {
        id: searchBarContainer
        visible: height > 0
        Layout.fillWidth: true
        implicitHeight: searchAction.checked ? searchBar.implicitHeight : 0
        Behavior on implicitHeight {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        QQC.ToolBar {
            id: searchBar
            anchors {
                left: parent.left
                right:parent.right
                bottom: parent.bottom
            }
            contentItem: Kirigami.SearchField {
                id: searchField
                onVisibleChanged: {
                    if (visible) {
                        forceActiveFocus()
                    } else {
                        text = ""
                    }
                }
                Keys.onEscapePressed: {
                    searchAction.checked = false;
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

    QQC.Menu {
        id: contextMenu
        property var model: {
            "index": -1,
            "display": "",
            "dbusPath": "",
            "folder": ""
        }
        onOpened: {
            App.secretItemForContextMenu.loadItem(App.walletModel.collectionPath, contextMenu.model.dbusPath);
        }
        QQC.MenuItem {
            text: i18n("Copy Secret")
            icon.name: "edit-copy-symbolic"
            enabled: App.secretItemForContextMenu.status !== SecretItemProxy.Locked
            onClicked: App.secretItemForContextMenu.copySecret()
        }
        QQC.MenuItem {
            text: i18n("Delete")
            icon.name: "usermenu-delete-symbolic"
            onClicked: {
                showDeleteDialog(
                    i18n("Are you sure you want to delete the item “%1”?", App.secretItemForContextMenu.label),
                         i18n("I understand that the item will be permanently deleted"),
                         () => {
                             App.secretItemForContextMenu.deleteItem()
                         })
            }
        }
        QQC.MenuSeparator {}
        QQC.MenuItem {
            text: i18n("Properties")
            icon.name: "configure-symbolic"
            onClicked: {
                view.currentIndex = contextMenu.model.index
                App.secretItem.loadItem(App.walletModel.collectionPath, contextMenu.model.dbusPath);
                page.Kirigami.ColumnView.view.currentIndex = 2;
            }
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
            id: delegate
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

            TapHandler {
                acceptedButtons: Qt.RightButton
                onPressedChanged: {
                    if (pressed) {
                        contextMenu.model = model
                        contextMenu.popup(delegate)
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: view.count === 0 &&
                    (App.stateTracker.status & StateTracker.CollectionReady ||
                     App.stateTracker.status & StateTracker.CollectionLocked ||
                     App.stateTracker.error === StateTracker.ServiceConnectionError)
            icon.name: {
                if (App.stateTracker.status & StateTracker.ServiceDisconnected) {
                    return "action-unavailable-symbolic";
                } else if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return "object-locked";
                } else {
                    return "wallet-closed";
                }
            }
            text: {
                if (App.stateTracker.status & StateTracker.ServiceDisconnected) {
                    return "";
                } else if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return i18n("Wallet is locked");
                } else if (App.stateTracker.status & StateTracker.CollectionReady) {
                    return i18n("Wallet is empty");
                } else {
                    return i18n("Select a wallet to open from the list");
                }
            }
            helpfulAction: {
                if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return lockAction;
                } else if (App.stateTracker.status &  StateTracker.CollectionReady) {
                    return newAction;
                } else {
                    return null;
                }
            }
        }
    }
}
