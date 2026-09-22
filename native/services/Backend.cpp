// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "Backend.h"
#include "JobService.h"
#include "TerminalService.h"
#include "WorkspaceSession.h"
#include <memory>
namespace mterm {
class Worker final : public QObject {
    Q_OBJECT
  public:
    explicit Worker(QString file)
        : session_(std::move(file)), jobs_(session_, this), terminal_(session_, this) {}
    QJsonObject call(const QString &method, const QJsonObject &args) {
        if (method == "open") {
            jobs_.cancelAll();
            terminal_.stop();
            return session_.open(args["root"].toString());
        }
        if (method == "profile" && args["developer"] == false) {
            session_.requireScope(args);
            jobs_.cancelAll();
            terminal_.stop();
        }
        if (method.startsWith("pty-"))
            return terminal_.call(method, args);
        if (method == "run")
            return jobs_.run(args);
        if (method == "cancel")
            return jobs_.cancel(args);
        if (method == "provider") {
            session_.requireScope(args);
            return {{"codexExecutable", JobService::findCodex()}};
        }
        return session_.call(method, args);
    }
    JobService *jobs() {
        return &jobs_;
    }
    TerminalService *terminal() {
        return &terminal_;
    }

  private:
    WorkspaceSession session_;
    JobService jobs_;
    TerminalService terminal_;
};
Backend::Backend(QString databaseFile, QObject *parent)
    : QObject(parent), worker_(new Worker(std::move(databaseFile))) {
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_->jobs(), &JobService::stream, this, &Backend::stream);
    connect(worker_->jobs(), &JobService::runFinished, this, &Backend::runFinished);
    connect(worker_->terminal(), &TerminalService::output, this, &Backend::terminalOutput);
    connect(worker_->terminal(), &TerminalService::stopped, this, &Backend::terminalStopped);
    workerThread_.setObjectName("MTerm-services");
    workerThread_.start();
}
Backend::~Backend() {
    workerThread_.quit();
    workerThread_.wait();
}
quint64 Backend::request(const QString &method, const QJsonObject &args) {
    const auto id = ++nextId_;
    QMetaObject::invokeMethod(
        worker_,
        [this, id, method, args] {
            try {
                const auto result = worker_->call(method, args);
                emit response(id, method, result, {});
            } catch (const std::exception &e) {
                emit response(id, method, {}, QString::fromUtf8(e.what()));
            }
        },
        Qt::QueuedConnection);
    return id;
}
} // namespace mterm
#include "Backend.moc"
