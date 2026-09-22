// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QColor>
#include <QIcon>
class QApplication;
namespace mterm::ui {
inline const QColor Background{"#080d16"}, Surface{"#101827"}, Raised{"#151f31"};
inline const QColor Border{"#263249"}, Text{"#e6edf7"}, Muted{"#8795ab"}, Accent{"#83aaff"};
/// Apply once to the native app. No network fonts, web rendering or polling.
void applyTheme(QApplication &app);
/// Original vector glyphs generated at high DPI; no third-party artwork.
QIcon icon(const QString &name, const QColor &color = Muted);
QColor kindColor(const QString &kind);
} // namespace mterm::ui
