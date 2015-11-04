/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "associationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram_scene/capabilities/intersectionable.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/arrowitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"

#include <QGraphicsScene>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QVector2D>
#include <QPair>
#include <qdebug.h>


namespace qmt {

AssociationItem::AssociationItem(DAssociation *association, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : RelationItem(association, diagramSceneModel, parent),
      m_association(association),
      m_endAName(0),
      m_endACardinality(0),
      m_endBName(0),
      m_endBCardinality(0)
{
}

AssociationItem::~AssociationItem()
{
}

void AssociationItem::update(const Style *style)
{
    RelationItem::update(style);

    updateEndLabels(m_association->getA(), m_association->getB(), &m_endAName, &m_endACardinality, style);
    updateEndLabels(m_association->getB(), m_association->getA(), &m_endBName, &m_endBCardinality, style);

    QMT_CHECK(m_arrow);
    QGraphicsItem *endAItem = m_diagramSceneModel->getGraphicsItem(m_association->getEndA());
    QMT_CHECK(endAItem);
    placeEndLabels(m_arrow->getFirstLineSegment(), m_endAName, m_endACardinality, endAItem, m_arrow->getStartHeadLength());
    QGraphicsItem *endBItem = m_diagramSceneModel->getGraphicsItem(m_association->getEndB());
    QMT_CHECK(endBItem);
    placeEndLabels(m_arrow->getLastLineSegment(), m_endBName, m_endBCardinality, endBItem, m_arrow->getEndHeadLength());
}

void AssociationItem::updateEndLabels(const DAssociationEnd &end, const DAssociationEnd &otherEnd, QGraphicsSimpleTextItem **endName, QGraphicsSimpleTextItem **endCardinality, const Style *style)
{
    Q_UNUSED(end);

    if (!otherEnd.getName().isEmpty()) {
        if (!*endName) {
            *endName = new QGraphicsSimpleTextItem(this);
        }
        (*endName)->setFont(style->getSmallFont());
        (*endName)->setBrush(style->getTextBrush());
        (*endName)->setText(otherEnd.getName());
    } else if (*endName) {
        (*endName)->scene()->removeItem(*endName);
        delete *endName;
        *endName = 0;
    }

    if (!otherEnd.getCardinality().isEmpty()) {
        if (!*endCardinality) {
            *endCardinality = new QGraphicsSimpleTextItem(this);
        }
        (*endCardinality)->setFont(style->getSmallFont());
        (*endCardinality)->setBrush(style->getTextBrush());
        (*endCardinality)->setText(otherEnd.getCardinality());
    } else if (*endCardinality) {
        (*endCardinality)->scene()->removeItem(*endCardinality);
        delete *endCardinality;
        *endCardinality = 0;
    }
}

void AssociationItem::placeEndLabels(const QLineF &lineSegment, QGraphicsItem *endName, QGraphicsItem *endCardinality, QGraphicsItem *endItem, double headLength)
{
    const double HEAD_OFFSET = headLength + 6.0;
    const double SIDE_OFFSET = 4.0;
    QPointF headOffset = QPointF(HEAD_OFFSET, 0);
    QPointF sideOffset = QPointF(0.0, SIDE_OFFSET);

    double angle = GeometryUtilities::calcAngle(lineSegment);
    if (angle >= -5 && angle <= 5) {
        if (endName) {
            endName->setPos(lineSegment.p1() + headOffset + sideOffset);
        }
        if (endCardinality) {
            endCardinality->setPos(lineSegment.p1() + headOffset - sideOffset - endCardinality->boundingRect().bottomLeft());
        }
    } else if (angle <= -175 || angle >= 175) {
        if (endName) {
            endName->setPos(lineSegment.p1() - headOffset + sideOffset - endName->boundingRect().topRight());
        }
        if (endCardinality) {
            endCardinality->setPos(lineSegment.p1() - headOffset - sideOffset - endCardinality->boundingRect().bottomRight());
        }
    } else {
        QRectF rect;
        if (endCardinality) {
            rect = endCardinality->boundingRect();
        }
        if (endName) {
            rect = rect.united(endName->boundingRect().translated(rect.bottomLeft()));
        }

        QPointF rectPlacement;
        GeometryUtilities::Side alignedSide = GeometryUtilities::SIDE_UNSPECIFIED;

        if (IIntersectionable *objectItem = dynamic_cast<IIntersectionable *>(endItem)) {
            QPointF intersectionPoint;
            QLineF intersectionLine;

            if (objectItem->intersectShapeWithLine(GeometryUtilities::stretch(lineSegment.translated(pos()), 2.0, 0.0), &intersectionPoint, &intersectionLine)) {
                if (!GeometryUtilities::placeRectAtLine(rect, lineSegment, HEAD_OFFSET, SIDE_OFFSET, intersectionLine, &rectPlacement, &alignedSide)) {
                    rectPlacement = intersectionPoint;
                }
            } else {
                rectPlacement = lineSegment.p1();
            }
        } else {
            rectPlacement = endItem->pos();
        }

        if (endCardinality) {
            if (alignedSide == GeometryUtilities::SIDE_RIGHT) {
                endCardinality->setPos(rectPlacement + QPointF(rect.width() - endCardinality->boundingRect().width(), 0.0));
            } else {
                endCardinality->setPos(rectPlacement);
            }
            rectPlacement += endCardinality->boundingRect().bottomLeft();
        }
        if (endName) {
            if (alignedSide == GeometryUtilities::SIDE_RIGHT) {
                endName->setPos(rectPlacement + QPointF(rect.width() - endName->boundingRect().width(), 0.0));
            } else {
                endName->setPos(rectPlacement);
            }
        }
    }
}

}
