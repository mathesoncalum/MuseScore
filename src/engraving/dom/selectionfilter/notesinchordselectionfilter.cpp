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

#include "notesinchordselectionfilter.h"

using namespace mu::engraving;

NotesInChordSelectionFilter::NotesInChordSelectionFilter(NotesInChordSelectionFilterTypes type)
    : AbstractSelectionFilter()
{
    m_filteredTypes = static_cast<int>(type);
}

bool NotesInChordSelectionFilter::isFiltered(const NotesInChordSelectionFilterTypes& type) const
{
    return AbstractSelectionFilter::isFiltered(static_cast<unsigned int>(type));
}

void NotesInChordSelectionFilter::setFiltered(const NotesInChordSelectionFilterTypes& type, bool filtered)
{
    AbstractSelectionFilter::setFiltered(static_cast<unsigned int>(type), filtered);
}

bool NotesInChordSelectionFilter::canSelectNote(size_t noteIdx) const
{
    switch (noteIdx) {
    case 0:
        return isFiltered(NotesInChordSelectionFilterTypes::BOTTOM_NOTE);
    case 1:
        return isFiltered(NotesInChordSelectionFilterTypes::SECOND_NOTE);
    case 2:
        return isFiltered(NotesInChordSelectionFilterTypes::THIRD_NOTE);
    case 3:
        return isFiltered(NotesInChordSelectionFilterTypes::FOURTH_NOTE);
    case 4:
        return isFiltered(NotesInChordSelectionFilterTypes::FIFTH_NOTE);
    case 5:
        return isFiltered(NotesInChordSelectionFilterTypes::SIXTH_NOTE);
    case 6:
        return isFiltered(NotesInChordSelectionFilterTypes::SEVENTH_NOTE);
    case 7:
        return isFiltered(NotesInChordSelectionFilterTypes::EIGHTH_NOTE);
    }
    return true;
}
