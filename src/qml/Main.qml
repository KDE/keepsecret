// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.keepsecret

Kirigami.ApplicationWindow {
    id: root

    title: i18n("KeepSecret")

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    onClosing: App.saveWindowGeometry(root)

    onWidthChanged: saveWindowGeometryTimer.restart()
    onHeightChanged: saveWindowGeometryTimer.restart()
    onXChanged: saveWindowGeometryTimer.restart()
    onYChanged: saveWindowGeometryTimer.restart()

    readonly property real minimumSidebarWidth: pageStack.defaultColumnWidth / 2
    readonly property real maximumSidebarWidth: (width - pageStack.defaultColumnWidth) / 2

    Component.onCompleted: {
        App.restoreWindowGeometry(root);
        if (width >= pageStack.defaultColumnWidth * 2 ) {
            pageStack.push(collectionContentsPage);
        }
        pageStack.columnView.savedState = App.sidebarState;
    }

    // This timer allows to batch update the window size change to reduce
    // the io load and also work around the fact that x/y/width/height are
    // changed when loading the page and overwrite the saved geometry from
    // the previous session.
    Timer {
        id: saveWindowGeometryTimer
        interval: 1000
        onTriggered: App.saveWindowGeometry(root)
    }

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: !Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: i18n("About KeepSecret")
                icon.name: "help-about"
                onTriggered: root.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/keepsecret/qml/About.qml")
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack {
        initialPage: collectionListPage
        columnView.columnResizeMode: pageStack.wideMode ? Kirigami.ColumnView.DynamicColumns : Kirigami.ColumnView.SingleColumn
        columnView.onSavedStateChanged: {
            App.sidebarState = pageStack.columnView.savedState;
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

    function showDeleteDialog(message, confirmationMessage, callback) {
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
        standardButtons: QQC.Dialog.Yes | QQC.Dialog.No

        onClosed: deletionConfirmation.checked = false
        Component.onCompleted: standardButton(QQC.Dialog.Yes).enabled = false

        contentItem: ColumnLayout {
            QQC.Label {
                id: messageLabel
                wrapMode: Text.WordWrap
            }
            QQC.CheckBox {
                id: deletionConfirmation
                onCheckedChanged: deleteDialog.standardButton(QQC.Dialog.Yes).enabled = checked
            }
        }

        onAccepted: acceptedCallback()
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
        title: i18n("Error")
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

    CollectionListPage {
        id: collectionListPage
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: maximumSidebarWidth
        onCollectionPathChanged: {
            collectionContentsPage.currentEntry = -1
            if (collectionPath.length >= 0) {
                if (pageStack.depth < 2) {
                    pageStack.push(collectionContentsPage)
                }

                pageStack.currentIndex = 1
            } else if (pageStack.wideMode) {
                pageStack.currentIndex = 0
            } else {
                pageStack.pop(collectionListPage)
            }
        }
    }

    CollectionContentsPage {
        id: collectionContentsPage
        visible: false
        Kirigami.ColumnView.fillWidth: true
        Kirigami.ColumnView.reservedSpace: collectionListPage.width + (pageStack.depth === 3 ? entryPage.width : 0)
        onCurrentEntryChanged: {
            if (currentEntry > -1) {
                if (pageStack.depth < 3) {
                    pageStack.push(entryPage)
                }
                pageStack.currentIndex = 2
            } else if (pageStack.depth == 3) {
                pageStack.pop(collectionContentsPage)
            }
        }
        onStatusChanged: {
            if (!(status & StateTracker.CollectionReady)) {
                pageStack.pop(collectionContentsPage)
            }
        }
    }

    EntryPage {
        id: entryPage
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: root.width - collectionListPage.width - root.pageStack.defaultColumnWidth
        // An arbitrary big width by default
        Kirigami.ColumnView.preferredWidth: Kirigami.Units.gridUnit * 30
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.fillWidth: false
        visible: false
    }
}
