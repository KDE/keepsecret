// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Wallets")

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
            pageStack.push(walletContentsPage);
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
                text: i18n("About kwallets")
                icon.name: "help-about"
                onTriggered: root.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/kwallets/contents/ui/About.qml")
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
        initialPage: walletListPage
        columnView.columnResizeMode: pageStack.wideMode ? Kirigami.ColumnView.DynamicColumns : Kirigami.ColumnView.SingleColumn
        columnView.onSavedStateChanged: {
            App.sidebarState = pageStack.columnView.savedState;
        }
    }
/*
   // pageStack.wideMode: pageStack.width >= pageStack.defaultColumnWidth + minimumSidebarWidth * 2
    Binding {
        target: pageStack.contentItem
        property: "columnResizeMode"
        value: pageStack.wideMode ? Kirigami.ColumnView.DynamicColumns : Kirigami.ColumnView.SingleColumn
    }
*/
    WalletListPage {
        id: walletListPage
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: maximumSidebarWidth
        onCurrentWalletChanged: {
            walletContentsPage.currentEntry = -1
            if (currentWallet.length >= 0) {
                if (pageStack.depth < 2) {
                    pageStack.push(walletContentsPage)
                }

                pageStack.currentIndex = 1
            } else if (pageStack.wideMode) {
                pageStack.currentIndex = 0
            } else {
                pageStack.pop(walletListPage)
            }
        }
    }

    WalletContentsPage {
        id: walletContentsPage
        visible: false
        Kirigami.ColumnView.fillWidth: true
        Kirigami.ColumnView.reservedSpace: walletListPage.width + (pageStack.depth === 3 ? entryPage.width : 0)
        onCurrentEntryChanged: {
            if (currentEntry > -1) {
                if (pageStack.depth < 3) {
                    pageStack.push(entryPage)
                }
                pageStack.currentIndex = 2
            } else if (pageStack.depth == 3) {
                pageStack.pop(walletContentsPage)
            }
        }
    }

    EntryPage {
        id: entryPage
        Kirigami.ColumnView.minimumWidth: minimumSidebarWidth
        Kirigami.ColumnView.maximumWidth: maximumSidebarWidth
        visible: false
        Kirigami.ColumnView.interactiveResizeEnabled: true
        Kirigami.ColumnView.fillWidth: false
    }
/*
    // TODO: a component in Kirigami to attach resize handles directly in ColumnView
    ResizeHandle {
        targetPage: walletListPage
        onRight: true
        maximumWidth: pageStack.width - pageStack.defaultColumnWidth - entryPage.width
    }
    ResizeHandle {
        visible: root.pageStack.wideMode && pageStack.depth === 3
        targetPage: entryPage
        onRight: false
        maximumWidth: pageStack.width - pageStack.defaultColumnWidth - walletListPage.width
    }*/
}
