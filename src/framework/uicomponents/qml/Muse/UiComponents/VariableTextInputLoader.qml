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
import QtQuick 2.9

import Muse.Ui 1.0
import Muse.UiComponents 1.0

Loader {
    id: root

    property bool useTextArea: false

    property string currentText: defaultText
    property string defaultText: ""

    property NavigationPanel navigationPanel: null
    property int navigationColumn: 0
    property string navigationAccessibleName: ""

    sourceComponent: root.useTextArea ? textAreaComponent : textFieldComponent
    width: parent.width

    signal textChanged(var newTextValue)

    function updateInfo(newTextValue) {
        root.currentText = (newTextValue === "") ? root.defaultText : newTextValue
        root.textChanged(root.currentText)
    }

    Component {
        id: textFieldComponent

        TextInputField {
            id: textField

            hint: root.defaultText

            navigation.panel: root.navigationPanel
            navigation.column: root.navigationColumn
            navigation.accessible.name: root.navigationAccessibleName

            onTextChanged: function(newTextValue) {
                root.updateInfo(newTextValue)
            }
        }
    }

    Component {
        id: textAreaComponent

        TextInputArea {
            id: textArea

            hint: root.defaultText

            navigation.panel: root.navigationPanel
            navigation.column: root.navigationColumn
            navigation.accessible.name: root.navigationAccessibleName

            onTextChanged: function(newTextValue) {
                root.updateInfo(newTextValue)
            }
        }
    }
}
