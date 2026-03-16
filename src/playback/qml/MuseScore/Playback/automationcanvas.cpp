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

#include "automationcanvas.h"
#include "log.h"

using namespace mu::playback;

AutomationCanvas::AutomationCanvas(QQuickItem* parent)
    :  QCanvasPainterItem(parent)
{
    // This is an overlay - make it transparent...
    setAlphaBlending(true);
    setFillColor(Qt::transparent);
}

QCanvasPainterItemRenderer* AutomationCanvas::createItemRenderer() const
{
    return new AutomationCanvasRenderer();
}

QVariant AutomationCanvas::viewMatrix() const
{
    return m_viewMatrix;
}

void AutomationCanvas::setViewMatrix(const QVariant& matrix)
{
    if (m_viewMatrix == matrix) {
        return;
    }
    m_viewMatrix = matrix.value<QTransform>();
    emit viewMatrixChanged();
    update();
}

void AutomationCanvasRenderer::synchronize(QCanvasPainterItem* item)
{
    //! NOTE: Important (from Qt reference) - "This method is the only place where it is safe for the painter
    //! and the item to read and write each others variables". So, if something relevant to the painter
    //! changes on the item, call QQuickItem::update to trigger this method and synchronize...
    AutomationCanvas* test = static_cast<AutomationCanvas*>(item);
    IF_ASSERT_FAILED(test) {
        return;
    }
    m_viewMatrix = test->m_viewMatrix;
}

void AutomationCanvasRenderer::paint(QCanvasPainter* p)
{
    // TODO: Placeholder - demonstrates a rect moving with the canvas...
    const QRect r(0, 0, 160, 160);
    p->setFillStyle(Qt::red);
    p->fillRect(m_viewMatrix.mapRect(r));
}
