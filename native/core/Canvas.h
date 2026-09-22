// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QJsonObject>
namespace mterm {
/// Validate legacy-compatible layout bounds and unique/non-dangling IDs.
/// Returns an empty string on success, otherwise a human-readable error.
QString validateCanvas(const QJsonObject &canvas);
QJsonObject initialCanvas();
QJsonObject newNode(const QString &kind, const QString &title, const QString &content);
}
