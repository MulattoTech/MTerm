// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// Modified: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro); see docs/ai/changes/.
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QHash>
#include <QJsonObject>
#include <QListWidget>
namespace mterm {
/// Alternate projection of the same resource IDs; no independent mutable canvas state.
class ProjectView final : public QListWidget {
    Q_OBJECT
  public:
    explicit ProjectView(QWidget *parent = nullptr);
    void setCanvas(const QJsonObject &canvas);
    void setFilter(const QString &text);
    void setRuntimeStatuses(const QHash<QString, QString> &statuses);

  private:
    QString filter_;
    QHash<QString, QString> runtimeStatuses_;
  signals:
    void nodeActivated(QJsonObject node);
};
} // namespace mterm
