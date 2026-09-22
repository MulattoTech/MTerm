// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QJsonArray>
#include <QJsonObject>
namespace mterm {
/// Read-only OS inventory. Unavailable metrics are JSON null, never fake zeros.
/// CPU is cumulative seconds, NOT utilization percentage. No shell is spawned.
QJsonArray processInventory(int limit = 512);
/// Current process only, no command-line/environment data. Byte units.
QJsonObject currentProcessFootprint();
} // namespace mterm
