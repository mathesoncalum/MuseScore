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

#include <engraving/types/pitchvalue.h>
#include <engraving/types/types.h>

#include "benddatacontextsplitchord.h"

namespace mu::engraving {
class Note;
class Chord;
}

namespace mu::iex::guitarpro {
class BendDataCollectorSplitChord
{
public:

    void storeBendData(const mu::engraving::Note* note, const mu::engraving::PitchValues& pitchValues);
    BendDataContextSplitChord collectBendDataContext();

private:
    std::unordered_map<mu::engraving::track_idx_t,
                       std::map<mu::engraving::Fraction,
                                std::unordered_map<const mu::engraving::Note*, ImportedBendInfo> > > m_bendInfoForNote;

    void fillBendsDurations(BendDataContextSplitChord& bendDataCtx);
    void fillBendData(BendDataContextSplitChord& bendDataCtx);
};
} // mu::iex::guitarpro
