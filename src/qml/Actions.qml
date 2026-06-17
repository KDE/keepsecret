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
        AC.ActionData {
            name: "unlock"
            text: i18nc("@action:inmenu unlock this wallet", "Unlock")
            icon.name: "unlock-symbolic"
        }
        AC.ActionData {
            name: "delete-wallet"
            text: i18nc("@title:window Delete this wallet", "Delete Wallet")
            icon.name: "delete-symbolic"
        }
        AC.ActionData {
            name: "export-wallet"
            text: i18nc("@action:inmenu", "Export…")
            icon.name: "document-export"
        }
        AC.ActionData {
            name: "import-wallet"
            text: i18nc("@action:inmenu", "Import…")
            icon.name: "document-import"
        }
    }
    AC.ActionCollection {
        name: "org.kde.keepsecret.item"
        text: i18n("Item Actions")
        AC.ActionData {
            name: "copy-password"
            text: i18nc("@action:button", "Copy Password")
            icon.name: "edit-copy-symbolic"
        }
        AC.ActionData {
            name: "save"
            text: i18nc("@action:button Save changes made to this secret", "Save")
            icon.name: "document-save"
        }
        AC.ActionData {
            name: "revert"
            text: i18nc("@action:button Revert changes made to this secret", "Revert")
            icon.name: "document-revert-symbolic"
        }
        AC.ActionData {
            name: "delete"
            text: i18nc("@action:button Delete this secret", "Delete Secret")
            icon.name: "delete-symbolic"
        }
    }
}