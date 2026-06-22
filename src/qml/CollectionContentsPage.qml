// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KAC
import org.kde.kirigami.actioncollection as AC
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
            id: newWalletAction
            visible: page.Window.window.shouldHideSidebar
            AC.ActionCollection.collection: "org.kde.keepsecret.collections"
            AC.ActionCollection.action: "new-wallet"
            onTriggered: page.Window.window.openWalletCreationDialog()
        },

        Kirigami.Action {
            id: newAction
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            AC.ActionCollection.collection: "org.kde.keepsecret.collection"
            AC.ActionCollection.action: "new-entry"
            onTriggered: creationDialog.open()
        },
        Kirigami.Action {
            id: searchAction
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            shortcut: checked ? "" : "Ctrl+F"
            checkable: true
            AC.ActionCollection.collection: "org.kde.keepsecret.collection"
            AC.ActionCollection.action: "search"
            onTriggered: {
                if (checked) {
                    searchField.forceActiveFocus()
                }
            }
        },
        Kirigami.Action {
            id: lockAction
            readonly property bool locked: App.stateTracker.status & StateTracker.CollectionLocked
            enabled: App.stateTracker.status & (StateTracker.CollectionReady | StateTracker.CollectionLocked)
            AC.ActionCollection.collection: "org.kde.keepsecret.collection"
            AC.ActionCollection.action: locked ? "unlock" : "lock"
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
            AC.ActionCollection.collection: "org.kde.keepsecret.collection"
            AC.ActionCollection.action: "delete-wallet"
            onTriggered: {
                showDeleteDialog(
                    i18nc("@title:window", "Delete Wallet"),
                    i18nc("@label", "Are you sure you want to delete the wallet “%1”?", App.collectionModel.collectionName),
                    i18nc("@action:check", "I understand that all the items will be permanently deleted"),
                    () => {
                        App.secretService.deleteCollection(App.collectionModel.collectionPath)
                    });
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: App.stateTracker.status & StateTracker.CollectionReady
            AC.ActionCollection.collection: "org.kde.keepsecret.collection"
            AC.ActionCollection.action: "export-wallet"
            onTriggered: exportDialog.open()
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Import")
            icon.name: "document-import"
            displayHint: Kirigami.DisplayHint.AlwaysHide

            Kirigami.Action {
                AC.ActionCollection.collection: "org.kde.keepsecret.collection"
                AC.ActionCollection.action: "import-keepsecret"
                onTriggered: importDialog.open()
            }
            Kirigami.Action {
                AC.ActionCollection.collection: "org.kde.keepsecret.collection"
                AC.ActionCollection.action: "import-kwallet-xml"
                onTriggered: importKWalletDialog.open()
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
                App.secretItem.loadItem(
                    App.collectionModel.collectionPath,
                    contextMenu.model.dbusPath
                );
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
            text: model.display
            highlighted: view.currentIndex == index
 
            function click() {
                if (contextMenu.visible) {
                    return;
                }
                view.currentIndex = index
                App.secretItem.loadItem(App.collectionModel.collectionPath, model.dbusPath);
                view.forceActiveFocus();
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
    FileDialog {
        id: exportDialog
        title: i18nc("@title:window", "Export Wallet")
        fileMode: FileDialog.SaveFile
        nameFilters: [i18nc("@label file type filter", "KeepSecret files (*.keepsecret)"), i18nc("@label file type filter", "All files (*)")]
        onAccepted: {
            App.importExportManager.exportToFile(
                selectedFile.toString().replace("file://", ""),
                App.collectionModel.collectionName,
                App.collectionModel.exportItems()
            )
        }
    }

    FileDialog {
        id: importDialog
        title: i18nc("@title:window", "Import Wallet")
        fileMode: FileDialog.OpenFile
        nameFilters: [i18nc("@label file type filter", "KeepSecret files (*.keepsecret)"), i18nc("@label file type filter", "All files (*)")]
        onAccepted: {
            App.importExportManager.importFromFile(
                selectedFile.toString().replace("file://", "")
            )
        }
    }

    FileDialog {
        id: importKWalletDialog
        title: i18nc("@title:window", "Import KWallet XML")
        fileMode: FileDialog.OpenFile
        nameFilters: [i18nc("@label file type filter", "KWallet XML files (*.xml)"), i18nc("@label file type filter", "All files (*)")]
        onAccepted: {
            App.importExportManager.importFromKWalletXml(
                selectedFile.toString().replace("file://", "")
            )
        }
    }

    Connections {
        target: App.importExportManager
        function onImportSucceeded(items) {
            for (let i = 0; i < items.length; i++) {
                let item = items[i]
                let label = item["label"] || ""
                let secret = item["secret"] || ""
                let attrs = item["attributes"] || {}
                let server = attrs["server"] || ""
                let user = attrs["user"] || ""
                App.secretItem.createItem(
                    label,
                    secret,
                    user,
                    server,
                    App.collectionModel.collectionPath
                )
            }
        }
        function onExportSucceeded(filePath) {
            console.log("Export succeeded:", filePath)
        }
        function onErrorOccurred(message) {
            page.Window.window.showErrorDialog(message)
        }
    }
}
