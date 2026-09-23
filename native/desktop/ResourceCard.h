// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QGraphicsObject>
#include <QJsonObject>
class QPainter;
namespace mterm {
/// Shared card presentation for canvas items and Project view delegates.
void paintResourceCard(QPainter &p, const QRectF &rect, const QJsonObject &node, bool selected,
                       bool hovered);
/// One graphics item per resource: no nested text widgets or embedded browsers.
class ResourceCard final : public QGraphicsObject {
    Q_OBJECT
  public:
    explicit ResourceCard(QJsonObject node, bool editable);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    void setNode(QJsonObject node, bool editable);
    QJsonObject node() const;
    /// Transient process badge only; never mutates the persisted canvas status.
    void setRuntimeStatus(const QString &status);
  signals:
    void activated(QJsonObject node);
    void edited();

  protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *) override;

  private:
    QJsonObject node_;
    bool editable_ = false, hovered_ = false;
    QPointF startPos_, pressPos_;
};
} // namespace mterm
