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

    property alias currentWallet: view.currentIndex

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
        model: 5
        delegate: QQC.ItemDelegate {
            width: view.width
            icon.name: "wallet-closed-symbolic"
            text: modelData
            highlighted: view.currentIndex == index
            onClicked: view.currentIndex = index
        }
    }
}
