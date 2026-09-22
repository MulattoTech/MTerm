// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-22-native-ux; docs/ai/changes/2026-09-22-native-ux.json
#pragma once
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHash>
#include <QJsonObject>
class QLabel;
namespace mterm {
class ResourceCard;
class CanvasView final : public QGraphicsView {
    Q_OBJECT
  public:
    explicit CanvasView(QWidget *parent = nullptr);
    void setCanvas(const QJsonObject &canvas, bool editable);
    QJsonObject canvas() const;
    void zoomBy(qreal multiplier);
    void fitResources();
    void arrangeResources();
  signals:
    void layoutEdited();
    void nodeActivated(QJsonObject node);

  protected:
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void scrollContentsBy(int dx, int dy) override;
    void drawBackground(QPainter *, const QRectF &) override;

  private:
    QGraphicsScene scene_;
    QJsonObject canvas_;
    QHash<QString, ResourceCard *> cards_;
    bool editable_ = false, panning_ = false;
    QPoint lastPan_;
    QWidget *controls_ = nullptr, *mini_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
    void placeOverlays();
    void changedViewport();
};
} // namespace mterm
