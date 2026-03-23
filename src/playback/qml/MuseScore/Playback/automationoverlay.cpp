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

#include "automationoverlay.h"
#include "draw/painter.h"

#include "log.h"

using namespace mu::playback;

// TODO: These are all placeholders
static int LANE_HEIGHT = 500;
static int PEN_STROKE = 15;
static int DEFAULT_POINT_RADIUS = 40;
static double SELECTED_SIZE_FACTOR = 1.5;

AutomationOverlay::AutomationOverlay(QQuickItem* parent)
    : muse::uicomponents::QuickPaintedView(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);

    QVariantList automationData;

    QVariantMap lane1;

    QVariantMap bp1;
    bp1["x"] = 100;
    bp1["value"] = 0.66; // always between 0 and 1

    QVariantMap bp2;
    bp2["x"] = 500;
    bp2["value"] = 1;

    QVariantMap bp3;
    bp3["x"] = 600;
    bp3["value"] = 0;

    QVariantMap bp4;
    bp4["x"] = 2000;
    bp4["value"] = 0.25;

    QVariantList pointsList;
    pointsList.emplaceBack(bp1);
    pointsList.emplaceBack(bp2);
    pointsList.emplaceBack(bp3);
    pointsList.emplaceBack(bp4);

    lane1["y"] = 200;
    lane1["pointsList"] = pointsList;

    automationData.emplaceBack(lane1);

    init(automationData);
}

void AutomationOverlay::init(const QVariantList& automationData)
{
    m_lanes.clear();
    m_lanes.reserve(automationData.size());

    // TODO: not this
    m_editedPoint = { 300, 300 };

    for (const QVariant& laneVariant : automationData) {
        const QVariantMap& laneMap = laneVariant.toMap();
        IF_ASSERT_FAILED(!laneMap.isEmpty()) {
            return;
        }
        bool ok = true;
        const int laneTopY = laneMap.value("y").toInt(&ok);
        const QVariantList pointsList = laneMap.value("pointsList").toList();
        IF_ASSERT_FAILED(ok) {
            continue;
        }
        AutomationLane lane;
        lane.laneY = laneTopY;
        lane.laneHeight = LANE_HEIGHT; // TODO: Placeholder
        lane.points.reserve(pointsList.size());
        for (const QVariant& point : pointsList) {
            const QVariantMap pointMap = point.toMap();
            IF_ASSERT_FAILED(!pointMap.empty()) {
                continue;
            }
            const int pointX = pointMap.value("x").toInt(&ok);
            const double pointValue = ok ? pointMap.value("value").toDouble(&ok) : 0.0;
            IF_ASSERT_FAILED(ok) {
                continue;
            }
            const bool isExponential = pointMap.value("isExponential").toBool();

            //! NOTE: A value of 0 places the point at the bottom of the automation lane, a value
            //! of 1 places the point at the top of the automation lane...
            const int pointY = laneTopY + (1 - pointValue) * LANE_HEIGHT;
            const Breakpoint p = { QPointF(pointX, pointY), isExponential };
            lane.points.emplace_back(p);
        }
        m_lanes.emplace_back(lane);
    }
}

void AutomationOverlay::paint(QPainter* qp)
{
    // TODO: correctDrawRect (?)
    muse::draw::Painter mup(qp, objectName().toStdString());
    muse::draw::Painter* painter = &mup;

    for (size_t laneIndex = 0; laneIndex < m_lanes.size(); ++laneIndex) {
        const AutomationLane& lane = m_lanes.at(laneIndex);
        paintLane(painter, lane, laneIndex);
    }
}

void AutomationOverlay::paintLane(muse::draw::Painter* painter, const AutomationLane& lane, size_t laneIndex)
{
    // DBG ----
    const QRectF topLine = m_viewMatrix.mapRect(QRectF(0, lane.laneY, 10000, 5));
    const QRectF botLine = m_viewMatrix.mapRect(QRectF(0, lane.laneY + lane.laneHeight, 10000, 5));
    painter->fillRect(muse::RectF::fromQRectF(topLine), muse::Color::BLUE);
    painter->fillRect(muse::RectF::fromQRectF(botLine), muse::Color::BLUE);
    // --------

    //! NOTE: Draw the actual breakpoints later (on top of the lines)...
    Breakpoint lastBreakpoint;
    for (const Breakpoint& breakpoint : lane.points) {
        if (!lastBreakpoint.center.isNull()) {
            paintLine(painter, lastBreakpoint.center, breakpoint.center, lastBreakpoint.isExponential);
        }
        lastBreakpoint = breakpoint;
    }

    for (size_t pointIndex = 0; pointIndex < lane.points.size(); ++pointIndex) {
        const Breakpoint& breakpoint = lane.points.at(pointIndex);
        const bool selected = m_editedPoint.first == laneIndex && m_editedPoint.second == pointIndex;
        paintBreakpoint(painter, breakpoint, selected);
    }
}

void AutomationOverlay::paintBreakpoint(muse::draw::Painter* painter, const Breakpoint& breakpoint, bool selected)
{
    const double radius = selected ? DEFAULT_POINT_RADIUS * SELECTED_SIZE_FACTOR : DEFAULT_POINT_RADIUS;

    const QRectF boundingRect(breakpoint.center.x() - radius,
                              breakpoint.center.y() - radius,
                              2 * radius,
                              2 * radius);

    const muse::RectF mappedRect = muse::RectF::fromQRectF(m_viewMatrix.mapRect(boundingRect));
    const double scaledStroke = PEN_STROKE * m_viewMatrix.m11(); //! NOTE: using horizontal scaling with m11

    // Draw background
    muse::draw::PainterPath path;
    path.addEllipse(mappedRect);
    painter->fillPath(path, muse::Color::WHITE);

    if (selected) {
        // Draw selection indicator
        const double scaledInnerRadius = radius - (PEN_STROKE * 1.5);

        const QRectF innerBoundingRect(breakpoint.center.x() - scaledInnerRadius,
                                       breakpoint.center.y() - scaledInnerRadius,
                                       2 * scaledInnerRadius,
                                       2 * scaledInnerRadius);

        muse::draw::PainterPath path2;
        const muse::RectF mappedInnerRect = muse::RectF::fromQRectF(m_viewMatrix.mapRect(innerBoundingRect));
        path2.addEllipse(mappedInnerRect);
        painter->fillPath(path2, muse::Color::BLUE);
    }

    // Draw border
    const muse::draw::Pen pen(muse::Color::BLACK, scaledStroke);
    painter->setPen(pen);
    painter->drawEllipse(mappedRect);
}

void AutomationOverlay::paintLine(muse::draw::Painter* painter, const QPointF& start, const QPointF& end, bool /*isExponential*/)
{
    const double scaledStroke = PEN_STROKE * m_viewMatrix.m11();

    const muse::draw::Pen pen(muse::Color::BLACK, scaledStroke);
    painter->setPen(pen);

    const QPointF mappedStart = m_viewMatrix.map(start);
    const QPointF mappedEnd = m_viewMatrix.map(end);

    painter->drawLine(muse::PointF::fromQPointF(mappedStart), muse::PointF::fromQPointF(mappedEnd));
}

void AutomationOverlay::mousePressEvent(QMouseEvent* event)
{
    const QPointF mappedPos = m_viewMatrix.inverted().map(event->pos());
    for (size_t laneIndex = 0; laneIndex < m_lanes.size(); ++laneIndex) { // TODO: Don't use brute force...
        const AutomationLane& lane = m_lanes.at(laneIndex);
        for (size_t pointIndex = 0; pointIndex < lane.points.size(); ++pointIndex) {
            const Breakpoint& point = lane.points.at(pointIndex);
            const QPointF delta = mappedPos - point.center;
            const double distanceSquared = QPointF::dotProduct(delta, delta);
            if (distanceSquared <= DEFAULT_POINT_RADIUS * DEFAULT_POINT_RADIUS) {
                m_editedPoint = { laneIndex, pointIndex };
                update();
                event->accept();
                return;
            }
        }
    }
    event->accept();
}

void AutomationOverlay::mouseMoveEvent(QMouseEvent* event)
{
    // TODO: hack, make it optional, or a struct, or both...
    if (m_editedPoint.first >= 100 && m_editedPoint.second >= 100) {
        return;
    }
    const QPointF mappedPos = m_viewMatrix.inverted().map(event->pos());
    AutomationLane& lane = m_lanes.at(m_editedPoint.first);
    Breakpoint& point = lane.points.at(m_editedPoint.second);
    point.center = mappedPos;
    update();
}

void AutomationOverlay::mouseReleaseEvent(QMouseEvent*)
{
    // TODO: hack, make it optional, or a struct, or both...
    if (m_editedPoint.first <= 100 && m_editedPoint.second <= 100) {
        m_editedPoint = { 300, 300 };
        update();
    }
}

QVariant AutomationOverlay::viewMatrix() const
{
    return m_viewMatrix;
}

void AutomationOverlay::setViewMatrix(const QVariant& matrix)
{
    if (m_viewMatrix == matrix) {
        return;
    }
    m_viewMatrix = matrix.value<QTransform>();
    emit viewMatrixChanged();
    update(); // TODO: pass in an update rect
}
