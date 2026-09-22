// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "CanvasView.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
namespace mterm {
CanvasView::CanvasView(QWidget *parent) : QGraphicsView(parent), scene_(this) {
    setObjectName("native-canvas");
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scene_.setSceneRect(-10000, -10000, 20000, 20000);
    setMinimumHeight(350);
}
void CanvasView::setCanvas(const QJsonObject &canvas, bool editable) {
    canvas_ = canvas;
    editable_ = editable;
    scene_.clear();
    for (const auto &value : canvas["nodes"].toArray()) {
        const auto node = value.toObject();
        const auto width = node["width"].toDouble(), height = node["height"].toDouble();
        auto *item =
            scene_.addRect(0, 0, width, height, QPen(QColor("#365c74")), QBrush(QColor("#162633")));
        item->setData(0, node["id"].toString());
        item->setFlags(QGraphicsItem::ItemIsSelectable);
        if (editable && !node["pinned"].toBool())
            item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setPos(node["x"].toDouble(), node["y"].toDouble());
        auto *title = new QGraphicsTextItem(item);
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        title->setFont(font);
        title->setDefaultTextColor(QColor("#eaf3f8"));
        title->setTextWidth(width - 28);
        title->setPos(12, 8);
        title->setPlainText(node["title"].toString());
        title->setAcceptedMouseButtons(Qt::NoButton);
        auto *body = new QGraphicsTextItem(item);
        body->setTextWidth(width - 28);
        body->setPos(12, 48);
        body->setDefaultTextColor(QColor("#bed0db"));
        body->setPlainText(node["content"].toString().left(1000));
        body->setAcceptedMouseButtons(Qt::NoButton);
        body->setFlag(QGraphicsItem::ItemClipsToShape);
    }
    const auto viewport = canvas["viewport"].toObject();
    const auto zoom = viewport["zoom"].toDouble(1);
    setTransform(QTransform::fromScale(zoom, zoom));
    centerOn((this->viewport()->width() / 2.0 - viewport["x"].toDouble()) / zoom,
             (this->viewport()->height() / 2.0 - viewport["y"].toDouble()) / zoom);
}
QJsonObject CanvasView::canvas() const {
    auto result = canvas_;
    auto nodes = result["nodes"].toArray();
    for (auto *item : scene_.items())
        if (item->data(0).isValid()) {
            for (qsizetype i = 0; i < nodes.size(); ++i) {
                auto node = nodes[i].toObject();
                if (node["id"].toString() != item->data(0).toString())
                    continue;
                node["x"] = item->pos().x();
                node["y"] = item->pos().y();
                nodes[i] = node;
                break;
            }
        }
    result["nodes"] = nodes;
    const auto origin = mapToScene(0, 0);
    const auto zoom = transform().m11();
    result["viewport"] =
        QJsonObject{{"x", -origin.x() * zoom}, {"y", -origin.y() * zoom}, {"zoom", zoom}};
    return result;
}
void CanvasView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const auto current = transform().m11();
        const auto next =
            std::clamp(current * std::pow(1.0015, event->angleDelta().y()), 0.15, 3.0);
        scale(next / current, next / current);
        event->accept();
    } else
        QGraphicsView::wheelEvent(event);
}
void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
    if (editable_)
        emit layoutEdited();
}
void CanvasView::drawBackground(QPainter *painter, const QRectF &rect) {
    painter->fillRect(rect, QColor("#0c161f"));
    painter->setPen(QPen(QColor("#243542"), 0));
    const qreal step = 40;
    const qreal left = std::floor(rect.left() / step) * step,
                top = std::floor(rect.top() / step) * step;
    for (qreal x = left; x < rect.right(); x += step)
        for (qreal y = top; y < rect.bottom(); y += step)
            painter->drawPoint(QPointF(x, y));
}
} // namespace mterm
