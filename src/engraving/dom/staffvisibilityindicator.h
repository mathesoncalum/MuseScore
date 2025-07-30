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

#include "engravingitem.h"

namespace mu::engraving {
class StaffVisibilityIndicator : public EngravingItem
{
    OBJECT_ALLOCATOR(engraving, StaffVisibilityIndicator)
    DECLARE_CLASSOF(ElementType::STAFF_VISIBILITY_INDICATOR)

public:
    StaffVisibilityIndicator(System* parent);
    StaffVisibilityIndicator* clone() const override { return new StaffVisibilityIndicator(*this); }

    const System* system() const { return toSystem(explicitParent()); }

    struct LayoutData : public EngravingItem::LayoutData {
    };
    DECLARE_LAYOUTDATA_METHODS(StaffVisibilityIndicator)

    muse::draw::Font font() const;

    void setSelected(bool v) override;

    char16_t iconCode() const { return 0xEF53; } // EYE_OPEN

    void undoChangeProperty(Pid, const PropertyValue&, PropertyFlags) override { return; } // not editable
};
} // namespace mu::engraving
