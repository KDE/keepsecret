// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// TODO: Future Form layout things are needed like that, as can't even have a show password button in FormTextFieldDelegate
Kirigami.HeaderFooterLayout {
    id: root

    property string label

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    Layout.rightMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    Layout.topMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    Layout.bottomMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing

    spacing: Kirigami.Units.mediumSpacing

    header: QQC.Label {
        visible: text.length > 0
        text: root.label
    }
}
