/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
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
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents 1.0
import MuseScore.NotationScene 1.0

StyledPopupView {
    id: root

    property alias notationViewNavigationSection: staffVisibilityNavPanel.section
    property alias navigationOrderStart: staffVisibilityNavPanel.order
    readonly property alias navigationOrderEnd: staffVisibilityNavPanel.order

    contentWidth: content.width
    contentHeight: content.height

    signal elementRectChanged(var elementRect)

    function updatePosition() {
        // TODO: Positioning not quite right
        root.x = root.parent.width / 2 - root.contentWidth / 2
        root.y = root.parent.height
    }

    NavigationPanel {
        id: staffVisibilityNavPanel
        name: "StaffVisibility"
        direction: NavigationPanel.Vertical
        accessible.name: qsTrc("notation", "Staff visibility")

        onNavigationEvent: function(event) {
            if (event.type === NavigationEvent.Escape) {
                root.close()
            }
        }
    }

    Rectangle {
        id: content
        width: 50
        height: 200
        color: "red"
    }
}
