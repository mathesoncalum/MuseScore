/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
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

#include <QQmlEngine>

#include "context/iglobalcontext.h"

namespace muse::uicomponents {
class Polyline;
}

namespace mu::playback {
class AutomationOverlay : public QQuickItem, public muse::Contextable
{
    Q_OBJECT
    Q_PROPERTY(QVariant viewMatrix READ viewMatrix WRITE setViewMatrix NOTIFY viewMatrixChanged)
    QML_ELEMENT

    muse::ContextInject<mu::context::IGlobalContext> globalContext = { this };

public:
    explicit AutomationOverlay(QQuickItem* parent = nullptr);
    Q_INVOKABLE void initAutomationLinesData(const QVariant& automationLinesData);

protected:
    void classBegin() override;

    QVariant viewMatrix() const;
    void setViewMatrix(const QVariant& matrix);

signals:
    void viewMatrixChanged();

private:
    QVariantList dummyAutomationLinesData() const;

    struct AutomationLineData {
        QVariantMap rawLineDataMap;
        muse::uicomponents::Polyline* polyline = nullptr;
    };

    bool lineIndexIsValid(size_t index) const;

    void updatePolylinesGeometry(size_t firstIndex, size_t lastIndex);
    void updateAllPolylinesGeometry() { updatePolylinesGeometry(0, m_automationLinesData.size() - 1); }

    QTransform m_viewMatrix;
    std::vector<AutomationLineData> m_automationLinesData;
};
}
