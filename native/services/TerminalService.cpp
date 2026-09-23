// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
#include "TerminalService.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace mterm {
namespace {
[[noreturn]] void fail(const QString &message) {
    throw std::runtime_error(message.toStdString());
}
QString stringArg(const QJsonObject &a, const QString &key, const QString &fallback,
                  int max = 4096) {
    if (!a.contains(key))
        return fallback;
    if (!a[key].isString())
        fail(key + " must be a string");
    const auto text = a[key].toString();
    if (text.isEmpty() || text.size() > max || text.contains(QChar::Null))
        fail("Invalid " + key);
    return text;
}
int dimension(const QJsonObject &a, const QString &key, int fallback, int max) {
    if (!a.contains(key))
        return fallback;
    const auto v = a[key].toDouble(-1);
    if (!a[key].isDouble() || !std::isfinite(v) || v != std::floor(v) || v < 1 || v > max)
        fail("Invalid terminal " + key);
    return int(v);
}
QString stamp() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
} // namespace
TerminalService::TerminalService(WorkspaceSession &session, QObject *parent)
    : QObject(parent), session_(session) {}
TerminalService::~TerminalService() {
    stop();
}
QString TerminalService::resourceId(const QJsonObject &args) const {
    return stringArg(args, "terminalId", "default", 128);
}
QJsonObject TerminalService::resource(const QString &id) const {
    if (id == "default")
        return {{"id", id}, {"title", "Workspace terminal"}};
    const auto nodes = session_.store().setting("canvas:" + session_.id())["nodes"].toArray();
    for (const auto &v : nodes) {
        auto n = v.toObject();
        if (n["id"] == id && n["kind"] == "terminal")
            return n;
    }
    fail("Terminal resource does not belong to the current workspace canvas");
}
std::shared_ptr<TerminalService::Slot> TerminalService::requireRun(const QString &id,
                                                                   const QJsonObject &args) const {
    const auto slot = active_.value(id);
    if (!slot || slot->workspace != session_.id())
        fail("Terminal is not running in this workspace");
    // Named resources always require an execution token; compatibility fallback only for the old
    // default API.
    if (id != "default" || args.contains("runId"))
        if (!args["runId"].isString() || args["runId"].toString() != slot->run)
            fail("STALE_TERMINAL: execution changed; refresh before acting");
    return slot;
}
void TerminalService::persist(const std::shared_ptr<Slot> &slot) {
    slot->metadata["updatedAt"] = stamp();
    session_.store().putRecord("terminal-resource", slot->workspace, slot->metadata);
    emit sessionChanged(slot->workspace, slot->id, slot->run, slot->metadata);
}
void TerminalService::stop() {
    const auto copy = active_.values();
    for (const auto &slot : copy) {
        slot->requestedStop = true;
        slot->pty->stop();
    }
}
QJsonObject TerminalService::call(const QString &method, const QJsonObject &args) {
    session_.requireScope(args);
    if (method == "pty-list") {
        session_.authorize("workspace.read", args);
        if (args.contains("offset") &&
            (!args["offset"].isDouble() || args["offset"].toDouble() < 0 ||
             args["offset"].toDouble() != args["offset"].toInt()))
            fail("Invalid terminal history offset");
        auto rows = session_.store().records("terminal-resource", session_.id(), 100,
                                             args["offset"].toInt());
        return {{"items", rows},
                {"activeCount", active_.size()},
                {"maxActive", MaxActive},
                {"pageSize", 100},
                {"mayHaveMore", rows.size() == 100}};
    }
    const auto id = resourceId(args);
    if (method == "pty-ack") {
        const auto slot = requireRun(id, args);
        const auto count = dimension(args, "bytes", 0, 2097152);
        if (!slot->flowControl || count < 1 || count > slot->outstanding)
            fail("Invalid terminal consumer acknowledgement");
        slot->outstanding -= count;
        if (slot->outstanding < 262144)
            slot->pty->setOutputPaused(false);
        return {{"accepted", true}};
    }
    if (method == "pty-stop") {
        if (!active_.contains(id) && id == "default" && !args.contains("runId"))
            return {{"stopped", true}};
        const auto slot = requireRun(id, args);
        slot->requestedStop = true;
        slot->pty->stop();
        return {{"stopped", true}, {"terminalId", id}, {"runId", slot->run}};
    }
    session_.authorize("terminal.execute", args);
    if (method == "pty-start") {
        const auto node = resource(id);
        if (args.contains("flowControl") && !args["flowControl"].isBool())
            fail("flowControl must be boolean");
        if (active_.contains(id))
            fail("This terminal resource is already running");
        if (active_.size() >= MaxActive)
            fail("Terminal capacity reached (8 live shells); stop a shell before starting another");
        const auto shell = stringArg(args, "shell", "pwsh", 32);
        const auto relative = stringArg(args, "cwd", ".");
        const auto cwd = session_.files().resolve(relative);
        if (!QFileInfo(cwd).isDir())
            fail("Terminal working directory must be an existing workspace directory");
        const auto columns = dimension(args, "columns", 100, 400),
                   rows = dimension(args, "rows", 30, 200);
        QString program;
        QStringList options;
#ifdef Q_OS_WIN
        if (shell == "cmd") {
            program = qEnvironmentVariable("SystemRoot") + "/System32/cmd.exe";
            options = {"/D", "/Q"};
        } else if (shell == "pwsh") {
            program = QStandardPaths::findExecutable("pwsh.exe");
            options = {"-NoLogo", "-NoProfile"};
        } else if (shell == "powershell") {
            program = qEnvironmentVariable("SystemRoot") +
                      "/System32/WindowsPowerShell/v1.0/powershell.exe";
            options = {"-NoLogo", "-NoProfile"};
        } else
            fail("Unknown native shell profile");
#else
        Q_UNUSED(columns);
        Q_UNUSED(rows);
        fail("Native PTY transport is currently Windows-only");
#endif
        auto slot = std::make_shared<Slot>();
        slot->id = id;
        slot->workspace = session_.id();
        slot->run = QUuid::createUuid().toString(QUuid::WithoutBraces);
        slot->flowControl = args["flowControl"].toBool(false);
        const auto recordId =
            "pty-" +
            QString::fromLatin1(QCryptographicHash::hash((slot->workspace + ":" + id).toUtf8(),
                                                         QCryptographicHash::Sha256)
                                    .toHex());
        slot->metadata = {{"id", recordId},
                          {"workspaceId", slot->workspace},
                          {"terminalId", id},
                          {"runId", slot->run},
                          {"title", node["title"].toString().left(160)},
                          {"shell", shell},
                          {"cwd", QDir(session_.root()).relativeFilePath(cwd)},
                          {"status", "STARTING"},
                          {"startedAt", stamp()},
                          {"reattached", false}};
        slot->metadata["flowControl"] = slot->flowControl;
        persist(slot); // Fail before OS execution if metadata cannot be recorded.
        slot->pty = new PtySession(this);
        active_.insert(id, slot);
        const std::weak_ptr<Slot> weak = slot;
        connect(slot->pty, &PtySession::dataReady, this, [this, weak](const QByteArray &bytes) {
            if (auto s = weak.lock()) {
                // High-water bound includes one <=64KiB delivery block. Final exit may flush
                // <=1MiB.
                if (s->flowControl) {
                    s->outstanding += bytes.size();
                    if (s->outstanding >= 262144)
                        s->pty->setOutputPaused(true);
                }
                emit sessionOutput(s->workspace, s->id, s->run, bytes);
                if (s->id == "default")
                    emit output(s->workspace, bytes);
            }
        });
        connect(slot->pty, &PtySession::exited, this, [this, weak](int code) {
            const auto s = weak.lock();
            if (!s)
                return;
            s->metadata["status"] = s->requestedStop ? "STOPPED" : code == 0 ? "EXITED" : "FAILED";
            s->metadata["exitCode"] = code;
            s->metadata["endedAt"] = stamp();
            try {
                persist(s);
                session_.store().audit(s->workspace, "terminal.exit", "ALLOW",
                                       s->id + QString("; exit=%1").arg(code));
            } catch (const std::exception &) {
                s->metadata["persistenceError"] = true;
                emit sessionChanged(s->workspace, s->id, s->run, s->metadata);
            }
            if (active_.value(s->id) == s)
                active_.remove(s->id);
            if (s->id == "default")
                emit stopped(s->workspace, code);
            s->pty->deleteLater();
        });
        if (!slot->pty->start(program, options, cwd, columns, rows)) {
            const auto error = slot->pty->error();
            active_.remove(id);
            slot->metadata["status"] = "FAILED";
            slot->metadata["endedAt"] = stamp();
            slot->pty->deleteLater();
            persist(slot);
            fail(error);
        }
        slot->metadata["status"] = "RUNNING";
        try {
            persist(slot);
            session_.store().audit(slot->workspace, "terminal.start", "ALLOW",
                                   id + "; native PTY started; content not persisted");
        } catch (...) {
            slot->requestedStop = true;
            slot->pty->stop();
            throw;
        }
        auto result = slot->metadata;
        result["running"] = true;
        return result;
    }
    const auto slot = requireRun(id, args);
    if (method == "pty-write") {
        if (!args["dataBase64"].isString())
            fail("Terminal input must be base64 text");
        const auto encoded = args["dataBase64"].toString().toLatin1();
        if (encoded.size() > 90000)
            fail("Terminal input exceeds limit");
        const auto decoded =
            QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || !slot->pty->write(decoded.decoded))
            fail("Terminal input rejected or backpressured");
        return {{"accepted", true}, {"terminalId", id}, {"runId", slot->run}};
    }
    if (method == "pty-resize") {
        if (!slot->pty->resize(dimension(args, "columns", 100, 400),
                               dimension(args, "rows", 30, 200)))
            fail("Terminal resize rejected");
        return {{"resized", true}, {"terminalId", id}, {"runId", slot->run}};
    }
    fail("Unknown native PTY operation");
}
} // namespace mterm
