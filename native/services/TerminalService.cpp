// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "TerminalService.h"
#include <QStandardPaths>
#include <stdexcept>
namespace mterm {
TerminalService::TerminalService(WorkspaceSession &session, QObject *parent)
    : QObject(parent), session_(session), pty_(this) {
    connect(&pty_, &PtySession::dataReady, this,
            [this](const QByteArray &bytes) { emit output(owner_, bytes); });
    connect(&pty_, &PtySession::exited, this, [this](int code) {
        if (!owner_.isEmpty())
            try {
                session_.store().audit(owner_, "terminal.exit", "ALLOW",
                                       QString("exit=%1").arg(code));
            } catch (const std::exception &) {
            }
        emit stopped(owner_, code);
    });
}
void TerminalService::stop() {
    pty_.stop();
}
QJsonObject TerminalService::call(const QString &method, const QJsonObject &args) {
    session_.requireScope(args);
    if (method == "pty-stop") {
        if (!owner_.isEmpty() && owner_ != session_.id())
            throw std::runtime_error("Terminal belongs to another workspace");
        stop();
        return {{"stopped", true}};
    }
    session_.authorize("terminal.execute", args);
    if (method == "pty-start") {
        if (pty_.running())
            throw std::runtime_error("The native terminal is already running");
        const auto shell = args["shell"].toString("pwsh");
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
            throw std::runtime_error("Unknown native shell profile");
#else
        Q_UNUSED(shell);
        throw std::runtime_error("Native PTY transport is currently Windows-only");
#endif
        owner_ = session_.id();
        if (!pty_.start(program, options, session_.root(), args["columns"].toInt(100),
                        args["rows"].toInt(30)))
            throw std::runtime_error(pty_.error().toStdString());
        session_.store().audit(owner_, "terminal.start", "ALLOW",
                               "Native PTY shell started; terminal content not persisted");
        return {{"running", true}, {"workspaceId", owner_}};
    }
    if (owner_ != session_.id())
        throw std::runtime_error("Terminal workspace mismatch");
    if (method == "pty-write") {
        const auto encoded = args["dataBase64"].toString().toLatin1();
        if (encoded.size() > 90000)
            throw std::runtime_error("Terminal input exceeds limit");
        const auto decoded =
            QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || !pty_.write(decoded.decoded))
            throw std::runtime_error("Terminal input rejected or backpressured");
        return {{"accepted", true}};
    }
    if (method == "pty-resize") {
        if (!pty_.resize(args["columns"].toInt(), args["rows"].toInt()))
            throw std::runtime_error("Terminal resize rejected");
        return {{"resized", true}};
    }
    throw std::runtime_error("Unknown native PTY operation");
}
} // namespace mterm
