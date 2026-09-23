// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "Theme.h"
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyleFactory>
static void initializeMTermResources() {
    Q_INIT_RESOURCE(resources);
}
namespace mterm::ui {
QColor kindColor(const QString &kind) {
    if (kind == "terminal" || kind == "process")
        return QColor("#61d7b4");
    if (kind == "agent")
        return QColor("#b09aff");
    if (kind == "task")
        return QColor("#f1c581");
    if (kind == "note")
        return QColor("#e0c899");
    return Accent;
}
QIcon icon(const QString &name, const QColor &color) {
    QPixmap image(40, 40);
    image.setDevicePixelRatio(2);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 1.45, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (name == "terminal" || name == "commands") {
        p.drawLine(4, 5, 8, 9);
        p.drawLine(8, 9, 4, 13);
        p.drawLine(10, 14, 16, 14);
    } else if (name == "agent") {
        QPainterPath q;
        q.moveTo(10, 2);
        q.lineTo(12, 7);
        q.lineTo(17, 9);
        q.lineTo(12, 11);
        q.lineTo(10, 16);
        q.lineTo(8, 11);
        q.lineTo(3, 9);
        q.lineTo(8, 7);
        q.closeSubpath();
        p.drawPath(q);
        p.drawLine(16, 2, 16, 5);
        p.drawLine(14.5, 3.5, 17.5, 3.5);
    } else if (name == "git") {
        p.drawLine(6, 3, 6, 16);
        p.drawLine(6, 12, 14, 7);
        p.setBrush(Background);
        p.drawEllipse(QPointF(6, 4), 2, 2);
        p.drawEllipse(QPointF(6, 15), 2, 2);
        p.drawEllipse(QPointF(14, 6), 2, 2);
    } else if (name == "task") {
        p.drawRoundedRect(QRectF(3, 3, 14, 14), 3, 3);
        p.drawLine(6, 10, 9, 13);
        p.drawLine(9, 13, 14, 7);
    } else if (name == "canvas") {
        p.drawRoundedRect(QRectF(2, 3, 7, 6), 1, 1);
        p.drawRoundedRect(QRectF(11, 3, 7, 9), 1, 1);
        p.drawRoundedRect(QRectF(2, 11, 7, 6), 1, 1);
    } else if (name == "process") {
        p.drawLine(2, 11, 5, 11);
        p.drawLine(5, 11, 8, 4);
        p.drawLine(8, 4, 12, 16);
        p.drawLine(12, 16, 15, 9);
        p.drawLine(15, 9, 18, 9);
    } else if (name == "search") {
        p.drawEllipse(QRectF(3, 3, 10, 10));
        p.drawLine(12, 12, 17, 17);
    } else if (name == "folder") {
        QPainterPath q;
        q.moveTo(2, 5);
        q.lineTo(8, 5);
        q.lineTo(10, 7);
        q.lineTo(18, 7);
        q.lineTo(17, 16);
        q.lineTo(2, 16);
        q.closeSubpath();
        p.drawPath(q);
    } else if (name == "plus") {
        p.drawLine(10, 4, 10, 16);
        p.drawLine(4, 10, 16, 10);
    } else if (name == "project") {
        for (int y : {3, 8, 13}) {
            p.drawRoundedRect(QRectF(3, y, 14, 3), 1, 1);
        }
    } else {
        p.drawRoundedRect(QRectF(4, 2, 12, 16), 2, 2);
        p.drawLine(7, 6, 13, 6);
        p.drawLine(7, 10, 13, 10);
        p.drawLine(7, 14, 11, 14);
    }
    return QIcon(image);
}
void applyTheme(QApplication &app) {
    if (app.property("mterm-theme-ready").toBool())
        return;
    app.setProperty("mterm-theme-ready", true);
    initializeMTermResources();
    app.setStyle(QStyleFactory::create("Fusion"));
    QFont font("Segoe UI");
    font.setPixelSize(13);
    app.setFont(font);
    QPalette palette;
    palette.setColor(QPalette::Window, Background);
    palette.setColor(QPalette::Base, Background);
    palette.setColor(QPalette::WindowText, Text);
    palette.setColor(QPalette::Text, Text);
    palette.setColor(QPalette::Button, Surface);
    palette.setColor(QPalette::ButtonText, Text);
    palette.setColor(QPalette::Highlight, QColor("#294775"));
    palette.setColor(QPalette::HighlightedText, Text);
    palette.setColor(QPalette::AlternateBase, QColor("#0e1623"));
    app.setPalette(palette);
    QFile file(":/mterm/theme.qss");
    if (file.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
}
} // namespace mterm::ui
