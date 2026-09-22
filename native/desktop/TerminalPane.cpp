// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-native-ux; compact reference-style inspector layout.
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "TerminalPane.h"
#include "TerminalWidget.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
namespace mterm {
TerminalPane::TerminalPane(Backend *backend, QWidget *parent) : QWidget(parent), backend_(backend) {
    auto *layout = new QVBoxLayout(this);
    auto *row = new QHBoxLayout;
    shell_ = new QComboBox(this);
    shell_->setObjectName("terminal-shell");
    shell_->addItems({"pwsh", "powershell", "cmd"});
    approve_ = new QPushButton("Approve terminal session", this);
    approve_->setObjectName("approve-pty");
    start_ = new QPushButton("Start shell", this);
    start_->setObjectName("start-pty");
    stop_ = new QPushButton("Stop shell", this);
    stop_->setEnabled(false);
    start_->setEnabled(false);
    row->addWidget(shell_);

    row->addWidget(start_);
    row->addWidget(stop_);
    row->addStretch();
    layout->addLayout(row);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    approve_->setText("Allow terminal for this session");
    start_->setText("Start terminal");
    start_->setProperty("role", "primary");
    stop_->setText("Stop");
    stop_->setProperty("role", "danger");
    layout->addWidget(approve_);
    status_ = new QLabel("Native ConPTY + libvterm. Requires Developer plus session approval. "
                         "Shell processes end when MTerm closes.",
                         this);
    status_->setObjectName("terminal-status");
    status_->setWordWrap(true);
    status_->setProperty("role", "muted");
    layout->addWidget(status_);
    screen_ = new TerminalWidget(this);
    layout->addWidget(screen_, 1);
    connect(approve_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(
                this, "Approve terminal",
                "Allow terminal execution for this workspace session (up to 8 hours)?\nThe shell "
                "runs with your OS permissions; it is not confined by file-tool path rules.") !=
            QMessageBox::Yes)
            return;
        grant_ = backend_->request(
            "grant", {{"workspaceId", workspaceId_}, {"capability", "terminal.execute"}});
    });
    connect(start_, &QPushButton::clicked, this, [this] {
        screen_->reset();
        pending_ = backend_->request("pty-start", {{"workspaceId", workspaceId_},
                                                   {"shell", shell_->currentText()},
                                                   {"columns", screen_->columns()},
                                                   {"rows", screen_->rows()}});
    });
    connect(stop_, &QPushButton::clicked, this,
            [this] { backend_->request("pty-stop", {{"workspaceId", workspaceId_}}); });
    connect(screen_, &TerminalWidget::input, this, &TerminalPane::input);
    connect(screen_, &TerminalWidget::dimensionsChanged, this, [this](int cols, int rows) {
        if (active_)
            backend_->request("pty-resize",
                              {{"workspaceId", workspaceId_}, {"columns", cols}, {"rows", rows}});
    });
    connect(screen_, &TerminalWidget::parsingFailed, this, [this](const QString &error) {
        status_->setText(error);
        backend_->request("pty-stop", {{"workspaceId", workspaceId_}});
    });
    connect(backend_, &Backend::response, this,
            [this](quint64 id, const QString &method, const QJsonObject &, const QString &error) {
                if (id == grant_) {
                    grant_ = 0;
                    approved_ = error.isEmpty();
                    start_->setEnabled(developer_ && approved_ && !active_);
                    status_->setText(error.isEmpty()
                                         ? "Terminal approved. Choose Start shell to begin."
                                         : error);
                }
                if (id == pending_) {
                    pending_ = 0;
                    active_ = error.isEmpty();
                    start_->setEnabled(developer_ && approved_ && !active_);
                    stop_->setEnabled(active_);
                    status_->setText(
                        active_ ? "Native shell running â€” click the terminal to type." : error);
                    if (active_)
                        screen_->setFocus();
                }
                if (method.startsWith("pty-") && !error.isEmpty())
                    status_->setText(error);
            });
    connect(backend_, &Backend::terminalOutput, this,
            [this](const QString &workspace, const QByteArray &bytes) {
                if (workspace == workspaceId_)
                    screen_->feed(bytes);
            });
    connect(backend_, &Backend::terminalStopped, this, [this](const QString &workspace, int code) {
        if (workspace != workspaceId_)
            return;
        active_ = false;
        start_->setEnabled(developer_ && approved_);
        stop_->setEnabled(false);
        status_->setText(
            QString("Shell ended; exit=%1. No detached process is claimed to survive.").arg(code));
    });
}
void TerminalPane::setWorkspace(const QJsonObject &state) {
    const auto id = state["workspaceId"].toString();
    if (id != workspaceId_) {
        workspaceId_ = id;
        approved_ = false;
        active_ = false;
        pending_ = 0;
        grant_ = 0;
        screen_->reset();
        start_->setEnabled(developer_ && approved_);
        stop_->setEnabled(false);
    }
    developer_ = state["profile"] == "developer";
    if (!developer_)
        approved_ = false;
    approve_->setEnabled(developer_ && !approved_);
    start_->setEnabled(developer_ && approved_ && !active_ && !pending_);
    if (!developer_)
        status_->setText(
            "Enable Developer, then approve this terminal session. Nothing runs automatically.");
}
void TerminalPane::input(const QByteArray &bytes) {
    if (active_)
        backend_->request("pty-write", {{"workspaceId", workspaceId_},
                                        {"dataBase64", QString::fromLatin1(bytes.toBase64())}});
}
} // namespace mterm
