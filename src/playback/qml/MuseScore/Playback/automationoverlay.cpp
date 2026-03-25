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

#include "async/notifylist.h"
#include "uicomponents/components/polyline.h"

using namespace mu::playback;

AutomationOverlay::AutomationOverlay(QQuickItem* parent)
    : QQuickItem(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
    setClip(true); // TODO: Not ideal - we'll still be rendering clipped points
}

void AutomationOverlay::classBegin()
{
    initAutomationLinesData(dummyAutomationLinesData());
}

void AutomationOverlay::initAutomationLinesData(const QVariant& automationLinesData)
{
    for (size_t i = 0; i < m_automationLinesData.size(); ++i) {
        delete m_automationLinesData.at(i).polyline;
    }
    m_automationLinesData.clear();

    const QVariantList automationDataList = automationLinesData.toList();
    IF_ASSERT_FAILED(!automationDataList.isEmpty()) {
        update();
        return;
    }

    m_automationLinesData.reserve(automationDataList.size());

    for (qsizetype i = 0; i < automationDataList.size(); ++i) {
        const QVariantMap& lineDataMap = automationDataList.at(i).toMap();
        IF_ASSERT_FAILED(!lineDataMap.isEmpty()) {
            continue;
        }

        const QVariantList pointsDataList = lineDataMap.value("points").toList();
        if (pointsDataList.size() < 2) { // TODO: Maybe it can have less than 2 points?
            continue;
        }

        bool ok = false;
        QVector<QPointF> pointsList;
        pointsList.reserve(pointsDataList.size());
        for (const QVariant& pointData : pointsDataList) {
            const QVariantMap& pointMap = pointData.toMap();
            IF_ASSERT_FAILED(!pointMap.isEmpty()) {
                continue;
            }
            const qreal pointX = pointMap.value("x").toReal(&ok);
            const qreal pointY = ok ? pointMap.value("y").toReal(&ok) : 0.0;
            IF_ASSERT_FAILED(ok) {
                return;
            }
            pointsList.emplaceBack(QPointF(pointX, pointY));
        }

        //! NOTE: Only set the points for now - we'll update the geometry separately...
        muse::uicomponents::Polyline* polyline = new muse::uicomponents::Polyline(this);
        polyline->setPoints(pointsList);

        AutomationLineData lineData = { lineDataMap, polyline };
        m_automationLinesData.emplace_back(lineData);
    }

    updateAllPolylinesGeometry();
    update();
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

    updateAllPolylinesGeometry();
    update();
}

bool AutomationOverlay::lineIndexIsValid(size_t index) const
{
    return index < m_automationLinesData.size();
}

void AutomationOverlay::updatePolylinesGeometry(size_t firstIndex, size_t lastIndex)
{
    IF_ASSERT_FAILED(lineIndexIsValid(firstIndex) && lineIndexIsValid(lastIndex)) {
        return;
    }

    //! NOTE: Polyline::setPoints doesn't need to be called in here. The points are always expressed
    //! locally (relative to the Polyline itself) so they don't need to be updated when something like
    //! the view matrix changes...
    for (size_t i = firstIndex; i <= lastIndex; ++i) {
        AutomationLineData& lineData = m_automationLinesData.at(i);
        const QVariantMap& rawMap = lineData.rawLineDataMap;
        muse::uicomponents::Polyline* polyline = lineData.polyline;
        IF_ASSERT_FAILED(!rawMap.isEmpty() && polyline) {
            continue;
        }

        bool ok = false;
        const qreal lineX = rawMap.value("x").toReal(&ok);
        const qreal lineY = ok ? rawMap.value("y").toReal(&ok) : 0.0;
        const qreal lineWidth = ok ? rawMap.value("width").toReal(&ok) : 0.0;
        const qreal lineHeight = ok ? rawMap.value("height").toReal(&ok) : 0.0;
        IF_ASSERT_FAILED(ok) {
            continue;
        }

        const QPointF topLeft(lineX, lineY);
        const QPointF topLeftMapped = m_viewMatrix.map(topLeft);
        polyline->setX(topLeftMapped.x());
        polyline->setY(topLeftMapped.y());
        polyline->setWidth(lineWidth * m_viewMatrix.m11());
        polyline->setHeight(lineHeight * m_viewMatrix.m22());
    }
}

QVariantList AutomationOverlay::dummyAutomationLinesData() const
{
    QVariantList dummyAutomationData;

    // TODO: use an abstraction between notation and this component? (i.e. don't do this in here)
    const notation::INotationPartsPtr notationParts = globalContext()->currentNotation()
                                                      ? globalContext()->currentNotation()->parts() : nullptr;

    if (!notationParts) {
        return QVariantList();
    }

    const notation::Part* partZero = notationParts->partList().at(0);
    if (!partZero) {
        return QVariantList();
    }

    // System one because zero is the title...
    const notation::System* systemOne = partZero->score() ? partZero->score()->systems().at(1) : nullptr;
    if (!systemOne) {
        return QVariantList();
    }

    const muse::PointF systemOnePos = systemOne->canvasPos();
    for (const notation::SysStaff* staff : systemOne->staves()) {
        QVariantMap lineData;

        const muse::RectF staffRect = staff->bbox().translated(systemOnePos);

        lineData["x"] = staffRect.x();
        lineData["y"] = staffRect.y();
        lineData["width"] = staffRect.width();
        lineData["height"] = staffRect.height();

        QVariantList points;

        QVariantMap point1;
        point1["x"] = 0.10;
        point1["y"] = 0.50;
        points << point1;

        QVariantMap point2;
        point2["x"] = 0.50;
        point2["y"] = 0.10;
        points << point2;

        QVariantMap point3;
        point3["x"] = 0.66;
        point3["y"] = 0.90;
        points << point3;

        lineData["points"] = points;

        dummyAutomationData << lineData;
    }

    return dummyAutomationData;
}
