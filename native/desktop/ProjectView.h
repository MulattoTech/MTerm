// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QJsonObject>
#include <QListWidget>
namespace mterm {
/// Alternate projection of the same resource IDs; no independent mutable canvas state.
class ProjectView final : public QListWidget {
    Q_OBJECT
  public:
    explicit ProjectView(QWidget *parent = nullptr);
    void setCanvas(const QJsonObject &canvas);
  signals:
    void nodeActivated(QJsonObject node);
};
} // namespace mterm
