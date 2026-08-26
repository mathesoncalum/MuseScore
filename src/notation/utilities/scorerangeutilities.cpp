/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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

#include "scorerangeutilities.h"

#include "engraving/dom/box.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/page.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/system.h"

using namespace mu::notation;
using namespace mu::engraving;

std::vector<muse::RectF> ScoreRangeUtilities::boundingArea(const Score* score,
                                                           const Segment* startSegment, const Segment* endSegment,
                                                           staff_idx_t startStaffIndex, staff_idx_t endStaffIndex,
                                                           const engraving::Box* startBox, const engraving::Box* endBox)
{
    if (!score || !startSegment || !endSegment || startSegment->tick() > endSegment->tick()) {
        return {};
    }

    std::vector<RectF> result;

    const std::vector<RangeSection> sections = splitRangeBySections(score, startSegment, endSegment, startBox, endBox);

    for (const RangeSection& section : sections) {
        const staff_idx_t firstStaff = firstVisibleStaffIdx(score, section.system, startStaffIndex);
        const staff_idx_t lastStaff = lastVisibleStaffIdx(score, section.system, endStaffIndex);
        if (firstStaff == muse::nidx || lastStaff == muse::nidx) {
            continue;
        }

        const SysStaff* segmentFirstStaff = section.system->staff(firstStaff);
        const SysStaff* segmentLastStaff = section.system->staff(lastStaff);

        const Staff* scoreFirstStaff = score->staff(firstStaff);
        const Staff* scoreLastStaff = score->staff(lastStaff);

        const double standardStaffHeight = 4 * scoreFirstStaff->spatium(Fraction(0, 1));
        const double firstStaffHeight = scoreFirstStaff->staffHeight();
        const double lastStaffHeight = scoreLastStaff->staffHeight();

        double topY = 0.0;
        if (firstStaffHeight < standardStaffHeight) {
            const double diff = standardStaffHeight - firstStaffHeight;
            topY -= 0.5 * diff;
        }

        double bottomY = lastStaffHeight;
        if (lastStaffHeight < standardStaffHeight) {
            const double diff = standardStaffHeight - lastStaffHeight;
            bottomY += 0.5 * diff;
        }

        double x1 = 0.0;
        if (section.startBox) {
            x1 = section.startBox->pageBoundingRect().left();
        } else {
            x1 = section.startSegment->pagePos().x();
        }

        double x2 = 0.0;
        if (section.endBox) {
            x2 = section.endBox->pageBoundingRect().right();
        } else {
            x2 = section.endSegment->pageBoundingRect().right();
        }

        const int padding = 0.5 * scoreFirstStaff->spatium(startSegment->tick());
        const double y1 = topY + segmentFirstStaff->y() + section.startSegment->pagePos().y() - padding;
        const double y2 = bottomY + segmentLastStaff->y() + section.endSegment->pagePos().y() + padding;

        if (!section.startBox && section.startSegment->measure()->firstEnabled() == section.startSegment) {
            x1 = section.startSegment->measure()->pagePos().x();
        }

        const RectF rect = RectF(PointF(x1, y1), PointF(x2, y2)).translated(section.system->page()->pos());
        result.push_back(rect);
    }

    return result;
}

std::vector<ScoreRangeUtilities::RangeSection> ScoreRangeUtilities::splitRangeBySections(
    const Score* score,
    const Segment* rangeStartSegment, const Segment* rangeEndSegment,
    const Box* startBox, const Box* endBox)
{
    std::vector<RangeSection> sections;

    const Segment* startSegment = rangeStartSegment;
    const Fraction rangeEndTick = rangeEndSegment->tick();

    const Box* currentStartBox = startBox; // the Box at current section start (if any)...

    const auto boxAt = [score](const Segment* seg, bool first) -> const Box* {
        const Box* box = nullptr;
        for (MeasureBase* mb : score->measureBasesAtTick(seg->tick())) {
            if (!mb->isBox() || mb->system() != seg->system()) {
                continue;
            }
            box = toBox(mb);
            if (first) {
                break;
            }
        }
        return box;
    };

    // A Box shares its tick with the measure which follows it, so the range's start/end Box may belong
    // to the system before/after the one we're currently working on...
    const auto boxOnSystem = [](const Box* box, const System* system) -> const Box* {
        return box && box->system() == system ? box : nullptr;
    };

    // The range's start/end Box can exist on a system which the segment loop below never visits (e.g. a range that starts
    // with a Box at the end of a system, or a range that ends at a Box at the start of a system). This lambda constructs
    // range sections for these cases...
    const auto addBoxOnlySection = [&sections, rangeStartSegment, rangeEndSegment](const Box* box, bool boxIsRangeStart) {
        const System* system = box->system();
        if (!system) {
            return;
        }

        // A Box holds no segments, so borrow the nearest segment on its system for the section's vertical extent...
        const Segment* seg = boxIsRangeStart ? rangeStartSegment : rangeEndSegment;
        while (seg && seg->system() != system) {
            seg = boxIsRangeStart ? seg->prev1MMenabled() : seg->next1MMenabled();
        }

        if (!seg) {
            return;
        }

        // Collect the run of Boxes extending from box towards the range's music...
        const Box* oppositeEndBox = box;
        for (const MeasureBase* mb = boxIsRangeStart ? box->next() : box->prev();
             mb && mb->isBox() && mb->system() == system;
             mb = boxIsRangeStart ? mb->next() : mb->prev()) {
            oppositeEndBox = toBox(mb);
        }

        RangeSection section;
        section.system = system;
        section.startSegment = seg;
        section.endSegment = seg;
        section.startBox = boxIsRangeStart ? box : oppositeEndBox;
        section.endBox = boxIsRangeStart ? oppositeEndBox : box;

        sections.push_back(section);
    };

    if (startBox && startBox->system() != rangeStartSegment->system()) {
        addBoxOnlySection(startBox, /*boxIsRangeStart*/ true);
    }

    for (const Segment* segment = startSegment; segment && segment != rangeEndSegment && segment->tick() < rangeEndTick;) {
        const System* currentSegmentSystem = segment->measure()->system();

        const Segment* nextSegment = segment->next1MMenabled();
        while (nextSegment && !nextSegment->visible()) {
            nextSegment = nextSegment->next1MMenabled();
        }

        if (!nextSegment) {
            RangeSection section;
            section.system = currentSegmentSystem;
            section.startSegment = startSegment;
            section.endSegment = segment;

            section.startBox = boxOnSystem(currentStartBox, currentSegmentSystem);
            section.endBox = boxOnSystem(endBox, currentSegmentSystem);

            sections.push_back(section);
            break;
        }

        const System* nextSegmentSystem = nextSegment->measure()->system();
        if (!nextSegmentSystem) {
            const Measure* mmr = nextSegment->measure()->coveringMMRestOrThis();
            if (mmr) {
                nextSegmentSystem = mmr->system();
            }
            if (!nextSegmentSystem) {
                break;
            }
        }

        const bool nextIsNewSystem = nextSegmentSystem != currentSegmentSystem;
        const bool nextIsOutOfRange = nextSegment->tick() >= rangeEndTick;
        if (nextIsNewSystem || nextIsOutOfRange) {
            RangeSection section;
            section.system = currentSegmentSystem;
            section.startSegment = startSegment;
            section.endSegment = segment;

            section.startBox = boxOnSystem(currentStartBox, currentSegmentSystem);
            section.endBox = nextIsOutOfRange
                              ? boxOnSystem(endBox, currentSegmentSystem)
                              : boxAt(segment, /*first*/ false);
            sections.push_back(section);

            startSegment = nextSegment;

            if (nextIsNewSystem) {
                // next segment is in a new system - look for a Box at start...
                currentStartBox = boxAt(nextSegment, /*first*/ true);
            }
        }

        segment = nextSegment;
    }

    // sections.back() is the system the range's music ended on, so the end Box is beyond the loop's reach if it isn't there...
    if (endBox && !sections.empty() && endBox->system() != sections.back().system) {
        addBoxOnlySection(endBox, /*boxIsRangeStart*/ false);
    }

    return sections;
}

staff_idx_t ScoreRangeUtilities::firstVisibleStaffIdx(const Score* score, const System* system, staff_idx_t startStaffIndex)
{
    for (staff_idx_t i = startStaffIndex; i < score->nstaves(); ++i) {
        if (system->staff(i)->show()) {
            return i;
        }
    }

    return muse::nidx;
}

staff_idx_t ScoreRangeUtilities::lastVisibleStaffIdx(const Score*, const System* system, staff_idx_t endStaffIndex)
{
    for (int i = static_cast<int>(endStaffIndex) - 1; i >= 0; --i) {
        if (system->staff(i)->show()) {
            return static_cast<staff_idx_t>(i);
        }
    }

    return muse::nidx;
}
