// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include "core/BoundedBuffer.h"
namespace mterm {
struct Command { QString program; QStringList arguments; QString cwd; QByteArray input; int timeoutMs=60000; };
struct ProcessResult {
 int exitCode=-1; bool cancelled=false; bool timedOut=false; QString error;
 QByteArray output; QByteArray errors; qint64 droppedBytes=0; qint64 durationMs=0;
};
/// Asynchronous, bounded child capture. Call only after policy approval.
/// Signals and public methods belong to the owning thread. No implicit shell.
class ProcessRunner final: public QObject {
 Q_OBJECT
public:
 explicit ProcessRunner(QObject *parent=nullptr);
 ~ProcessRunner() override;
 bool start(const Command &command);
 bool running() const {return active_;}
 qint64 pid() const {return process_.processId();}
 void cancel();
signals:
 void outputReady(QByteArray data,bool standardError);
 void completed(mterm::ProcessResult result);
private:
 QProcess process_; QTimer timer_; QElapsedTimer elapsed_;
 BoundedBuffer output_{262144},errors_{262144};
 bool active_=false,cancelled_=false,timedOut_=false;
 QString error_; void *job_=nullptr;
 void finish(int code);
 void closeJob();
};
}
Q_DECLARE_METATYPE(mterm::ProcessResult)
