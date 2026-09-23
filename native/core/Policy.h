// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QHash>
#include <QString>
namespace mterm {
/// Human-local policy. Workspace/profile changes revoke all execution grants.
/// Not an OS sandbox. Callers must also enforce file and process ownership.
class Policy {
  public:
    explicit Policy(QString workspace = {});
    void setWorkspace(QString workspace);
    void setDeveloper(bool enabled);
    bool developer() const {
        return developer_;
    }
    bool allows(const QString &capability, qint64 nowMs) const;
    bool grant(const QString &capability, qint64 nowMs, qint64 durationMs);
    void revoke();

  private:
    QString workspace_;
    bool developer_ = false;
    QHash<QString, qint64> grants_;
};
} // namespace mterm
