/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited
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

#include "percussionpanelpadlistmodel.h"

#include "notation/utilities/engravingitempreviewpainter.h"
#include "notation/utilities/percussionutilities.h"

using namespace mu::notation;

PercussionPanelPadListModel::PercussionPanelPadListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QVariant PercussionPanelPadListModel::data(const QModelIndex& index, int role) const
{
    if (!indexIsValid(index.row())) {
        return QVariant();
    }

    PercussionPanelPadModel* item = m_padModels.at(index.row());

    switch (role) {
    case PadModelRole: return QVariant::fromValue(item);
    default: break;
    }

    return QVariant();
}

QHash<int, QByteArray> PercussionPanelPadListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { PadModelRole, "padModelRole" },
    };
    return roles;
}

void PercussionPanelPadListModel::init()
{
    m_padModels.clear();
    addRow();
}

void PercussionPanelPadListModel::addRow()
{
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        m_padModels.append(nullptr);
    }
    emit layoutChanged();
    emit numPadsChanged();
}

void PercussionPanelPadListModel::deleteRow(int row)
{
    m_padModels.remove(row * NUM_COLUMNS, NUM_COLUMNS);
    emit layoutChanged();
    emit numPadsChanged();
}

bool PercussionPanelPadListModel::rowIsEmpty(int row) const
{
    return numEmptySlotsAtRow(row) == NUM_COLUMNS;
}

void PercussionPanelPadListModel::startDrag(int startIndex)
{
    m_dragStartIndex = startIndex;
}

void PercussionPanelPadListModel::endDrag(int endIndex)
{
    if (indexIsValid(m_dragStartIndex) && indexIsValid(endIndex)) {
        movePad(m_dragStartIndex, endIndex);
    } else {
        emit layoutChanged();
    }
    m_dragStartIndex = -1;
}

void PercussionPanelPadListModel::setDrumset(const mu::engraving::Drumset* drumset)
{
    if (drumset == m_drumset) {
        return;
    }

    const bool drumsetWasValid = m_drumset;

    m_drumset = drumset;

    if (drumsetWasValid ^ bool(m_drumset)) {
        m_hasActivePadsChanged.notify();
    }
}

void PercussionPanelPadListModel::resetLayout()
{
    beginResetModel();

    m_padModels.clear();

    if (!m_drumset) {
        endResetModel();
        addRow();
        return;
    }

    for (int pitch = 0; pitch < mu::engraving::DRUM_INSTRUMENTS; ++pitch) {
        if (!m_drumset->isValid(pitch)) {
            continue;
        }

        PercussionPanelPadModel* model = new PercussionPanelPadModel(this);
        model->setInstrumentName(m_drumset->name(pitch));

        const QString shortcut = m_drumset->shortcut(pitch) ? QChar(m_drumset->shortcut(pitch)) : QString("-");
        model->setKeyboardShortcut(shortcut);

        model->setPitch(pitch);

        model->padTriggered().onNotify(this, [this, pitch]() {
            m_triggeredChannel.send(pitch);
        });

        model->setNotationPreviewItem(PercussionUtilities::getDrumNoteForPreview(m_drumset, pitch));

        const int panelRow = m_drumset->panelRow(pitch);
        const int panelColumn = m_drumset->panelColumn(pitch);

        IF_ASSERT_FAILED(panelRow > -1 && panelColumn > -1 && panelColumn < NUM_COLUMNS) {
            LOGE() << "Percussion panel - invalid row/column data for drum: " << m_drumset->name(pitch);
            m_padModels.clear();
            return;
        }

        const int modelIndex = panelRow * NUM_COLUMNS + panelColumn;

        if (modelIndex < m_padModels.size()) {
            IF_ASSERT_FAILED(!m_padModels.at(modelIndex)) {
                LOGE() << "Percussion panel - layout conflict at row " << panelRow << ", column " << panelColumn;
                m_padModels.clear();
                return;
            }
        }

        // Note - probably use a different data structure here
        if (modelIndex >= m_padModels.size()) {
            m_padModels.resize(modelIndex + 1);
        }

        m_padModels.insert(modelIndex, model);
    }

    // Fill the remainder of the column with empty pads...
    while (m_padModels.size() % NUM_COLUMNS > 0) {
        m_padModels.append(nullptr);
    }

    endResetModel();

    emit numPadsChanged();
}

bool PercussionPanelPadListModel::indexIsValid(int index) const
{
    return index > -1 && index < m_padModels.count();
}

void PercussionPanelPadListModel::movePad(int fromIndex, int toIndex)
{
    const int fromRow = fromIndex / NUM_COLUMNS;
    const int toRow = toIndex / NUM_COLUMNS;

    // fromRow will become empty if there's only 1 "occupied" slot, toRow will no longer be empty if it was previously...
    const bool fromRowEmptyChanged = numEmptySlotsAtRow(fromRow) == NUM_COLUMNS - 1;
    const bool toRowEmptyChanged = rowIsEmpty(toRow);

    m_padModels.swapItemsAt(fromIndex, toIndex);
    emit layoutChanged();

    if (fromRowEmptyChanged) {
        emit rowIsEmptyChanged(fromRow, /*isEmpty*/ true);
    }

    if (toRowEmptyChanged) {
        emit rowIsEmptyChanged(toRow, /*isEmpty*/ false);
    }
}

int PercussionPanelPadListModel::numEmptySlotsAtRow(int row) const
{
    int count = 0;
    const size_t rowStartIdx = row * NUM_COLUMNS;
    for (size_t i = rowStartIdx; i < rowStartIdx + NUM_COLUMNS; ++i) {
        if (!m_padModels.at(i)) {
            ++count;
        }
    }
    return count;
}
