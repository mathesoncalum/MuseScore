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

#pragma once

#include "../mscore.h"

#include "abstractselectionfilter.h"

namespace mu::engraving {
static constexpr size_t NUM_NOTES_IN_CHORD_SELECTION_FILTER_TYPES = 8;

enum class NotesInChordSelectionFilterTypes : unsigned int {
    NONE                    = 0,
    EIGHTH_NOTE             = 1 << 0,
    SEVENTH_NOTE            = 1 << 1,
    SIXTH_NOTE              = 1 << 2,
    FIFTH_NOTE              = 1 << 3,
    FOURTH_NOTE             = 1 << 4,
    THIRD_NOTE              = 1 << 5,
    SECOND_NOTE             = 1 << 6,
    BOTTOM_NOTE             = 1 << 7,
    ALL                     = ~(~0u << NUM_NOTES_IN_CHORD_SELECTION_FILTER_TYPES)
};

class NotesInChordSelectionFilter : public AbstractSelectionFilter
{
public:
    NotesInChordSelectionFilter(NotesInChordSelectionFilterTypes type = NotesInChordSelectionFilterTypes::ALL);

    unsigned int getAll() const override { return static_cast<unsigned int>(NotesInChordSelectionFilterTypes::ALL); }
    unsigned int getNone() const override { return static_cast<unsigned int>(NotesInChordSelectionFilterTypes::NONE); }

    bool isFiltered(const NotesInChordSelectionFilterTypes& type) const;
    void setFiltered(const NotesInChordSelectionFilterTypes& type, bool filtered);

    bool canSelectNote(size_t noteIdx) const;

    bool includeSingleNotes() const { return m_includeSingleNotes; }
    void setIncludeSingleNotes(bool include) { m_includeSingleNotes = include; }

private:
    bool m_includeSingleNotes = true;
};
}
