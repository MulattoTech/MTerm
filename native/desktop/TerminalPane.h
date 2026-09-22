// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "services/Backend.h"
#include <QJsonObject>
#include <QWidget>
class QComboBox;
class QLabel;
class QPushButton;
namespace mterm {
class TerminalWidget;
/// Human-facing approval and shell lifecycle UI. Does not hold OS handles.
class TerminalPane final : public QWidget {
    Q_OBJECT
  public:
    explicit TerminalPane(Backend *backend, QWidget *parent = nullptr);
    void setWorkspace(const QJsonObject &state);

  private:
    Backend *backend_;
    TerminalWidget *screen_;
    QComboBox *shell_;
    QLabel *status_;
    QPushButton *start_, *approve_, *stop_;
    QString workspaceId_;
    bool active_ = false;
    quint64 pending_ = 0, grant_ = 0;
    void input(const QByteArray &bytes);
};
} // namespace mterm
