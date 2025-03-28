// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    //title: i18n("Wallets")

    readonly property string currentWallet: App.itemsModel.currentWallet

    actions: [
        Kirigami.Action {
            text: i18n("New Wallet")
            icon.name: "list-add"
            tooltip: i18n("Create a new wallet")
            onTriggered: root.counter += 1
        }
    ]

    ListView {
        id: view
        currentIndex: -1
        model: App.walletsModel
        delegate: QQC.ItemDelegate {
            required property var model
            required property int index
            width: view.width
            icon.name: "wallet-closed-symbolic"
            text: model.display
            highlighted: view.currentIndex == index
            onClicked: {
                App.itemsModel.currentWallet = model.display
                view.currentIndex = index
            }
        }
    }
}
