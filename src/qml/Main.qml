// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Window
import org.kde.config as Config
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

    readonly property real minimumSidebarWidth: pageStack.defaultColumnWidth / 2
    readonly property real maximumSidebarWidth: (width - pageStack.defaultColumnWidth) / 2

    Component.onCompleted: {
        if (width >= pageStack.defaultColumnWidth * 2 ) {
            pageStack.insertPage(1, collectionContentsPage);
        }
        pageStack.columnView.savedState = App.sidebarState;
        collectionListPage.forceActiveFocus();
    }

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: !Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Report Bug...")
                icon.name: "tools-report-bug"
                onTriggered: Qt.openUrlExternally("https://bugs.kde.org/enter_bug.cgi?format=guided&product=keepsecret&version="+AboutData.version)
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18nc("@action:inMenu", "Donate...")
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

    CollectionListPage {
        id: collectionListPage
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: maximumSidebarWidth
        onCollectionPathChanged: {
            collectionContentsPage.currentEntry = -1
            if (collectionPath.length >= 0) {
                if (pageStack.depth < 2) {
                    pageStack.insertPage(1, collectionContentsPage)
                }

                if (!pageStack.wideMode) {
                    pageStack.currentIndex = 1;
                }
            } else if (pageStack.wideMode) {
                pageStack.currentIndex = 0;
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
                if (!pageStack.wideMode) {
                    pageStack.currentIndex = 2;
                }
            } else if (pageStack.depth == 3 && pageStack.depth > 1) {
                pageStack.pop(collectionContentsPage)
            }
        }
        onStatusChanged: {
            if (!(status & StateTracker.CollectionReady) && pageStack.depth > 1) {
                pageStack.pop(collectionContentsPage)
            } else if (App.stateTracker.status & StateTracker.ItemReady) {
                if (pageStack.depth < 3) {
                    pageStack.insertPage(2, entryPage);
                    if (pageStack.wideMode) {
                        collectionContentsPage.forceActiveFocus();
                    } else {
                        pageStack.currentIndex = 2;
                    }
                }
            } else if (!pageStack.wideMode && pageStack.depth > 1) {
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
