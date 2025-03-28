// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    title: App.itemsModel.currentWallet

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
        model: App.itemsModel
        onModelChanged: currentIndex = -1
        section.property: "folder"
        section.delegate: RowLayout {
            x: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                source: "folder"
                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                implicitHeight: implicitWidth
            }
            Kirigami.Heading {
                level: 4
                text: section
                color: Kirigami.Theme.textColor
               // height: implicitHeight + Kirigami.Units.largeSpacing
               // verticalAlignment: Qt.AlignBottom
            }
            Kirigami.Separator {
                Layout.alignment: Qt.alignCenter
                Layout.fillWidth: true
            }
        }
        section.criteria: ViewSection.FullString
        delegate: QQC.ItemDelegate {
            required property var model
            required property int index
            width: view.width
            //icon.name: "encrypted"
            leftPadding: 35
            text: model.display
            highlighted: view.currentIndex == index
            onClicked: {
                view.currentIndex = index
                App.secretItem.loadItem(App.itemsModel.currentWallet, model.folder, model.display);
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: view.count === 0
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
