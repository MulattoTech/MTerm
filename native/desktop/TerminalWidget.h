// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "terminal/TerminalScreen.h"
#include <QWidget>
#include <memory>
namespace mterm {
/// Native cell renderer over libvterm. Full scrollback/selection/accessibility are issue #3.
class TerminalWidget final : public QWidget {
    Q_OBJECT
  public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    void feed(const QByteArray &data);
    void reset();
    int columns() const {
        return screen_->columns();
    }
    int rows() const {
        return screen_->rows();
    }
    QString screenText() const {
        return screen_->text();
    }
  signals:
    void input(QByteArray bytes);
    void dimensionsChanged(int columns, int rows);
    void parsingFailed(QString error);

  protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool event(QEvent *) override;
    void inputMethodEvent(QInputMethodEvent *) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

  private:
    std::unique_ptr<TerminalScreen> screen_;
    int cellWidth_ = 8, cellHeight_ = 17, ascent_ = 13;
};
} // namespace mterm
