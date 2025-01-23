/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.15

import Muse.UiComponents 1.0
import Muse.Shortcuts 1.0

import MuseScore.NotationScene 1.0

StyledDialogView {
    id: root

    property var originDrum: null
    property var drumsWithShortcut: null

    property var appNoteInputShortcuts: null

    title: qsTrc("shortcuts", "Enter shortcut")

    contentWidth: 538
    contentHeight: 200

    margins: 20

    onNavigationActivateRequested: {
        editShortcutDialogContent.requestActive()
    }

    Component.onCompleted: {
        model.load(root.originDrum, root.drumsWithShortcut, root.appNoteInputShortcuts)
    }

    EditPercussionShortcutModel {
        id: model
    }

    EditShortcutDialogContent {
        id: editShortcutDialogContent

        navigationSection: root.navigationSection

        headerText: qsTrc("shortcuts", "Define percussion keyboard shortcut")

        originShortcutText: model.originShortcutText
        newShortcutText: model.newShortcutText
        informationText: model.informationText

        onSaveRequested: {
            model.trySave()
            root.accept()
        }

        onCancelRequested: {
            root.reject()
        }

        onKeyPressed: function(event) {
            model.inputKey(event.key)
        }
    }
}
