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

    title: App.walletModel.currentWallet

    property alias currentEntry: view.currentIndex
    property int state: {
        if (App.walletModel.currentWallet.length === 0) {
            return WalletContentsPage.NoWallet
        } else if (App.walletModel.locked) {
            return WalletContentsPage.Locked
        } else if (view.count === 0) {
            return WalletContentsPage.Empty
        }
        return WalletContentsPage.Loaded
    }

    enum State {
        NoWallet,
        Locked,
        Empty,
        Loaded
    }

    actions: [
        Kirigami.Action {
            id: newAction
            text: i18n("New Entry")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new entry in this wallet")
        },
        Kirigami.Action {
            id: searchAction
            text: i18n("Search")
            icon.name: "search-symbolic"
            tooltip: i18n("Search entries in this wallet")
            checkable: true
        },
        Kirigami.Action {
            id: lockAction
            text: App.walletModel.locked ? i18n("Unlock") : i18n("Lock")
            icon.name: App.walletModel.locked ? "unlock-symbolic" : "lock-symbolic"
            tooltip: App.walletModel.locked ? i18n("Unlock this wallet") : i18n("Lock this wallet")
            onTriggered: {
                if (App.walletModel.locked) {
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

    ListView {
        id: view
        currentIndex: -1
        model: KSortFilterProxyModel {
            sourceModel: App.walletModel
            sortRole: Qt.Display
            sortCaseSensitivity: Qt.CaseInsensitive
            filterRole: Qt.Display
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
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: view.count === 0
            icon.name: {
                switch (page.state) {
                case WalletContentsPage.Locked:
                    return "folder-locked"
                case WalletContentsPage.Empty:
                    return "wallet-closed"
                default:
                    return ""
                }
            }
            text: {
                switch (page.state) {
                case WalletContentsPage.Locked:
                    return i18n("Wallet is locked")
                case WalletContentsPage.Empty:
                    return i18n("Wallet is empty")
                default:
                    return ""
                }
            }
            helpfulAction: {
                switch (page.state) {
                case WalletContentsPage.Locked:
                    return lockAction
                case WalletContentsPage.Empty:
                    return newAction
                default:
                    return null
                }
            }
        }
    }
}
