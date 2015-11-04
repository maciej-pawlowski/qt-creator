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

#ifndef QMT_ANNOTATIONITEM_H
#define QMT_ANNOTATIONITEM_H

#include <QGraphicsItem>

#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/editable.h"


namespace qmt {

class DAnnotation;
class DiagramSceneModel;
class RectangularSelectionItem;
class Style;


class AnnotationItem :
        public QGraphicsItem,
        public IResizable,
        public IMoveable,
        public ISelectable,
        public IEditable
{
    class AnnotationTextItem;

public:

    AnnotationItem(DAnnotation *annotation, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = 0);

    ~AnnotationItem();

public:

    DAnnotation *getAnnotation() const { return m_annotation; }

    DiagramSceneModel *getDiagramSceneModel() const { return m_diagramSceneModel; }

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public:

    virtual void update();

public:

    QPointF getPos() const;

    QRectF getRect() const;

    QSizeF getMinimumSize() const;

    void setPosAndRect(const QPointF &originalPos, const QRectF &originalRect, const QPointF &topLeftDelta, const QPointF &bottomRightDelta);

    void alignItemSizeToRaster(Side adjustHorizontalSide, Side adjustVerticalSide, double rasterWidth, double rasterHeight);

public:

    void moveDelta(const QPointF &delta);

    void alignItemPositionToRaster(double rasterWidth, double rasterHeight);

public:

    bool isSecondarySelected() const;

    void setSecondarySelected(bool secondarySelected);

    bool isFocusSelected() const;

    void setFocusSelected(bool focusSelected);

public:

    bool isEditable() const;

    void edit();

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

protected:

    void updateSelectionMarker();

    void updateSelectionMarkerGeometry(const QRectF &annotationRect);

    const Style *getAdaptedStyle();

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

private slots:

    void onContentsChanged();

private:

    QSizeF calcMinimumGeometry() const;

    void updateGeometry();

private:

    DAnnotation *m_annotation;

    DiagramSceneModel *m_diagramSceneModel;

    bool m_secondarySelected;

    bool m_focusSelected;

    RectangularSelectionItem *m_selectionMarker;

    QGraphicsRectItem *m_noTextItem;

    AnnotationTextItem *m_textItem;

    bool m_onUpdate;

    bool m_onChanged;
};

}

#endif // QMT_ANNOTATIONITEM_H
