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

#include <QCanvasPainterItemRenderer>

namespace mu::playback {
class AutomationCanvas : public QCanvasPainterItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant viewMatrix READ viewMatrix WRITE setViewMatrix NOTIFY viewMatrixChanged)
    QML_ELEMENT
public:
    AutomationCanvas(QQuickItem* parent = nullptr);
    QCanvasPainterItemRenderer* createItemRenderer() const override;

    QVariant viewMatrix() const;
    void setViewMatrix(const QVariant& matrix);

signals:
    void viewMatrixChanged();

private:
    friend class AutomationCanvasRenderer;

    QTransform m_viewMatrix;
};

class AutomationCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
    void synchronize(QCanvasPainterItem* item) override;
    void paint(QCanvasPainter* p) override;

private:
    QTransform m_viewMatrix;
};
}
