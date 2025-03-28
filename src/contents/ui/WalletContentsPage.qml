// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    title: i18n("Name")

    property alias currentEntry: view.currentIndex
    property int state: WalletContentsPage.Loaded

    enum State {
        NoWallet,
        Locked,
        Empty,
        Loaded
    }

    actions: [
        Kirigami.Action {
            text: i18n("Search")
            icon.name: "search-symbolic"
            tooltip: i18n("Search entries in this wallet")
            onTriggered: root.counter += 1
        },
        Kirigami.Action {
            text: i18n("Lock")
            icon.name: "lock-symbolic"
            tooltip: i18n("Lock this wallet")
            onTriggered: {
                walletListPage.currentWallet = -1
                currentEntry = -1
                page.state = WalletContentsPage.Locked
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

    ListView {
        id: view
        currentIndex: -1
        model: page.state == WalletContentsPage.Loaded ? 100 : 0
        onModelChanged: currentIndex = -1
        delegate: QQC.ItemDelegate {
            width: view.width
            icon.name: "folder"
            text: modelData
            highlighted: view.currentIndex == index
            onClicked: view.currentIndex = index
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: page.state != WalletContentsPage.Loaded
            icon.name: "folder-locked-symbolic"
            text: i18n("Wallet is locked")
            helpfulAction: Kirigami.Action {
                text: i18n("Unlock")
                onTriggered: {
                    page.state = WalletContentsPage.Loaded
                }
            }
        }
    }
}
