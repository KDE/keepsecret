// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs 
import org.kde.config as Config
import org.kde.kirigami.actioncollection as AC
import org.kde.kirigami as Kirigami
import org.kde.keepsecret
import org.kde.coreaddons

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "KeepSecret")

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    Config.WindowStateSaver {
        id: windowStateSaver
        configGroupName: "MainWindow"
    }
    Actions {
        id: actions
        pageRow: root.pageStack
    }

    readonly property real minimumSidebarWidth: pageStack.defaultColumnWidth / 2
    readonly property real maximumSidebarWidth: (width - pageStack.defaultColumnWidth) / 2
    readonly property int walletCount: App.collectionsModel.count
    readonly property bool shouldHideSidebar: walletCount <= 1
    readonly property bool itemOpen: App.stateTracker.status & StateTracker.ItemReady
    property list<Item> desiredPages: []
    onDesiredPagesChanged: {
        for (let p of pageStack.items) {
            if (!desiredPages.includes(p)) {
                pageStack.removePage(p)
            }
        }
        for (let i = 0; i < desiredPages.length; ++i) {
            if (!pageStack.items.includes(desiredPages[i])) {
                pageStack.insertPage(i, desiredPages[i])
            }
        }
    }
    Item {
        id: layoutStateMachine
        states: [
            State {
                name: "multiWalletWithEntry"
                when: !shouldHideSidebar && itemOpen && collectionListLoader.item !== null
                PropertyChanges {
                    target: root
                    desiredPages: [collectionListLoader.item, collectionContentsPage, entryPage]
                }
            },
            State {
                name: "multiWallet"
                when: !shouldHideSidebar && !itemOpen && collectionListLoader.item !== null
                PropertyChanges {
                    target: root
                    desiredPages: [collectionListLoader.item, collectionContentsPage]
                }
            },
            State {
                name: "singleWalletWithEntry"
                when: shouldHideSidebar && itemOpen
                PropertyChanges {
                    target: root
                    desiredPages: [collectionContentsPage, entryPage]
                }
            },
            State {
                name: "singleWallet"
                when: shouldHideSidebar && !itemOpen
                PropertyChanges {
                    target: root
                    desiredPages: [collectionContentsPage]
                }
            }
        ]
    }

    function updateSidebarVisibility() {
        if(walletCount === 1) {
            const path = App.collectionsModel.dbusPathAt(0)
            if (path && path.length > 0) {
                App.collectionModel.collectionPath = path
            }
        }
    }

    Component.onCompleted: {
        pageStack.columnView.savedState = shouldHideSidebar ? "" : App.sidebarState
        Qt.callLater(updateSidebarVisibility)
    }

    onWalletCountChanged: {
        Qt.callLater(updateSidebarVisibility) 
    }
    
    Connections {
        target: App.collectionsModel
        function onModelReset() {
            Qt.callLater(updateSidebarVisibility)
        }
        function onRowsInserted() {
            Qt.callLater(updateSidebarVisibility)
        }
        function onRowsRemoved() {
            Qt.callLater(updateSidebarVisibility)
        }
    }
    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: !Kirigami.Settings.isMobile
        modal: false
        handleVisible: false
        drawerOpen: false                          
        actions: [
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Report Bug…")
                icon.name: "tools-report-bug"
                onTriggered: Qt.openUrlExternally("https://bugs.kde.org/enter_bug.cgi?format=guided&product=keepsecret&version="+AboutData.version)
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Donate…")
                icon.name: "help-donate-" + Qt.locale().currencySymbol(Locale.CurrencyIsoCode).toLowerCase()
                onTriggered: Qt.openUrlExternally("https://kde.org/donate/?app=keepsecret")
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "About KeepSecret")
                icon.name: "help-about"
                onTriggered: root.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/keepsecret/qml/About.qml")
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "About KDE")
                icon.name: "kde"
                onTriggered: root.pageStack.pushDialogLayer(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutKDEPage"))
            },
            Kirigami.Action {
                AC.ActionCollection.collection: "org.kde.globalactions"
                AC.ActionCollection.action: AC.StandardActionData.KeyBindings
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "New Wallet")
                icon.name: "list-add-symbolic"
                AC.ActionCollection.collection: "org.kde.keepsecret.collections"
                AC.ActionCollection.action: "new-wallet"
                onTriggered: walletCreationDialog.open()
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Export…")
                icon.name: "document-export"
                onTriggered: exportDialog.open()
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Import…")
                icon.name: "document-import"
                onTriggered: importDialog.open()
            }
        ]
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack {
        id: pageStack
        columnView.columnResizeMode: shouldHideSidebar
            ? Kirigami.ColumnView.DynamicColumns
            : (pageStack.wideMode ? Kirigami.ColumnView.DynamicColumns : Kirigami.ColumnView.SingleColumn)
        columnView.onSavedStateChanged: {
            if (!shouldHideSidebar) {
                App.sidebarState = pageStack.columnView.savedState;
            }
        }
        globalToolBar {
            style: Kirigami.ApplicationHeaderStyle.ToolBar
            showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton
        }
    }

    Kirigami.InlineMessage {
        visible: App.stateTracker.error === StateTracker.ServiceConnectionError
        parent: root.overlay
        width: parent.width
        y: root.pageStack.globalToolBar.preferredHeight
        position: Kirigami.InlineMessage.Header
        type: Kirigami.MessageType.Error
        text: visible ? App.stateTracker.errorMessage : ""
    }
    function openWalletCreationDialog() { walletCreationDialog.open() }
    function showDeleteDialog(title, message, confirmationMessage, callback) {
        deleteDialog.title = title
        deleteDialog.message = message;
        deleteDialog.confirmationMessage = confirmationMessage;
        deleteDialog.acceptedCallback = callback;
        deleteDialog.open()
    }
    QQC.Dialog {
        id: deleteDialog
        property alias message: messageLabel.text
        property alias confirmationMessage: deletionConfirmation.text
        property var acceptedCallback: () => {}
        modal: true
        standardButtons: QQC.Dialog.Save | QQC.Dialog.Cancel

        onClosed: deletionConfirmation.checked = false
        Component.onCompleted: {
            standardButton(QQC.Dialog.Save).text = i18nc("@action:button permanently delete the wallet", "Permanently Delete")
            standardButton(QQC.Dialog.Save).icon.name = "edit-delete"
            standardButton(QQC.Dialog.Save).enabled = false

            standardButton(QQC.Dialog.Cancel).text = i18nc("@action:button keep the wallet", "Keep")
            standardButton(QQC.Dialog.Cancel).icon.name = "love-symbolic"
        }

        contentItem: ColumnLayout {
            QQC.Label {
                id: messageLabel
                wrapMode: Text.WordWrap
            }
            QQC.CheckBox {
                id: deletionConfirmation
                onCheckedChanged: deleteDialog.standardButton(QQC.Dialog.Save).enabled = checked
            }
        }

        onAccepted: acceptedCallback()
    }

    QQC.Dialog {
        id: walletCreationDialog
        modal: true
        title: i18nc("@title:window", "Create New Wallet")
        standardButtons: QQC.Dialog.Save | QQC.Dialog.Cancel

        function checkSaveEnabled() {
            let button = standardButton(QQC.Dialog.Save);
            button.enabled = walletNameField.text.length > 0;
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
                text: i18nc("@label:textbox", "Wallet name:")
            }
            QQC.TextField {
                id: walletNameField
                Layout.fillWidth: true
                onVisibleChanged: {
                    if (visible) {
                        forceActiveFocus();
                    }
                }
                onTextChanged: walletCreationDialog.checkSaveEnabled()
                onAccepted: walletCreationDialog.maybeAccept()
            }
        }

        onAccepted: App.secretService.createCollection(walletNameField.text)
        onVisibleChanged: {
            if (!visible) {
                walletNameField.text = ""
            }
        }
    }
    FileDialog {
        id: exportDialog
        title: i18nc("@title:window", "Export Wallet")
        fileMode: FileDialog.SaveFile
        nameFilters: [i18nc("@label file type filter", "KWallet XML files (*.xml)"), i18nc("@label file type filter", "All files (*)")]
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
        fileMode: QQC.FileDialog.OpenFile
        nameFilters: [i18nc("@label file type filter", "KWallet XML files (*.xml)"), i18nc("@label file type filter", "All files (*)")]
        onAccepted: {
            App.importExportManager.importFromFile(
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
            errorLabel.text = message
            errorDialog.open()
        }
    }

    Connections {
        target: App.stateTracker
        function onErrorChanged(error, message) {
            if (error !== StateTracker.NoError && error != StateTracker.ServiceConnectionError) {
                errorLabel.text = message
                errorDialog.open();
            }
        }
        function onOperationsChanged(oldOp, newOp) {
            if (newOp === StateTracker.OperationNone) {
                loadingPopup.close();
            } else {
                loadingIndicatorTimer.restart();
            }
        }
    }
    QQC.Dialog {
        id: errorDialog
        modal: true
        standardButtons: QQC.Dialog.Ok
        width: Math.round(Math.min(implicitWidth, root.width * 0.8))
        title: i18nc("@title:window", "Error")
        contentItem: RowLayout {
            Kirigami.SelectableLabel {
                id: errorLabel
            }
        }
    }

    QQC.Popup {
        id: loadingPopup
        x: parent.width - width - Kirigami.Units.smallSpacing
        y: parent.height - height - Kirigami.Units.smallSpacing
        padding: Kirigami.Units.smallSpacing
        parent: root.QQC.Overlay.overlay
        modal: false
        contentItem: RowLayout {
            QQC.BusyIndicator {
                id: busyIndicator
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: implicitWidth
                running: visible
            }
            QQC.Label {
                Layout.preferredHeight: busyIndicator.implicitHeight
                text: App.stateTracker.operationsReadableName
            }
        }
        Timer {
            id: loadingIndicatorTimer
            interval: Kirigami.Units.humanMoment
            onTriggered: {
                if (App.stateTracker.operations !== StateTracker.OperationNone) {
                    loadingPopup.open()
                }
            }
        }
    }
    Component {
        id: collectionListComponent
        CollectionListPage {
            Kirigami.ColumnView.interactiveResizeEnabled: true
            Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
            Kirigami.ColumnView.maximumWidth: maximumSidebarWidth
        }
    }
    Loader {
        id: collectionListLoader
        active: true
        sourceComponent: collectionListComponent
        onLoaded: {
            Qt.callLater(() => {
                let temp = root.shouldHideSidebar
            })
        }
    }

    CollectionContentsPage {
        id: collectionContentsPage
        Kirigami.ColumnView.fillWidth: true
        Kirigami.ColumnView.reservedSpace: {
            let sidebarW = (!shouldHideSidebar && collectionListLoader.item && collectionListLoader.item.width > 0) 
            ? collectionListLoader.item.width 
            : (!shouldHideSidebar ? minimumSidebarWidth : 0)
            let entryW = itemOpen ? entryPage.Kirigami.ColumnView.preferredWidth : 0
            return sidebarW + entryW
        }
    }

    EntryPage {
        id: entryPage
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: root.width - (root.walletCount > 1 && collectionListLoader.item ? collectionListLoader.item.width : 0) - root.pageStack.defaultColumnWidth

        // An arbitrary big width by default
        Kirigami.ColumnView.preferredWidth: Kirigami.Units.gridUnit * 30
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.fillWidth: false
    }
}
