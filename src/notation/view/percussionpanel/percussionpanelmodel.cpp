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

#include "percussionpanelmodel.h"
#include "types/translatablestring.h"
#include "ui/view/iconcodes.h"

#include "engraving/dom/factory.h"
#include "engraving/dom/undo.h"

#include "defer.h"

static const QString INSTRUMENT_NAMES_CODE("percussion-instrument-names");
static const QString NOTATION_PREVIEW_CODE("percussion-notation-preview");
static const QString EDIT_LAYOUT_CODE("percussion-edit-layout");
static const QString RESET_LAYOUT_CODE("percussion-reset-layout");

using namespace muse;
using namespace ui;
using namespace mu::notation;

PercussionPanelModel::PercussionPanelModel(QObject* parent)
    : QObject(parent)
{
    m_padListModel = new PercussionPanelPadListModel(this);
}

bool PercussionPanelModel::enabled() const
{
    return m_enabled;
}

void PercussionPanelModel::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged();
}

PanelMode::Mode PercussionPanelModel::currentPanelMode() const
{
    return m_currentPanelMode;
}

void PercussionPanelModel::setCurrentPanelMode(const PanelMode::Mode& panelMode, bool updateNoteInput)
{
    if (m_currentPanelMode == panelMode) {
        return;
    }

    // After editing, return to the last non-edit mode...
    if (panelMode != PanelMode::Mode::EDIT_LAYOUT && m_panelModeToRestore != panelMode) {
        m_panelModeToRestore = panelMode;
    }

    m_currentPanelMode = panelMode;
    emit currentPanelModeChanged(m_currentPanelMode);

    if (!updateNoteInput || !interaction() || !interaction()->noteInput()) {
        return;
    }

    const INotationNoteInputPtr noteInput = interaction()->noteInput();
    panelMode == PanelMode::Mode::WRITE ? noteInput->startNoteInput() : noteInput->endNoteInput();
}

bool PercussionPanelModel::useNotationPreview() const
{
    return m_useNotationPreview;
}

void PercussionPanelModel::setUseNotationPreview(bool useNotationPreview)
{
    if (m_useNotationPreview == useNotationPreview) {
        return;
    }

    m_useNotationPreview = useNotationPreview;
    emit useNotationPreviewChanged(m_useNotationPreview);
}

PercussionPanelPadListModel* PercussionPanelModel::padListModel() const
{
    return m_padListModel;
}

void PercussionPanelModel::init()
{
    setUpConnections();

    globalContext()->currentNotationChanged().onNotify(this, [this] {
        setUpConnections();
    });
}

QList<QVariantMap> PercussionPanelModel::layoutMenuItems() const
{
    const TranslatableString instrumentNamesTitle("notation", "Instrument names");
    // Using IconCode for this instead of "checked" because we want the tick to display on the left
    const int instrumentNamesIcon = static_cast<int>(m_useNotationPreview ? IconCode::Code::NONE : IconCode::Code::TICK_RIGHT_ANGLE);

    const TranslatableString notationPreviewTitle("notation", "Notation preview");
    // Using IconCode for this instead of "checked" because we want the tick to display on the left
    const int notationPreviewIcon = static_cast<int>(m_useNotationPreview ? IconCode::Code::TICK_RIGHT_ANGLE : IconCode::Code::NONE);

    const TranslatableString editLayoutTitle = m_currentPanelMode == PanelMode::Mode::EDIT_LAYOUT
                                               ? TranslatableString("notation", "Finish editing")
                                               : TranslatableString("notation", "Edit layout");
    const int editLayoutIcon = static_cast<int>(IconCode::Code::CONFIGURE);

    const TranslatableString resetLayoutTitle("notation", "Reset layout");
    const int resetLayoutIcon = static_cast<int>(IconCode::Code::UNDO);

    QList<QVariantMap> menuItems = {
        { { "id", INSTRUMENT_NAMES_CODE },
            { "title", instrumentNamesTitle.qTranslated() }, { "icon", instrumentNamesIcon }, { "enabled", true } },

        { { "id", NOTATION_PREVIEW_CODE },
            { "title", notationPreviewTitle.qTranslated() }, { "icon", notationPreviewIcon }, { "enabled", true } },

        { }, // separator

        { { "id", EDIT_LAYOUT_CODE },
            { "title", editLayoutTitle.qTranslated() }, { "icon", editLayoutIcon }, { "enabled", true } },

        { { "id", RESET_LAYOUT_CODE },
            { "title", resetLayoutTitle.qTranslated() }, { "icon", resetLayoutIcon }, { "enabled", true } },
    };

    return menuItems;
}

void PercussionPanelModel::handleMenuItem(const QString& itemId)
{
    if (itemId == INSTRUMENT_NAMES_CODE) {
        setUseNotationPreview(false);
    } else if (itemId == NOTATION_PREVIEW_CODE) {
        setUseNotationPreview(true);
    } else if (itemId == EDIT_LAYOUT_CODE) {
        const bool currentlyEditing = m_currentPanelMode == PanelMode::Mode::EDIT_LAYOUT;
        currentlyEditing ? finishEditing() : setCurrentPanelMode(PanelMode::Mode::EDIT_LAYOUT, false);
    } else if (itemId == RESET_LAYOUT_CODE) {
        resetLayout();
    }
}

void PercussionPanelModel::finishEditing()
{
    Drumset* updatedDrumset = m_padListModel->drumset();
    m_padListModel->removeEmptyRows();

    NoteInputState inputState = interaction()->noteInput()->state();
    const Staff* staff = inputState.staff;

    IF_ASSERT_FAILED(staff && staff->part()) {
        return;
    }

    Instrument* inst = staff->part()->instrument(inputState.segment->tick());

    IF_ASSERT_FAILED(inst) {
        return;
    }

    // Return if nothing changed after edit...
    if (inst->drumset() && updatedDrumset
        && *inst->drumset() == *updatedDrumset) {
        return;
    }

    for (int i = 0; i < m_padListModel->padList().size(); ++i) {
        const PercussionPanelPadModel* model = m_padListModel->padList().at(i);
        if (!model) {
            continue;
        }
        const int row = i / m_padListModel->numColumns();
        const int column = i % m_padListModel->numColumns();
        engraving::DrumInstrument& drum = updatedDrumset->drum(model->pitch());
        drum.panelRow = row;
        drum.panelColumn = column;
    }

    INotationUndoStackPtr undoStack = notation()->undoStack();

    DEFER {
        undoStack->commitChanges();
    };

    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Edit percussion panel layout"));
    score()->undo(new engraving::ChangeDrumset(inst, updatedDrumset, staff->part()));

    setCurrentPanelMode(m_panelModeToRestore, false);
}

void PercussionPanelModel::customizeKit()
{
    dispatcher()->dispatch("customize-kit");
}

void PercussionPanelModel::setUpConnections()
{
    const auto updatePadModels = [this](Drumset* drumset) {
        if (drumset && m_padListModel->drumset() && *drumset == *m_padListModel->drumset()) {
            return;
        }

        if (m_currentPanelMode == PanelMode::Mode::EDIT_LAYOUT) {
            finishEditing();
        }

        m_padListModel->setDrumset(drumset);
    };

    if (!notation()) {
        updatePadModels(nullptr);
        return;
    }

    const INotationNoteInputPtr noteInput = interaction()->noteInput();
    updatePadModels(noteInput->state().drumset);
    setEnabled(m_padListModel->hasActivePads());

    noteInput->stateChanged().onNotify(this, [this, updatePadModels]() {
        if (!notation()) {
            updatePadModels(nullptr);
            return;
        }
        const INotationNoteInputPtr ni = interaction()->noteInput();
        updatePadModels(ni->state().drumset);
    });

    m_padListModel->hasActivePadsChanged().onNotify(this, [this]() {
        setEnabled(m_padListModel->hasActivePads());
    });

    m_padListModel->padTriggered().onReceive(this, [this](int pitch) {
        switch (currentPanelMode()) {
        case PanelMode::Mode::EDIT_LAYOUT: return;
        case PanelMode::Mode::WRITE: writePitch(pitch); // fall through
        case PanelMode::Mode::SOUND_PREVIEW: playPitch(pitch);
        }
    });
}

void PercussionPanelModel::writePitch(int pitch)
{
    INotationUndoStackPtr undoStack = notation()->undoStack();
    if (!interaction() || !undoStack) {
        return;
    }

    DEFER {
        undoStack->commitChanges();
    };

    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Enter percussion note"));

    interaction()->noteInput()->startNoteInput();

    score()->addMidiPitch(pitch, false, /*transpose*/ false);

    const mu::engraving::InputState& inputState = score()->inputState();
    if (inputState.cr()) {
        interaction()->showItem(inputState.cr());
    }
}

void PercussionPanelModel::playPitch(int pitch)
{
    const mu::engraving::InputState& inputState = score()->inputState();
    if (!inputState.cr()) {
        return;
    }

    Chord* chord = mu::engraving::Factory::createChord(inputState.lastSegment());
    chord->setParent(inputState.lastSegment());

    Note* note = mu::engraving::Factory::createNote(chord);
    note->setParent(chord);

    note->setStaffIdx(mu::engraving::track2staff(inputState.cr()->track()));

    const mu::engraving::NoteVal nval = score()->noteVal(pitch, /*transpose*/ false);
    note->setNval(nval);

    playbackController()->playElements({ note });

    note->setParent(nullptr);
    delete note;
}

void PercussionPanelModel::resetLayout()
{
    if (m_currentPanelMode == PanelMode::Mode::EDIT_LAYOUT) {
        // TODO: There's a minor technical limitation here. There are competing requirements where changing the drumset "naturally" (e.g. by
        // moving the selection to another staff) *should* cause us to finish editing, but we *shouldn't* finish editing when the drumset is changed
        // through a layout reset. At the moment we don't have any way of distinguishing between these two cases inside updatePadModels, so we
        // always finish editing. The solution is probably to avoid using ChangeDrumset, and instead introduce a more granular UndoCommand...
        finishEditing();
    }

    NoteInputState inputState = interaction()->noteInput()->state();
    const Staff* staff = inputState.staff;

    IF_ASSERT_FAILED(staff && staff->part()) {
        return;
    }

    Instrument* inst = staff->part()->instrument(inputState.segment->tick());

    IF_ASSERT_FAILED(inst) {
        return;
    }

    const InstrumentTemplate instTemplate = instrumentsRepository()->instrumentTemplate(inst->id());
    const Drumset* drumsetTemplate = instTemplate.drumset;

    IF_ASSERT_FAILED(drumsetTemplate) {
        return;
    }

    Drumset tempDrumset = *m_padListModel->drumset();

    int highestIndex = -1;
    QList<int /*pitch*/> noTemplateFound;
    for (int pitch = 0; pitch < mu::engraving::DRUM_INSTRUMENTS; ++pitch) {
        if (!tempDrumset.isValid(pitch)) {
            // We aren't currently using this pitch, doesn't matter if it's valid in the template..
            continue;
        }
        //! NOTE: Pitch + drum name isn't exactly the most robust identifier, but this will change with the new percussion ID system
        if (!drumsetTemplate->isValid(pitch) || tempDrumset.name(pitch) != drumsetTemplate->name(pitch)) {
            // Drum is valid, but we can't find a template for it. We'll set the position chromatically once we know the rest of the layout...
            noTemplateFound.emplaceBack(pitch);
            continue;
        }

        const int templateRow = drumsetTemplate->drum(pitch).panelRow;
        const int templateColumn = drumsetTemplate->drum(pitch).panelColumn;

        tempDrumset.drum(pitch).panelRow = templateRow;
        tempDrumset.drum(pitch).panelColumn = templateColumn;

        const int modelIndex = templateRow * m_padListModel->numColumns() + templateColumn;

        if (modelIndex > highestIndex) {
            highestIndex = modelIndex;
        }
    }

    for (int pitch : noTemplateFound) {
        ++highestIndex;
        tempDrumset.drum(pitch).panelRow = highestIndex / m_padListModel->numColumns();
        tempDrumset.drum(pitch).panelColumn = highestIndex % m_padListModel->numColumns();
    }

    // Return if nothing changed after reset...
    if (tempDrumset == *m_padListModel->drumset()) {
        return;
    }

    INotationUndoStackPtr undoStack = notation()->undoStack();

    DEFER {
        undoStack->commitChanges();
    };

    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Reset percussion panel layout"));
    score()->undo(new engraving::ChangeDrumset(inst, &tempDrumset, staff->part()));
}

const INotationPtr PercussionPanelModel::notation() const
{
    return globalContext()->currentNotation();
}

const INotationInteractionPtr PercussionPanelModel::interaction() const
{
    return notation() ? notation()->interaction() : nullptr;
}

mu::engraving::Score* PercussionPanelModel::score() const
{
    return notation() ? notation()->elements()->msScore() : nullptr;
}
