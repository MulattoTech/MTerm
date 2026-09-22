// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "ResourceCard.h"
#include "ui/Theme.h"
#include <QFontMetrics>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
namespace mterm {
namespace {
QFont font(int pixels, bool bold = false) {
    QFont f("Segoe UI");
    f.setPixelSize(pixels);
    f.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    return f;
}
QString actionName(QString kind) {
    if (kind == "terminal")
        return "Open terminal";
    if (kind == "agent")
        return "Open agent";
    if (kind == "editor" || kind == "file")
        return "Open editor";
    if (kind == "task")
        return "View tasks";
    if (kind == "process")
        return "Inspect processes";
    if (kind == "diff")
        return "Open Git / Diff";
    return "Open details";
}
} // namespace
void paintResourceCard(QPainter &p, const QRectF &rect, const QJsonObject &node, bool selected,
                       bool hovered) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    const auto box = rect.adjusted(2, 2, -2, -2);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 35));
    p.drawRoundedRect(box.translated(0, 2), 12, 12);
    p.setBrush(ui::Surface);
    p.setPen(QPen(selected  ? ui::Accent
                  : hovered ? QColor("#3c5174")
                            : ui::Border,
                  selected ? 1.6 : 1));
    p.drawRoundedRect(box, 12, 12);
    const auto kind = node["kind"].toString("note"), status = node["status"].toString("draft");
    const auto accent = ui::kindColor(kind);
    p.setFont(font(10, true));
    p.setPen(accent);
    const auto badge = kind.toUpper();
    QFontMetrics metrics(p.font());
    const int badgeW = metrics.horizontalAdvance(badge) + 18;
    QColor tint = accent;
    tint.setAlpha(25);
    p.setBrush(tint);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(box.left() + 16, box.top() + 16, badgeW, 23), 6, 6);
    p.setPen(accent);
    p.drawText(QRectF(box.left() + 16, box.top() + 16, badgeW, 23), Qt::AlignCenter, badge);
    p.setPen(ui::Muted);
    p.setFont(font(10));
    p.drawText(QRectF(box.right() - 114, box.top() + 16, 96, 23), Qt::AlignRight | Qt::AlignVCenter,
               status.toUpper());
    p.setFont(font(16, true));
    p.setPen(ui::Text);
    const auto title = QFontMetrics(p.font()).elidedText(node["title"].toString("Untitled"),
                                                         Qt::ElideRight, int(box.width() - 34));
    p.drawText(QRectF(box.left() + 16, box.top() + 51, box.width() - 32, 25),
               Qt::AlignLeft | Qt::AlignVCenter, title);
    if (node["collapsed"].toBool()) {
        p.restore();
        return;
    }
    p.setFont(font(12));
    p.setPen(QColor("#99abc6"));
    QRectF body(box.left() + 16, box.top() + 88, box.width() - 32, qMax(12.0, box.height() - 143));
    p.setClipRect(body);
    p.drawText(body, Qt::TextWordWrap | Qt::AlignTop, node["content"].toString().left(800));
    p.setClipping(false);
    p.setPen(QPen(QColor("#223048"), 1));
    p.drawLine(QPointF(box.left() + 16, box.bottom() - 45),
               QPointF(box.right() - 16, box.bottom() - 45));
    p.setFont(font(11, true));
    p.setPen(hovered ? ui::Text : ui::Accent);
    p.drawText(QRectF(box.left() + 16, box.bottom() - 38, box.width() - 52, 28), Qt::AlignVCenter,
               actionName(kind));
    p.drawText(QRectF(box.right() - 35, box.bottom() - 38, 20, 28), Qt::AlignCenter,
               QString::fromUtf8("\xe2\x86\x97"));
    if (node["pinned"].toBool()) {
        p.setPen(ui::Muted);
        p.setFont(font(9));
        p.drawText(QRectF(box.left() + 110, box.top() + 16, 60, 23), Qt::AlignVCenter, "PINNED");
    }
    p.restore();
}
ResourceCard::ResourceCard(QJsonObject node, bool editable) {
    setAcceptHoverEvents(true);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setNode(std::move(node), editable);
}
QRectF ResourceCard::boundingRect() const {
    return {0, 0, node_["width"].toDouble(360),
            node_["collapsed"].toBool() ? 92 : node_["height"].toDouble(260)};
}
void ResourceCard::setNode(QJsonObject node, bool editable) {
    prepareGeometryChange();
    node_ = std::move(node);
    editable_ = editable;
    setData(0, node_["id"].toString());
    setFlag(ItemIsSelectable);
    setFlag(ItemIsMovable, editable && !node_["pinned"].toBool());
    setPos(node_["x"].toDouble(), node_["y"].toDouble());
    setToolTip(node_["title"].toString() +
               "\nDouble-click to open. Right-click for position and collapse controls.");
    update();
}
QJsonObject ResourceCard::node() const {
    auto n = node_;
    n["x"] = pos().x();
    n["y"] = pos().y();
    return n;
}
void ResourceCard::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) {
    paintResourceCard(*p, boundingRect(), node_, isSelected(), hovered_);
}
void ResourceCard::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
    hovered_ = true;
    update();
    QGraphicsObject::hoverEnterEvent(e);
}
void ResourceCard::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
    hovered_ = false;
    update();
    QGraphicsObject::hoverLeaveEvent(e);
}
void ResourceCard::mousePressEvent(QGraphicsSceneMouseEvent *e) {
    startPos_ = pos();
    pressPos_ = e->pos();
    QGraphicsObject::mousePressEvent(e);
}
void ResourceCard::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
    QGraphicsObject::mouseReleaseEvent(e);
    if (pos() != startPos_) {
        emit edited();
        return;
    }
    if ((e->pos() - pressPos_).manhattanLength() < 4 && e->pos().y() > boundingRect().height() - 46)
        emit activated(node());
}
void ResourceCard::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
    emit activated(node());
    e->accept();
}
void ResourceCard::contextMenuEvent(QGraphicsSceneContextMenuEvent *e) {
    QMenu menu;
    auto *open = menu.addAction(actionName(node_["kind"].toString()));
    menu.addSeparator();
    auto *pin = menu.addAction("Pin position");
    pin->setCheckable(true);
    pin->setChecked(node_["pinned"].toBool());
    pin->setEnabled(editable_);
    auto *collapse = menu.addAction("Collapse card");
    collapse->setCheckable(true);
    collapse->setChecked(node_["collapsed"].toBool());
    collapse->setEnabled(editable_);
    const auto *chosen = menu.exec(e->screenPos());
    if (chosen == open)
        emit activated(node());
    else if (chosen == pin || chosen == collapse) {
        auto n = node();
        n[chosen == pin ? "pinned" : "collapsed"] = chosen->isChecked();
        setNode(n, editable_);
        emit edited();
    }
    e->accept();
}
} // namespace mterm
