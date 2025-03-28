// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

Kirigami.ScrollablePage {
    id: page

    title: App.secretItem.itemName

    actions: [
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete-symbolic"
            tooltip: i18n("Delete this entry")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: root.counter += 1
        }
    ]

    ColumnLayout {
        QQC.Label {
            text: i18n("Value:")
        }
        Kirigami.PasswordField {
            text: App.secretItem.secretValue
        }
    }
}
