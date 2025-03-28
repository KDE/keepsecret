// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    title: i18n("Wallets")

    actions: [
        Kirigami.Action {
            text: i18n("Plus One")
            icon.name: "list-add"
            tooltip: i18n("Add one to the counter")
            onTriggered: root.counter += 1
        }
    ]

    ListView {
        model: 100
        delegate: QQC.ItemDelegate {
            width: parent.width
            text: modelData
        }
    }
}
