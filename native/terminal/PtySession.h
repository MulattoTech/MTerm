// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <memory>
namespace mterm {
/// Native Windows ConPTY transport. Policy approval is required before start/write.
/// Owns the shell job. Bounded queues and separate reader/writer threads prevent pipe deadlocks.
/// All public methods belong to one service thread; raw output contains UTF-8/VT sequences.
class PtySession final : public QObject {
    Q_OBJECT
  public:
    explicit PtySession(QObject *parent = nullptr);
    ~PtySession() override;
    bool start(const QString &executable, const QStringList &arguments, const QString &cwd,
               int columns = 100, int rows = 30);
    bool write(const QByteArray &bytes);
    bool resize(int columns, int rows);
    /// Pause service-thread delivery; the bounded reader queue applies OS-pipe backpressure.
    void setOutputPaused(bool paused);
    void stop();
    bool running() const;
    QString error() const {
        return error_;
    }
  signals:
    void dataReady(QByteArray bytes);
    void exited(int code);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QTimer drain_, poll_;
    QString error_;
    void finish(int code);
};
} // namespace mterm
