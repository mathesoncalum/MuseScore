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

#include "notesinchordselectionfiltermodel.h"

#include "log.h"
#include "translation.h"

using namespace mu::notation;
using namespace mu::engraving;

NotesInChordSelectionFilterModel::NotesInChordSelectionFilterModel(QObject* parent)
    : AbstractSelectionFilterModel(parent)
{
}

bool NotesInChordSelectionFilterModel::enabled() const
{
    const INotationSelectionPtr selection = currentNotationInteraction() ? currentNotationInteraction()->selection() : nullptr;
    return selection && selection->isRange() && m_topNoteIdx > 0;
}

void NotesInChordSelectionFilterModel::loadTypes()
{
    for (size_t i = 0; i < NUM_NOTES_IN_CHORD_SELECTION_FILTER_TYPES; i++) {
        m_types << static_cast<NotesInChordSelectionFilterTypes>(1 << i);
    }
}

bool NotesInChordSelectionFilterModel::isAllowed(const SelectionFilterTypesVariant& variant) const
{
    const NotesInChordSelectionFilterTypes type = std::get<NotesInChordSelectionFilterTypes>(variant);
    return noteIdxForType(type) <= m_topNoteIdx;
}

QString NotesInChordSelectionFilterModel::titleForType(const SelectionFilterTypesVariant& variant) const
{
    const NotesInChordSelectionFilterTypes type = std::get<NotesInChordSelectionFilterTypes>(variant);

    if (noteIdxForType(type) == m_topNoteIdx) {
        return muse::qtrc("notation", "Top note");
    }

    switch (type) {
    case NotesInChordSelectionFilterTypes::ALL:
        return muse::qtrc("notation", "All");
    case NotesInChordSelectionFilterTypes::EIGHTH_NOTE:
        return muse::qtrc("notation", "Eighth note");
    case NotesInChordSelectionFilterTypes::SEVENTH_NOTE:
        return muse::qtrc("notation", "Seventh note");
    case NotesInChordSelectionFilterTypes::SIXTH_NOTE:
        return muse::qtrc("notation", "Sixth note");
    case NotesInChordSelectionFilterTypes::FIFTH_NOTE:
        return muse::qtrc("notation", "Fifth note");
    case NotesInChordSelectionFilterTypes::FOURTH_NOTE:
        return muse::qtrc("notation", "Fourth note");
    case NotesInChordSelectionFilterTypes::THIRD_NOTE:
        return muse::qtrc("notation", "Third note");
    case NotesInChordSelectionFilterTypes::SECOND_NOTE:
        return muse::qtrc("notation", "Second note");
    case NotesInChordSelectionFilterTypes::BOTTOM_NOTE:
        return muse::qtrc("notation", "Bottom note");
    case NotesInChordSelectionFilterTypes::NONE:
        break;
    }

    return {};
}

bool NotesInChordSelectionFilterModel::isIndeterminate(const SelectionFilterTypesVariant& variant) const
{
    const NotesInChordSelectionFilterTypes type = std::get<NotesInChordSelectionFilterTypes>(variant);

    if (type == NotesInChordSelectionFilterTypes::ALL) {
        return !isFiltered(NotesInChordSelectionFilterTypes::ALL) && !isFiltered(NotesInChordSelectionFilterTypes::NONE);
    }

    return false;
}

void NotesInChordSelectionFilterModel::notifyAboutDataChanged(const QModelIndex& index, const SelectionFilterTypesVariant& variant)
{
    const NotesInChordSelectionFilterTypes type = std::get<NotesInChordSelectionFilterTypes>(variant);

    if (type == NotesInChordSelectionFilterTypes::ALL) {
        emit dataChanged(this->index(0), this->index(rowCount() - 1), { IsSelectedRole, IsIndeterminateRole });
        return;
    }

    emit dataChanged(this->index(0), this->index(0), { IsSelectedRole, IsIndeterminateRole });
    emit dataChanged(index, index, { IsSelectedRole });
}

bool NotesInChordSelectionFilterModel::includeSingleNotes() const
{
    return currentNotationInteraction() ? currentNotationInteraction()->selectionFilterIncludeSingleNotes() : false;
}

void NotesInChordSelectionFilterModel::setIncludeSingleNotes(bool include)
{
    if (currentNotationInteraction()) {
        currentNotationInteraction()->setSelectionFilterIncludeSingleNotes(include);
    }
    emit includeSingleNotesChanged();
}

void NotesInChordSelectionFilterModel::onSelectionChanged()
{
    updateTopNoteIdx();

    // Need to work this out - if we don't re-enable includeSingleNotes then you won't be able to select single notes
    // when the selection contains solely single notes...
    if (!enabled()) {
        // setIncludeSingleNotes(true);
    }

    emit dataChanged(this->index(0), this->index(rowCount() - 1), { TitleRole, IsAllowedRole });
    emit enabledChanged();
}

void NotesInChordSelectionFilterModel::updateTopNoteIdx()
{
    const INotationSelectionPtr selection = currentNotationInteraction() ? currentNotationInteraction()->selection() : nullptr;
    if (!selection) {
        m_topNoteIdx = muse::nidx;
        return;
    }

    m_topNoteIdx = muse::nidx;

    QSet<const Chord*> scannedChords;
    for (const Note* note : selection->notes()) {
        const Chord* chord = note->chord();
        if (scannedChords.contains(chord)) {
            continue;
        }
        const size_t currIdx = chord->notes().size() - 1;
        if (m_topNoteIdx == muse::nidx || currIdx > m_topNoteIdx) {
            m_topNoteIdx = currIdx;
        }
        scannedChords.insert(chord);
    }
}

size_t NotesInChordSelectionFilterModel::noteIdxForType(const NotesInChordSelectionFilterTypes& type) const
{
    switch (type) {
    case NotesInChordSelectionFilterTypes::EIGHTH_NOTE: return 7;
    case NotesInChordSelectionFilterTypes::SEVENTH_NOTE: return 6;
    case NotesInChordSelectionFilterTypes::SIXTH_NOTE: return 5;
    case NotesInChordSelectionFilterTypes::FIFTH_NOTE: return 4;
    case NotesInChordSelectionFilterTypes::FOURTH_NOTE: return 3;
    case NotesInChordSelectionFilterTypes::THIRD_NOTE: return 2;
    case NotesInChordSelectionFilterTypes::SECOND_NOTE: return 1;
    case NotesInChordSelectionFilterTypes::BOTTOM_NOTE: return 0;
    case NotesInChordSelectionFilterTypes::ALL:
    case NotesInChordSelectionFilterTypes::NONE:
        break;
    }

    return muse::nidx;
}
