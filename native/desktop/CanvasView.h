// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QGraphicsView>
#include <QJsonObject>
namespace mterm {
/// Lightweight native scene. Expensive terminal/editor controls are not duplicated per card.
class CanvasView final : public QGraphicsView {
    Q_OBJECT
  public:
    explicit CanvasView(QWidget *parent = nullptr);
    void setCanvas(const QJsonObject &canvas, bool editable);
    QJsonObject canvas() const;
  signals:
    void layoutEdited();

  protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

  private:
    QGraphicsScene scene_;
    QJsonObject canvas_;
    bool editable_ = false;
};
} // namespace mterm
