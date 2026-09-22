// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-native-ux; compact native inspector presentation.
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "JobPane.h"
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>
namespace mterm {
JobPane::JobPane(QString kind, Backend *backend, QWidget *parent)
    : QWidget(parent), kind_(std::move(kind)), backend_(backend) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    status_ = new QLabel(this);
    status_->setWordWrap(true);
    status_->setProperty("role", "muted");
    layout->addWidget(status_);
    if (kind_ == "command") {
        status_->setText(
            "Captured command runner, not an interactive terminal. Enter an executable and a JSON "
            "argument array. Shells require explicit selection.");
        program_ = new QLineEdit(this);
        program_->setPlaceholderText("Executable, for example git or pwsh.exe");
        args_ = new QLineEdit("[]", this);
        args_->setPlaceholderText("JSON array: [\"--version\"]");
        layout->addWidget(program_);
        layout->addWidget(args_);
    } else if (kind_ == "git") {
        status_->setText("Native read-only Git. Mutations and worktree lifecycle remain available "
                         "in the reference implementation.");
        gitAction_ = new QComboBox(this);
        gitAction_->addItems({"status", "diff", "log", "worktrees"});
        layout->addWidget(gitAction_);
    } else {
        status_->setText("Codex native executable adapter: read-only sandbox, explicit approval. "
                         "Running a prompt consumes provider usage.");
        resume_ = new QComboBox(this);
        resume_->addItem("New Codex session", QString{});
        layout->addWidget(resume_);
        prompt_ = new QPlainTextEdit(this);
        prompt_->setPlaceholderText("Prompt â€” never sent automatically");
        prompt_->setMaximumHeight(160);
        layout->addWidget(prompt_);
    }
    auto *buttons = new QHBoxLayout;
    run_ = new QPushButton("Run", this);
    approve_ = new QPushButton("Approve execution for session", this);
    stop_ = new QPushButton("Stop owned run", this);
    stop_->setEnabled(false);
    buttons->addWidget(run_);

    buttons->addWidget(stop_);
    buttons->addStretch();
    layout->addLayout(buttons);
    run_->setText(kind_ == "codex" ? "Run agent" : kind_ == "git" ? "Refresh Git" : "Run command");
    run_->setProperty("role", "primary");
    stop_->setText("Stop");
    stop_->setProperty("role", "danger");
    approve_->setText("Allow execution for this session");
    layout->addWidget(approve_);
    if (kind_ == "git")
        approve_->hide();
    output_ = new QPlainTextEdit(this);
    output_->setReadOnly(true);
    output_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    output_->setMaximumBlockCount(2000);
    layout->addWidget(output_);
    connect(run_, &QPushButton::clicked, this, &JobPane::run);
    connect(approve_, &QPushButton::clicked, this, [this] {
        const auto cap = kind_ == "codex" ? "agent.execute" : "terminal.execute";
        if (QMessageBox::question(this, "Approve local execution",
                                  "Allow " + QString(cap) +
                                      " for this workspace session (up to 8 hours)?\nThis is "
                                      "OS-user execution, not an MTerm security sandbox.") !=
            QMessageBox::Yes)
            return;
        backend_->request("grant", {{"workspaceId", workspaceId_}, {"capability", cap}});
    });
    connect(stop_, &QPushButton::clicked, this, [this] {
        backend_->request("cancel", {{"workspaceId", workspaceId_}, {"runId", runId_}});
    });
    connect(
        backend_, &Backend::response, this,
        [this](quint64 id, const QString &method, const QJsonObject &result, const QString &error) {
            if (id == pendingRun_) {
                pendingRun_ = 0;
                if (!error.isEmpty()) {
                    status_->setText(error);
                    run_->setEnabled(true);
                    return;
                }
                runId_ = result["runId"].toString();
                stop_->setEnabled(true);
                status_->setText("Running " + runId_);
            }
            if (method == "grant" && error.isEmpty())
                status_->setText(
                    "Execution granted. Choose Run to start; nothing was launched automatically.");
        });
    connect(backend_, &Backend::stream, this,
            [this](const QString &workspace, const QString &id, const QString &text) {
                if (workspace == workspaceId_ && id == runId_)
                    append(text);
            });
    connect(backend_, &Backend::runFinished, this,
            [this](const QString &workspace, const QString &id, const QJsonObject &result) {
                if (workspace != workspaceId_ || id != runId_)
                    return;
                status_->setText(QString("Exit %1 Â· %2 ms Â· dropped %3 bytes Â· %4")
                                     .arg(result["exitCode"].toInt())
                                     .arg(result["durationMs"].toInteger())
                                     .arg(result["droppedBytes"].toInteger())
                                     .arg(result["error"].toString()));
                runId_.clear();
                stop_->setEnabled(false);
                run_->setEnabled(true);
                backend_->request("state", {{"workspaceId", workspaceId_}});
            });
}
void JobPane::setWorkspace(const QJsonObject &state) {
    const auto id = state["workspaceId"].toString();
    if (id != workspaceId_) {
        workspaceId_ = id;
        runId_.clear();
        pendingRun_ = 0;
        output_->clear();
        stop_->setEnabled(false);
        run_->setEnabled(!id.isEmpty());
    }
    approve_->setEnabled(state["profile"] == "developer");
    if (resume_) {
        const auto selected = resume_->currentData().toString();
        resume_->clear();
        resume_->addItem("New Codex session", QString{});
        for (const auto &v : state["sessions"].toArray()) {
            const auto s = v.toObject();
            if (s["providerId"] != "codex-cli" ||
                s["resumabilityData"].toObject()["threadId"].toString().isEmpty())
                continue;
            resume_->addItem(s["status"].toString() + " Â· " + s["id"].toString().left(12),
                             s["id"].toString());
        }
        const auto index = resume_->findData(selected);
        if (index >= 0)
            resume_->setCurrentIndex(index);
    }
}
void JobPane::run() {
    QJsonObject args{{"workspaceId", workspaceId_}, {"kind", kind_}};
    if (kind_ == "command") {
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(args_->text().toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isArray()) {
            status_->setText("Arguments must be a JSON array of strings");
            return;
        }
        args["program"] = program_->text();
        args["arguments"] = doc.array();
    } else if (kind_ == "git")
        args["action"] = gitAction_->currentText();
    else {
        args["prompt"] = prompt_->toPlainText();
        args["resumeId"] = resume_->currentData().toString();
    }
    output_->clear();
    run_->setEnabled(false);
    pendingRun_ = backend_->request("run", args);
}
void JobPane::append(const QString &text) {
    auto cursor = output_->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    constexpr int maxChars = 262144;
    const int excess = output_->document()->characterCount() - maxChars;
    if (excess > 0) {
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, excess);
        cursor.removeSelectedText();
    }
    cursor.movePosition(QTextCursor::End);
    output_->setTextCursor(cursor);
}
} // namespace mterm
