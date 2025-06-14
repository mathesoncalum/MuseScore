/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
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

import Muse.UiComponents 1.0
import Muse.Ui 1.0

import MuseScore.NotationScene 1.0

RowLayout {
    id: root

    required property StyleItem styleItem
    property string label
    property alias labelComponent: labelLoader.sourceComponent

    property double labelAreaWidth: 120
    property double controlAreaWidth: 326

    default property alias data: control.data

    property bool hasReset: true
    property var resetEnabled: null
    property var resetOnClicked: null

    spacing: 8

    Loader {
        id: labelLoader
        Layout.preferredWidth: root.labelAreaWidth

        sourceComponent: StyledTextLabel {
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: root.label
        }
    }

    Item {
        id: control
        Layout.preferredWidth: root.controlAreaWidth
        implicitHeight: children.length === 1 ? children[0].implicitHeight : 0
    }

    FlatButton {
        id: resetButton
        visible: hasReset
        icon: IconCode.UNDO
        enabled: resetEnabled ? resetEnabled : !styleItem.isDefault
        onClicked: resetOnClicked ? resetOnClicked : styleItem.value = styleItem.defaultValue
    }
}
