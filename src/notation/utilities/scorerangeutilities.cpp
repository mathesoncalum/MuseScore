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
        if (!section.startSegment || !section.endSegment) {
            // This is a "Box only" section...
            IF_ASSERT_FAILED(section.startBox && section.endBox) {
                continue;
            }
            const RectF& startRect = section.startBox->canvasBoundingRect();
            const RectF& endRect = section.endBox->canvasBoundingRect();
            result.push_back(startRect.united(endRect));
            continue;
        }

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

    const auto boxAtSegment = [score](const Segment* seg, bool first) -> const Box* {
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

    // The range's start/end Box can exist in a section which the segment loop below never visits (e.g. a VBox base, a range
    // that starts with a Box at the end of a system, or a range that ends at a Box at the start of a system). This lambda
    // constructs range sections for these cases...
    const auto addBoxOnlySection = [&sections, score, startBox, endBox](const Box* box) {
        const System* system = box->system();
        if (!system) {
            return;
        }

        const Segment* seg = score->tick2segment(box->tick());
        if (!seg) {
            return;
        }

        const Box* leftBox = box;
        if (leftBox != startBox) {
            // Extend left from Box to start of system or to startBox (whichever comes first)...
            const MeasureBase* prevMB = leftBox->prevMM();
            while (prevMB && prevMB->isBox() && prevMB->system() == system) {
                leftBox = toBox(prevMB);
                if (leftBox == startBox) {
                    break;
                }
                prevMB = leftBox->prevMM();
            }
        }

        const Box* rightBox = box;
        if (rightBox != endBox) {
            // Extend right from Box to end of system or to endBox (whichever comes first)...
            const MeasureBase* nextMB = box->nextMM();
            while (nextMB && nextMB->isBox() && nextMB->system() == system) {
                rightBox = toBox(nextMB);
                if (rightBox == endBox) {
                    break;
                }
                nextMB = rightBox->nextMM();
            }
        }

        RangeSection section;
        section.system = system;
        section.startBox = leftBox;
        section.endBox = rightBox;

        sections.push_back(section);
    };

    if (startBox && startBox->system() != rangeStartSegment->system()) {
        addBoxOnlySection(startBox);
        currentStartBox = boxAtSegment(startSegment, /*first*/ true);
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
                             : boxAtSegment(segment, /*first*/ false);
            sections.push_back(section);

            startSegment = nextSegment;

            if (nextIsNewSystem) {
                // next segment is in a new system - look for a Box at start...
                currentStartBox = boxAtSegment(nextSegment, /*first*/ true);
            }
        }

        segment = nextSegment;
    }

    // sections.back() is the last section we added using segments - if endBox isn't on the same
    // system then it's a "box only section"...
    if (endBox && !sections.empty() && endBox->system() != sections.back().system) {
        addBoxOnlySection(endBox);
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
