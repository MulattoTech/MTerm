// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-22-native-ux; docs/ai/changes/2026-09-22-native-ux.json
#include "CanvasView.h"
#include "ResourceCard.h"
#include "ui/Theme.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSet>
#include <QWheelEvent>
#include <cmath>
namespace mterm {
class MiniMap final : public QWidget {
    CanvasView *view_;
    QRectF bounds() const {
        return view_->scene()
            ->itemsBoundingRect()
            .united(view_->mapToScene(view_->viewport()->rect()).boundingRect())
            .adjusted(-60, -60, 60, 60);
    }

  public:
    explicit MiniMap(CanvasView *v) : QWidget(v->viewport()), view_(v) {
        setObjectName("canvas-minimap");
        setFixedSize(148, 94);
        setToolTip("Workspace minimap â€” click to navigate");
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(ui::Border);
        p.setBrush(QColor("#101a2a"));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 9, 9);
        const auto b = bounds();
        if (b.isEmpty())
            return;
        const auto sx = (width() - 16) / b.width(), sy = (height() - 16) / b.height();
        auto mapped = [&](QRectF r) {
            return QRectF(8 + (r.x() - b.x()) * sx, 8 + (r.y() - b.y()) * sy, r.width() * sx,
                          r.height() * sy);
        };
        p.setPen(Qt::NoPen);
        for (auto *item : view_->scene()->items()) {
            p.setBrush(QColor("#42597e"));
            p.drawRoundedRect(mapped(item->sceneBoundingRect()), 1, 1);
        }
        p.setBrush(QColor(119, 161, 242, 20));
        p.setPen(QColor("#729be0"));
        p.drawRect(mapped(view_->mapToScene(view_->viewport()->rect()).boundingRect()));
    }
    void mousePressEvent(QMouseEvent *event) override {
        const auto b = bounds();
        view_->centerOn(b.x() + (event->position().x() - 8) / (width() - 16) * b.width(),
                        b.y() + (event->position().y() - 8) / (height() - 16) * b.height());
        update();
    }
};
CanvasView::CanvasView(QWidget *parent) : QGraphicsView(parent), scene_(this) {
    setObjectName("native-canvas");
    setScene(&scene_);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scene_.setSceneRect(-10000, -10000, 20000, 20000);
    setMinimumHeight(260);
    setMinimumWidth(320);
    controls_ = new QFrame(viewport());
    controls_->setObjectName("canvas-controls");
    auto *row = new QHBoxLayout(controls_);
    row->setContentsMargins(5, 5, 5, 5);
    row->setSpacing(3);
    auto *minus = new QPushButton("−", controls_), *plus = new QPushButton("+", controls_),
         *fit = new QPushButton("Fit", controls_);
    minus->setObjectName("canvas-zoom-out");
    plus->setObjectName("canvas-zoom-in");
    fit->setObjectName("canvas-fit");
    minus->setFixedWidth(33);
    plus->setFixedWidth(33);
    zoomLabel_ = new QLabel("100%", controls_);
    zoomLabel_->setAlignment(Qt::AlignCenter);
    zoomLabel_->setFixedWidth(47);
    row->addWidget(minus);
    row->addWidget(zoomLabel_);
    row->addWidget(plus);
    row->addWidget(fit);
    controls_->adjustSize();
    connect(minus, &QPushButton::clicked, this, [this] { zoomBy(0.85); });
    connect(plus, &QPushButton::clicked, this, [this] { zoomBy(1.18); });
    connect(fit, &QPushButton::clicked, this, &CanvasView::fitResources);
    mini_ = new MiniMap(this);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] { mini_->update(); });
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { mini_->update(); });
    placeOverlays();
}
void CanvasView::setCanvas(const QJsonObject &canvas, bool editable) {
    canvas_ = canvas;
    editable_ = editable;
    QSet<QString> present;
    for (const auto &value : canvas["nodes"].toArray()) {
        const auto node = value.toObject();
        const auto id = node["id"].toString();
        present.insert(id);
        if (auto *card = cards_.value(id, nullptr))
            card->setNode(node, editable);
        else {
            auto *created = new ResourceCard(node, editable);
            cards_[id] = created;
            scene_.addItem(created);
            connect(created, &ResourceCard::activated, this, &CanvasView::nodeActivated);
            connect(created, &ResourceCard::edited, this, [this] {
                mini_->update();
                emit layoutEdited();
            });
        }
    }
    for (const auto &id : cards_.keys())
        if (!present.contains(id))
            delete cards_.take(id);
    const auto v = canvas["viewport"].toObject();
    const auto zoom = qBound(0.15, v["zoom"].toDouble(1), 3.0);
    setTransform(QTransform::fromScale(zoom, zoom));
    centerOn((viewport()->width() / 2.0 - v["x"].toDouble()) / zoom,
             (viewport()->height() / 2.0 - v["y"].toDouble()) / zoom);
    changedViewport();
}
QJsonObject CanvasView::canvas() const {
    auto result = canvas_;
    auto nodes = result["nodes"].toArray();
    for (qsizetype i = 0; i < nodes.size(); ++i)
        if (auto *card = cards_.value(nodes[i].toObject()["id"].toString(), nullptr))
            nodes[i] = card->node();
    result["nodes"] = nodes;
    const auto origin = mapToScene(0, 0);
    const auto z = transform().m11();
    result["viewport"] = QJsonObject{{"x", -origin.x() * z}, {"y", -origin.y() * z}, {"zoom", z}};
    return result;
}
void CanvasView::changedViewport() {
    zoomLabel_->setText(QString::number(qRound(transform().m11() * 100)) + "%");
    mini_->update();
}
void CanvasView::zoomBy(qreal factor) {
    const auto old = transform().m11();
    const auto next = qBound(0.15, old * factor, 3.0);
    scale(next / old, next / old);
    changedViewport();
    if (editable_)
        emit layoutEdited();
}
void CanvasView::fitResources() {
    if (scene_.items().isEmpty())
        return;
    fitInView(scene_.itemsBoundingRect().adjusted(-50, -50, 50, 80), Qt::KeepAspectRatio);
    const auto z = qBound(0.15, transform().m11(), 1.0);
    setTransform(QTransform::fromScale(z, z));
    centerOn(scene_.itemsBoundingRect().center());
    changedViewport();
    if (editable_)
        emit layoutEdited();
}
void CanvasView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        zoomBy(std::pow(1.0015, event->angleDelta().y()));
        event->accept();
    } else {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - event->angleDelta().x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - event->angleDelta().y());
        changedViewport();
        event->accept();
    }
}
void CanvasView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && !itemAt(event->pos()) &&
         !(event->modifiers() & Qt::ShiftModifier))) {
        panning_ = true;
        lastPan_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}
void CanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (panning_) {
        const auto d = event->pos() - lastPan_;
        lastPan_ = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}
void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (panning_) {
        panning_ = false;
        unsetCursor();
        changedViewport();
        if (editable_)
            emit layoutEdited();
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
void CanvasView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    placeOverlays();
}
// QAbstractScrollArea scrolls child widgets with the viewport; pin overlays after scrolling.
void CanvasView::scrollContentsBy(int dx, int dy) {
    QGraphicsView::scrollContentsBy(dx, dy);
    placeOverlays();
}
void CanvasView::placeOverlays() {
    if (!controls_ || !mini_)
        return;
    controls_->adjustSize();
    controls_->move(12, qMax(0, viewport()->height() - controls_->height() - 12));
    mini_->move(qMax(0, viewport()->width() - mini_->width() - 14),
                qMax(0, viewport()->height() - mini_->height() - 14));
    mini_->setVisible(viewport()->width() > 410);
}
void CanvasView::drawBackground(QPainter *p, const QRectF &rect) {
    p->save();
    p->fillRect(rect, ui::Background);
    p->setPen(QPen(QColor("#243148"), 0));
    const qreal step = 26 * qMax(1.0, std::ceil(0.55 / transform().m11()));
    const qreal left = std::floor(rect.left() / step) * step,
                top = std::floor(rect.top() / step) * step;
    for (qreal x = left; x < rect.right(); x += step)
        for (qreal y = top; y < rect.bottom(); y += step)
            p->drawPoint(QPointF(x, y));
    p->setPen(QPen(QColor("#394e71"), 1.4));
    for (const auto &v : canvas_["edges"].toArray()) {
        const auto edge = v.toObject();
        auto *a = cards_.value(edge["source"].toString(), nullptr),
             *b = cards_.value(edge["target"].toString(), nullptr);
        if (!a || !b)
            continue;
        const auto from = a->mapToScene(
                       QPointF(a->boundingRect().right(), a->boundingRect().center().y())),
                   to = b->mapToScene(QPointF(0, b->boundingRect().center().y()));
        QPainterPath path(from);
        const auto bend = qMax(50.0, qAbs(to.x() - from.x()) / 2);
        path.cubicTo(from + QPointF(bend, 0), to - QPointF(bend, 0), to);
        p->drawPath(path);
    }
    p->restore();
}
} // namespace mterm
