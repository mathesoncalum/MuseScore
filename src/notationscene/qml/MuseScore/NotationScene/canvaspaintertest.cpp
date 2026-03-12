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

#include "canvaspaintertest.h"
#include "log.h"

using namespace mu::notation;

CanvasPainterTest::CanvasPainterTest(QQuickItem* parent)
    :  QCanvasPainterItem(parent)
{
    setAlphaBlending(true);
    setFillColor(Qt::transparent);
    m_label = QString("GPU accelerated moisture");
}

QCanvasPainterItemRenderer* CanvasPainterTest::createItemRenderer() const
{
    return new CanvasPainterTestRenderer();
}

void CanvasPainterTestRenderer::synchronize(QCanvasPainterItem* item)
{
    CanvasPainterTest* test = static_cast<CanvasPainterTest*>(item);
    m_label = test->m_label;
}

void CanvasPainterTestRenderer::paint(QCanvasPainter* p)
{
    float size = std::min(width(), height());
    QPointF center(width() * 0.5, height() * 0.5);

    p->setFillStyle(0x2CDE85);
    p->setTextAlign(QCanvasPainter::TextAlign::Center);
    p->setTextBaseline(QCanvasPainter::TextBaseline::Middle);
    QFont font("Titillium Web");
    font.setWeight(QFont::Weight::Thin);
    font.setPixelSize(size * 0.12);
    p->setFont(font);
    p->fillText(m_label, center.x(), center.y() - size * 0.16);
}
