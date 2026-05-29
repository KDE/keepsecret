// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

import QtQuick
import org.kde.kirigami.actioncollection as AC

AC.ActionCollectionManager {
    id: manager
    pageRow: root.pageStack

    AC.ActionCollection {
        name: "org.kde.keepsecret.collections"
        text: i18n("Wallet List Actions")

        AC.ActionData {
            name: "new-wallet"
            text: i18nc("@action:button", "New Wallet")
            icon.name: "list-add-symbolic"
        }
    }

    AC.ActionCollection {
        name: "org.kde.keepsecret.collection"
        text: i18n("Wallet Actions")

        AC.ActionData {
            name: "new-entry"
            text: i18nc("@action:button Create a new secret", "New Entry")
            icon.name: "list-add-symbolic"
        }
        AC.ActionData {
            name: "search"
            text: i18nc("@action:button", "Search")
            icon.name: "search-symbolic"
        }
        AC.ActionData {
            name: "lock"
            text: i18nc("@action:inmenu lock this wallet", "Lock")
            icon.name: "lock-symbolic"
        }
    }
}