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
            onTriggered: print("stub")
        }
    ]

    QQC.Menu {
        id: contextMenu
        property string collection
        QQC.MenuItem {
            text: i18n("Set as Default")
            enabled: App.secretService.defaultCollection !== contextMenu.collection
            onClicked: App.secretService.defaultCollection = contextMenu.collection
        }
        QQC.MenuItem {
            text: i18n("Lock")
            icon.name: "lock-symbolic"
            onClicked: print("Wallet delete Not implemented yet")
        }
        QQC.MenuItem {
            text: i18n("Delete")
            icon.name: "usermenu-delete-symbolic"
            onClicked: print("Wallet delete Not implemented yet")
        }
    }
    ListView {
        id: view
        currentIndex: App.walletsModel.currentIndex
        model: App.walletsModel
        delegate: QQC.ItemDelegate {
            id: delegate
            required property var model
            required property int index
            readonly property bool current: App.walletModel.currentWallet === model.display
            width: view.width
            icon.name: highlighted && !App.walletModel.locked ? "wallet-open" : "wallet-closed"
            text: model.display
            highlighted: view.currentIndex == index
            font.bold: App.secretService.defaultCollection === model.display
            onClicked: {
                App.walletModel.currentWallet = model.display
                page.Kirigami.ColumnView.view.currentIndex = 1
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                onPressedChanged: {
                    if (pressed) {
                        contextMenu.collection = model.display
                        contextMenu.popup(delegate)
                    }
                }
            }
        }
    }
}
