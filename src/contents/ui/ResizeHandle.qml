// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kwallets

// TODO: a component in Kirigami to attach resize handles directly in ColumnView
MouseArea {
    property Kirigami.Page targetPage
    property bool onRight
    visible: root.pageStack.wideMode
    cursorShape: Qt.SizeHorCursor
    property real minimumWidth: root.minimumSidebarWidth
    property real maximumWidth: root.pageStack.defaultColumnWidth

    anchors {
        top: parent.top
        bottom: parent.bottom
    }
    x: targetPage.parent.x + targetPage.x + (onRight ? targetPage.width : 0) - width / 2
    z: 999
    width: Kirigami.Units.gridUnit / 4

    property real __startX: 0
    property real __startWidth: 0

    onPressed: mouse => {
        let pos = mapToItem(null, mouse.x, mouse.y)
        __startX = pos.x
        __startWidth = targetPage.width
    }
    onPositionChanged: mouse => {
        let pos = mapToItem(null, mouse.x, mouse.y)
        let w = 0
        if (onRight) {
            w = __startWidth - __startX + pos.x
        } else {
            w = __startWidth + __startX - pos.x
        }
        if (onRight) {
            root.leadingSidebarWidth = Math.max(minimumWidth, Math.min(maximumWidth, w))
        } else {
            root.trailingSidebarWidth = Math.max(minimumWidth, Math.min(maximumWidth, w))
        }
    }
}

