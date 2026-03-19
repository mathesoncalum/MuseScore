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

#include "uicomponents/qml/Muse/UiComponents/quickpaintedview.h"

namespace muse::draw {
class Painter;
}

namespace mu::playback {
class AutomationOverlay : public muse::uicomponents::QuickPaintedView
{
    Q_OBJECT
    Q_PROPERTY(QVariant viewMatrix READ viewMatrix WRITE setViewMatrix NOTIFY viewMatrixChanged)
    QML_ELEMENT

public:
    explicit AutomationOverlay(QQuickItem* parent = nullptr);

protected:
    void paint(QPainter* painter) override;

    void mousePressEvent(QMouseEvent* event) override;

    QVariant viewMatrix() const;
    void setViewMatrix(const QVariant& matrix);

signals:
    void viewMatrixChanged();

private:
    struct Breakpoint {
        QPointF center;
        bool isExponential = false;
        bool operator==(const Breakpoint& o) const { return this->center == o.center; }
    };

    struct AutomationLane {
        int laneY;
        int laneHeight;
        std::vector<Breakpoint> points;
    };

    void init(const QVariantList& automationData);

    void paintLane(muse::draw::Painter* painter, const AutomationLane& lane);
    void paintBreakpoint(muse::draw::Painter* painter, const Breakpoint& breakpoint);
    void paintLine(muse::draw::Painter* painter, const QPointF& start, const QPointF& end, bool isExponential);

    QTransform m_viewMatrix;

    std::vector<AutomationLane> m_lanes;
    Breakpoint m_editedPoint;
};
}
