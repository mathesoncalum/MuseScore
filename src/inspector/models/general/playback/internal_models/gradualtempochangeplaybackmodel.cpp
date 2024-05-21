/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited
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

#include "gradualtempochangeplaybackmodel.h"

#include "engraving/types/types.h"
#include "dom/gradualtempochange.h"

using namespace mu::inspector;
using namespace mu::engraving;

static const QString MIN_KEY("min");
static const QString MAX_KEY("max");

GradualTempoChangePlaybackModel::GradualTempoChangePlaybackModel(QObject* parent, IElementRepositoryService* repository)
    : AbstractInspectorModel(parent, repository, ElementType::GRADUAL_TEMPO_CHANGE)
{
    setTitle(muse::qtrc("inspector", "Tempo change"));
    setModelType(InspectorModelType::TYPE_GRADUAL_TEMPO_CHANGE);

    createProperties();
}

PropertyItem* GradualTempoChangePlaybackModel::tempoChangeFactor() const
{
    return m_tempoChangeFactor;
}

PropertyItem* GradualTempoChangePlaybackModel::tempoEasingMethod() const
{
    return m_tempoEasingMethod;
}

QVariantList GradualTempoChangePlaybackModel::possibleEasingMethods() const
{
    QVariantList methods {
        object(ChangeMethod::NORMAL, muse::qtrc("inspector", "Normal")),
        object(ChangeMethod::EASE_IN, muse::qtrc("inspector", "Ease in")),
        object(ChangeMethod::EASE_OUT, muse::qtrc("inspector", "Ease out"))
    };

    return methods;
}

QVariantMap GradualTempoChangePlaybackModel::tempoChangeFactorRange() const
{
    //! NOTE: See GradualTempoChange::minTempoChangeFactor
    // There may be multiple gradual tempo changes in our selection - ensure that we limit user input to the
    // "highest minimum" and "lowest maximum" factors to prevent tempos outwith our desired range.
    double currentMin = 0;
    double currentMax = std::numeric_limits<double>::max();
    for (const EngravingItem* element : m_elementList) {
        if (element->isGradualTempoChange()) {
            const GradualTempoChange* gtc = toGradualTempoChange(element);
            if (gtc->minTempoChangeFactor() > currentMin) {
                currentMin = gtc->minTempoChangeFactor();
            }
            if (gtc->maxTempoChangeFactor() < currentMax) {
                currentMax = gtc->maxTempoChangeFactor();
            }
        }
    }
    int min = static_cast<int>(muse::DataFormatter::roundDouble(currentMin * 100.0));
    int max = static_cast<int>(muse::DataFormatter::roundDouble(currentMax * 100.0));
    return QVariantMap { { MIN_KEY, min }, { MAX_KEY, max } };
}

void GradualTempoChangePlaybackModel::createProperties()
{
    m_tempoChangeFactor = buildPropertyItem(Pid::TEMPO_CHANGE_FACTOR, [this](const Pid pid, const QVariant& newValue) {
        onPropertyValueChanged(pid, newValue.toDouble() / 100);
    });

    m_tempoEasingMethod = buildPropertyItem(Pid::TEMPO_EASING_METHOD);
}

void GradualTempoChangePlaybackModel::loadProperties()
{
    loadPropertyItem(m_tempoChangeFactor, [](const QVariant& elementPropertyValue) -> QVariant {
        return static_cast<int>(muse::DataFormatter::roundDouble(elementPropertyValue.toDouble() * 100.0));
    });

    loadPropertyItem(m_tempoEasingMethod);
}

void GradualTempoChangePlaybackModel::resetProperties()
{
    m_tempoChangeFactor->resetToDefault();
    m_tempoEasingMethod->resetToDefault();
}
