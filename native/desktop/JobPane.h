// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "services/Backend.h"
#include <QJsonObject>
#include <QWidget>
class QLineEdit;
class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QLabel;
namespace mterm {
/// One view owns one displayed run ID; unrelated agent/command output is never mixed.
class JobPane final : public QWidget {
    Q_OBJECT
  public:
    JobPane(QString kind, Backend *backend, QWidget *parent = nullptr);
    void setWorkspace(const QJsonObject &state);

  private:
    QString kind_, workspaceId_, runId_;
    Backend *backend_;
    QLineEdit *program_ = nullptr, *args_ = nullptr;
    QPlainTextEdit *prompt_ = nullptr, *output_ = nullptr;
    QComboBox *gitAction_ = nullptr, *resume_ = nullptr;
    QPushButton *run_ = nullptr, *approve_ = nullptr, *stop_ = nullptr;
    QLabel *status_ = nullptr;
    quint64 pendingRun_ = 0;
    void run();
    void append(const QString &text);
};
} // namespace mterm
