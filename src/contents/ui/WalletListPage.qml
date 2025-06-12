// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    title: Kirigami.Settings.isMobile ? i18n("Wallets") : ""

    readonly property string currentWallet: App.walletModel.currentWallet

    actions: [
        Kirigami.Action {
            text: i18n("New Wallet")
            icon.name: "list-add-symbolic"
            tooltip: i18n("Create a new wallet")
            onTriggered: root.counter += 1
        }
    ]

    ListView {
        id: view
        currentIndex: App.walletsModel.currentIndex
        model: App.walletsModel
        delegate: QQC.ItemDelegate {
            required property var model
            required property int index
            readonly property bool current: App.walletModel.currentWallet === model.display
            width: view.width
            icon.name: "wallet-closed"
            text: model.display
            highlighted: view.currentIndex == index
            onClicked: {
                App.walletModel.currentWallet = model.display
              //  view.currentIndex = index
                page.Kirigami.ColumnView.view.currentIndex = 1
            }
        }
    }
}
