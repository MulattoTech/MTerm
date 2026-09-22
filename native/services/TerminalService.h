// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "WorkspaceSession.h"
#include "terminal/PtySession.h"
namespace mterm {
/// One workspace-scoped native PTY. Extra sessions/re-attach are tracked in issue #3.
class TerminalService final : public QObject {
    Q_OBJECT
  public:
    TerminalService(WorkspaceSession &session, QObject *parent = nullptr);
    QJsonObject call(const QString &method, const QJsonObject &args);
    void stop();
  signals:
    void output(QString workspaceId, QByteArray bytes);
    void stopped(QString workspaceId, int code);

  private:
    WorkspaceSession &session_;
    PtySession pty_;
    QString owner_;
};
} // namespace mterm
