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

#include "abstractselectionfiltermodel.h"

namespace mu::notation {
class NotesInChordSelectionFilterModel : public AbstractSelectionFilterModel
{
    Q_OBJECT

    Q_PROPERTY(bool includeSingleNotes READ includeSingleNotes WRITE setIncludeSingleNotes NOTIFY includeSingleNotesChanged)

public:
    explicit NotesInChordSelectionFilterModel(QObject* parent = nullptr);

protected:
    void loadTypes() override;

    SelectionFilterTypesVariant getAll() const override { return engraving::NotesInChordSelectionFilterTypes::ALL; }
    SelectionFilterTypesVariant getNone() const override { return engraving::NotesInChordSelectionFilterTypes::NONE; }

    bool isAllowed(const SelectionFilterTypesVariant&) const override;
    QString titleForType(const SelectionFilterTypesVariant& variant) const override;

    bool includeSingleNotes() const;
    void setIncludeSingleNotes(bool include);

    void onSelectionChanged() override;

signals:
    void includeSingleNotesChanged();

private:
    void updateTopNoteIdx();
    size_t m_topNoteIdx = muse::nidx; // cached - calculating this isn't free
};
}
