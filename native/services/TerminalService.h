// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
#pragma once
#include "WorkspaceSession.h"
#include "terminal/PtySession.h"
#include <QHash>
#include <memory>
namespace mterm {
/// Worker-thread pool. Stable resource identity is separate from each execution's runId.
/// At most MaxActive native shells; approval remains workspace-session scoped.
/// Metadata persists, terminal content does not; process reattachment is not implemented.
class TerminalService final : public QObject {
    Q_OBJECT
  public:
    static constexpr int MaxActive = 8;
    explicit TerminalService(WorkspaceSession &session, QObject *parent = nullptr);
    ~TerminalService() override;
    QJsonObject call(const QString &method, const QJsonObject &args);
    void stop();
  signals:
    // Legacy default-terminal events, retained for the existing local adapter/tests.
    void output(QString workspaceId, QByteArray bytes);
    void stopped(QString workspaceId, int code);
    void sessionOutput(QString workspaceId, QString terminalId, QString runId, QByteArray bytes);
    void sessionChanged(QString workspaceId, QString terminalId, QString runId,
                        QJsonObject metadata);

  private:
    struct Slot {
        QString workspace, id, run;
        QJsonObject metadata;
        PtySession *pty = nullptr;
        bool requestedStop = false;
        bool flowControl = false;
        qsizetype outstanding = 0;
    };
    WorkspaceSession &session_;
    QHash<QString, std::shared_ptr<Slot>> active_;
    QString resourceId(const QJsonObject &args) const;
    QJsonObject resource(const QString &id) const;
    std::shared_ptr<Slot> requireRun(const QString &id, const QJsonObject &args) const;
    void persist(const std::shared_ptr<Slot> &slot);
};
} // namespace mterm
