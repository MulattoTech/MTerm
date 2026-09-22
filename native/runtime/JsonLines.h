// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QJsonObject>
#include <QList>
#include <QByteArray>
namespace mterm {
/// Bounded JSONL decoder. Throws on malformed or oversized protocol input.
class JsonLines {
public:
 QList<QJsonObject> feed(const QByteArray &chunk);
 QList<QJsonObject> finish();
private:
 QByteArray pending_;
};
}
