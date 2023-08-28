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

import MuseScore.Ui 1.0
import MuseScore.UiComponents 1.0
import MuseScore.Inspector 1.0

import "../../common"
import "internal"

Item {
    id: root

    property QtObject proxyModel: null

    property NavigationPanel navigationPanel: null
    property int navigationRowStart: 1

    implicitHeight: contentColumn.height
    width: parent.width

    Column {
        id: contentColumn

        width: parent.width
        height: childrenRect.height

        spacing: 12

        property var fullModel: [
            { typeRole: Inspector.TYPE_NOTE, componentRole: noteSection },
            { typeRole: Inspector.TYPE_ARPEGGIO, componentRole: arpeggioSection },
            { typeRole: Inspector.TYPE_FERMATA, componentRole: fermataSection },
            { typeRole: Inspector.TYPE_BREATH, componentRole: pausesSection },
            { typeRole: Inspector.TYPE_GLISSANDO, componentRole: glissandoSection },
            { typeRole: Inspector.TYPE_GRADUAL_TEMPO_CHANGE, componentRole: tempoChangeSection }
        ]

        function getVisibleModel() {
            var visibleModel = [];
            var currType = null;
            for (let i = 0; i < fullModel.length; i++) {
                currType = root.proxyModel ? root.proxyModel.modelByType(fullModel[i].typeRole) : null;
                if (Boolean(currType) && !currType.isEmpty) {
                    visibleModel.push(fullModel[i]);
                }
            }
            return visibleModel;
        }

        Repeater {
            id: repeater

            model: contentColumn.getVisibleModel()

            delegate: Column {
                id: itemColumn

                width: parent.width
                height: childrenRect.height

                spacing: contentColumn.spacing

                visible: expandableLoader.active

                SeparatorLine {
                    anchors.margins: -12
                    visible: model.index>0
                }

                Loader {
                    id: expandableLoader

                    property var itemModel: root.proxyModel ?  proxyModel.modelByType(modelData.typeRole) : null

                    width: parent.width

                    sourceComponent: modelData["componentRole"]

                    onLoaded: {
                        expandableLoader.item.model = expandableLoader.itemModel
                    }
                }
            }
        }
    }

    Component {
        id: noteSection

        NoteExpandableBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart
        }
    }

    Component {
        id: arpeggioSection

        ArpeggioExpandableBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart + 1000
        }
    }

    Component {
        id: fermataSection

        FermataExpandableBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart + 2000
        }
    }

    Component {
        id: pausesSection

        PausesExpandableBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart + 3000
        }
    }

    Component {
        id: glissandoSection

        GlissandoExpandableBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart + 4000
        }
    }

    Component {
        id: tempoChangeSection

        GradualTempoChangeBlank {
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRowStart + 5000
        }
    }
}
