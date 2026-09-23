// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "TerminalWidget.h"
#include <QFontDatabase>
#include <QFontMetrics>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QResizeEvent>
#include <algorithm>
namespace mterm {
TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent), screen_(std::make_unique<TerminalScreen>()) {
    setObjectName("terminal-screen");
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setMinimumSize(320, 160);
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    setFont(font);
    const QFontMetrics metrics(font);
    cellWidth_ = std::max(1, metrics.horizontalAdvance('M'));
    cellHeight_ = metrics.height();
    ascent_ = metrics.ascent();
}
void TerminalWidget::feed(const QByteArray &data) {
    try {
        const auto replies = screen_->feed(data);
        if (!replies.isEmpty())
            emit input(replies);
        update();
    } catch (const std::exception &error) {
        emit parsingFailed(QString::fromUtf8(error.what()));
    }
}
void TerminalWidget::reset() {
    screen_ = std::make_unique<TerminalScreen>(rows(), columns());
    update();
}
void TerminalWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(12, 22, 31));
    painter.setFont(font());
    for (int row = 0; row < rows(); ++row)
        for (int col = 0; col < columns(); ++col) {
            const auto cell = screen_->cell(row, col);
            const QRect area(4 + col * cellWidth_, 4 + row * cellHeight_,
                             cellWidth_ * std::max(1, cell.width), cellHeight_);
            if (area.top() >= height() || area.left() >= width())
                continue;
            if (cell.background != 0xff0c161f)
                painter.fillRect(area, QColor::fromRgb(cell.background));
            if (cell.text.isEmpty() || cell.width == 0)
                continue;
            QFont glyphFont = font();
            glyphFont.setBold(cell.bold);
            glyphFont.setUnderline(cell.underline);
            painter.setFont(glyphFont);
            painter.setPen(QColor::fromRgb(cell.foreground));
            painter.drawText(area.left(), area.top() + ascent_, cell.text);
        }
    if (hasFocus()) {
        const auto cursor = screen_->cursor();
        painter.setPen(QColor(125, 204, 220));
        painter.drawRect(4 + cursor.col * cellWidth_, 4 + cursor.row * cellHeight_, cellWidth_ - 1,
                         cellHeight_ - 1);
    }
}
void TerminalWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    const int cols = std::clamp((width() - 8) / cellWidth_, 20, 400),
              rs = std::clamp((height() - 8) / cellHeight_, 5, 200);
    if (cols != columns() || rs != rows()) {
        screen_->resize(rs, cols);
        emit dimensionsChanged(cols, rs);
    }
    update();
}
bool TerminalWidget::event(QEvent *event) {
    // A focused terminal owns its keyboard shortcuts; Ctrl+K must reach the shell.
    if (event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    return QWidget::event(event);
}
void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    int modifiers = VTERM_MOD_NONE;
    if (event->modifiers() & Qt::ShiftModifier)
        modifiers |= VTERM_MOD_SHIFT;
    if (event->modifiers() & Qt::ControlModifier)
        modifiers |= VTERM_MOD_CTRL;
    if (event->modifiers() & Qt::AltModifier)
        modifiers |= VTERM_MOD_ALT;
    VTermKey key = VTERM_KEY_NONE;
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        key = VTERM_KEY_ENTER;
        break;
    case Qt::Key_Tab:
        key = VTERM_KEY_TAB;
        break;
    case Qt::Key_Backspace:
        key = VTERM_KEY_BACKSPACE;
        break;
    case Qt::Key_Escape:
        key = VTERM_KEY_ESCAPE;
        break;
    case Qt::Key_Up:
        key = VTERM_KEY_UP;
        break;
    case Qt::Key_Down:
        key = VTERM_KEY_DOWN;
        break;
    case Qt::Key_Left:
        key = VTERM_KEY_LEFT;
        break;
    case Qt::Key_Right:
        key = VTERM_KEY_RIGHT;
        break;
    case Qt::Key_Home:
        key = VTERM_KEY_HOME;
        break;
    case Qt::Key_End:
        key = VTERM_KEY_END;
        break;
    case Qt::Key_Insert:
        key = VTERM_KEY_INS;
        break;
    case Qt::Key_Delete:
        key = VTERM_KEY_DEL;
        break;
    case Qt::Key_PageUp:
        key = VTERM_KEY_PAGEUP;
        break;
    case Qt::Key_PageDown:
        key = VTERM_KEY_PAGEDOWN;
        break;
    default:
        break;
    }
    QByteArray bytes;
    if (key != VTERM_KEY_NONE)
        bytes = screen_->key(key, VTermModifier(modifiers));
    else
        for (auto ch : event->text().toUcs4())
            bytes += screen_->unicode(char32_t(ch), VTermModifier(modifiers));
    if (!bytes.isEmpty())
        emit input(bytes);
    event->accept();
}
void TerminalWidget::inputMethodEvent(QInputMethodEvent *event) {
    if (!event->commitString().isEmpty())
        emit input(event->commitString().toUtf8());
    event->accept();
}
QVariant TerminalWidget::inputMethodQuery(Qt::InputMethodQuery query) const {
    if (query == Qt::ImCursorRectangle) {
        const auto cursor = screen_->cursor();
        return QRect(4 + cursor.col * cellWidth_, 4 + cursor.row * cellHeight_, cellWidth_,
                     cellHeight_);
    }
    return QWidget::inputMethodQuery(query);
}
} // namespace mterm
