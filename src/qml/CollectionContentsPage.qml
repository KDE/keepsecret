// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KAC
import org.kde.kitemmodels
import org.kde.keepsecret

Kirigami.ScrollablePage {
    id: page

    property alias currentEntry: view.currentIndex

    title: App.collectionModel.collectionName

    // FIXME: why int?
    property int status: App.stateTracker.status

    onFocusChanged: {
        if (focus) {
            view.forceActiveFocus();
        }
    }

    actions: [
        Kirigami.Action {
            id: newAction
            text: i18nc("@action:button Create a new secret", "New Entry")
            icon.name: "list-add-symbolic"
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            onTriggered: creationDialog.open()
        },
        Kirigami.Action {
            id: searchAction
            text: i18nc("@action:button", "Search")
            icon.name: "search-symbolic"
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            shortcut: checked ? "" : "Ctrl+F"
            checkable: true
            onTriggered: {
                if (checked) {
                    searchField.forceActiveFocus()
                }
            }
        },
        Kirigami.Action {
            id: lockAction
            readonly property bool locked: App.stateTracker.status & StateTracker.CollectionLocked
            text: locked
                ? i18nc("@action:inmenu unlock this wallet", "Unlock")
                : i18nc("@action:inmenu lock this wallet", "Lock")
            icon.name: locked ? "unlock-symbolic" : "lock-symbolic"
            enabled: App.stateTracker.status & (StateTracker.CollectionReady | StateTracker.CollectionLocked)
            onTriggered: {
                if (locked) {
                    App.collectionModel.unlock()
                } else {
                    App.collectionModel.lock()
                }
            }
        },
        Kirigami.Action {
            text: i18nc("@title:window Delete this wallet", "Delete Wallet")
            icon.name: "delete-symbolic"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                showDeleteDialog(
                    i18nc("@title:window", "Delete Wallet"),
                    i18nc("@label", "Are you sure you want to delete the wallet “%1”?", App.collectionModel.collectionName),
                    i18nc("@action:check", "I understand that all the items will be permanently deleted"),
                    () => {
                        App.secretService.deleteCollection(App.collectionModel.collectionPath)
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
        title: i18nc("@title:window", "Create New Entry")
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
                text: i18nc("@label:textbox name of this secret", "Label:")
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
                text: i18nc("@label:textbox password for this secret", "Password:")
            }
            Kirigami.PasswordField {
                id: passwordField
                Layout.fillWidth: true
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18nc("@label:textbox user of this secret", "User:")
            }
            QQC.TextField {
                id: userField
                Layout.fillWidth: true
                onTextChanged: creationDialog.checkSaveEnabled()
                onAccepted: creationDialog.maybeAccept()
            }
            QQC.Label {
                text: i18nc("@label:textbox server/provider of this secret", "Server:")
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
                                App.collectionModel.collectionPath);
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
            App.secretItemForContextMenu.loadItem(App.collectionModel.collectionPath, contextMenu.model.dbusPath);
        }
        QQC.MenuItem {
            text: i18nc("@action:inmenu Copy this secret", "Copy Secret")
            icon.name: "edit-copy-symbolic"
            enabled: App.secretItemForContextMenu.status !== SecretItemProxy.Locked
            onClicked: App.secretItemForContextMenu.copySecret()
        }
        QQC.MenuItem {
            text: i18nc("@action:inmenu Delete this secret", "Delete")
            icon.name: "usermenu-delete-symbolic"
            onClicked: {
                showDeleteDialog(
                    i18nc("@title:window", "Delete Secret"),
                    i18nc("@label", "Are you sure you want to delete the item “%1”?", App.secretItemForContextMenu.label),
                    i18nc("@action:check", "I understand that the item will be permanently deleted"),
                    () => {
                        App.secretItemForContextMenu.deleteItem()
                    })
            }
        }
        QQC.MenuSeparator {}
        QQC.MenuItem {
            text: i18nc("@action:inmenu Show properties", "Properties")
            icon.name: "configure-symbolic"
            onClicked: {
                view.currentIndex = contextMenu.model.index
                App.secretItem.loadItem(App.collectionModel.collectionPath, contextMenu.model.dbusPath);
                Kirigami.PageStack.pageStack.currentIndex = 2;
            }
        }
    }

    ListView {
        id: view
        currentIndex: -1
        keyNavigationEnabled: true
        activeFocusOnTab: true
        model: KSortFilterProxyModel {
            sourceModel: App.collectionModel
            sortRoleName: "folder"
            sortCaseSensitivity: Qt.CaseInsensitive
            filterRole: Qt.DisplayRole
            filterString: searchField.text
            filterCaseSensitivity: Qt.CaseInsensitive
        }
        onModelChanged: currentIndex = -1
        section.property: "folder"
        section.delegate: Kirigami.ListSectionHeader {
            width: view.width
            text: section
            icon.name: "folder"
        }

        section.criteria: ViewSection.FullString
        delegate: QQC.ItemDelegate {
            id: delegate
            required property var model
            required property int index
            width: view.width
            // FIXME: this imitates an item with the space for the icon even if there is none, there should be something to do that more cleanly
            leftPadding: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.largeSpacing * 2
            implicitHeight: Kirigami.Units.iconSizes.smallMedium + padding * 2
            text: model.display
            highlighted: view.currentIndex == index

            function click() {
                if (contextMenu.visible) {
                    return;
                }
                view.currentIndex = index
                App.secretItem.loadItem(App.collectionModel.collectionPath, model.dbusPath);
                if (!Kirigami.PageStack.pageStack.wideMode) {
                    Kirigami.PageStack.pageStack.currentIndex = 2;
                } else {
                    view.forceActiveFocus();
                }
            }
            onClicked: click()
            Keys.onPressed: (event) => {
                if (contextMenu.visible) {
                    return;
                }
                if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                    delegate.click();
                }
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
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            opacity: view.count === 0 &&
                !(App.stateTracker.operations & StateTracker.ServiceConnecting) &&
                !(App.stateTracker.operations & StateTracker.ServiceLoadingCollections)
            icon.name: {
                if (App.stateTracker.status & StateTracker.ServiceDisconnected) {
                    return "action-unavailable-symbolic";
                } else if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return "object-locked";
                } else if (searchField.text.length > 0) {
                    return "search-symbolic";
                } else {
                    return "wallet-closed";
                }
            }
            text: {
                if (App.stateTracker.status & StateTracker.ServiceDisconnected) {
                    return "";
                } else if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return i18nc("@info:status", "Wallet is locked");
                } else if (searchField.text.length > 0) {
                    return i18nc("@info:status", "No search results");
                } else if (App.stateTracker.status & StateTracker.CollectionReady) {
                    return i18nc("@info:status", "Wallet is empty");
                } else {
                    return i18nc("@info:status", "Select a wallet to open from the list");
                }
            }
            helpfulAction: {
                if (App.stateTracker.status & StateTracker.CollectionLocked) {
                    return lockAction;
                } else if (searchField.text.length > 0) {
                    return null;
                } else if (App.stateTracker.status & StateTracker.CollectionReady) {
                    return newAction;
                } else {
                    return null;
                }
            }
        }
        Image {
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            z: -1
            width: Math.round(Math.min(parent.width, parent.height) * 0.8)
            height: width
            sourceSize.width: width
            sourceSize.height: height
            source: "qrc:/watermark.svg"
        }
    }
    KAC.FloatingButton {
        parent: page
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        margins: Kirigami.Units.gridUnit
        visible: Kirigami.Settings.isMobile && App.stateTracker.status & StateTracker.CollectionReady

        action: newAction
    }
}
