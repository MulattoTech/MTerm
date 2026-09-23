// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
#include "TerminalPane.h"
#include "TerminalWidget.h"
#include "ui/Theme.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QVBoxLayout>
namespace mterm {
TerminalPane::TerminalPane(Backend *backend, QWidget *parent) : QWidget(parent), backend_(backend) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    auto *pick = new QHBoxLayout;
    resources_ = new QComboBox(this);
    resources_->setObjectName("terminal-resource");
    resources_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    resources_->setMinimumContentsLength(14);
    resources_->setToolTip("Each terminal resource owns a separate native shell");
    create_ = new QPushButton("+", this);
    create_->setObjectName("new-terminal-resource");
    create_->setToolTip("Add a terminal resource to the workspace canvas");
    create_->setFixedWidth(34);
    release_ = new QPushButton("Close view", this);
    release_->setObjectName("close-terminal-view");
    release_->setToolTip("Release a stopped screen; its canvas resource and metadata remain");
    pick->addWidget(resources_, 1);
    pick->addWidget(create_);
    pick->addWidget(release_);
    layout->addLayout(pick);
    auto *row = new QHBoxLayout;
    shell_ = new QComboBox(this);
    shell_->setObjectName("terminal-shell");
    shell_->addItems({"pwsh", "powershell", "cmd"});
    start_ = new QPushButton("Start terminal", this);
    start_->setObjectName("start-pty");
    start_->setProperty("role", "primary");
    stop_ = new QPushButton("Stop", this);
    stop_->setObjectName("stop-pty");
    stop_->setProperty("role", "danger");
    row->addWidget(shell_, 1);
    row->addWidget(start_);
    row->addWidget(stop_);
    layout->addLayout(row);
    cwd_ = new QLineEdit(".", this);
    cwd_->setObjectName("terminal-cwd");
    cwd_->setPlaceholderText("Workspace-relative working directory");
    cwd_->setToolTip(
        "Existing workspace-relative directory. Shell execution still has your OS permissions.");
    layout->addWidget(cwd_);
    approve_ = new QPushButton("Allow terminal execution for this workspace session", this);
    approve_->setObjectName("approve-pty");
    layout->addWidget(approve_);
    status_ = new QLabel(this);
    status_->setObjectName("terminal-status");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    status_->setProperty("role", "muted");
    layout->addWidget(status_);
    screens_ = new QStackedWidget(this);
    screens_->setObjectName("terminal-screens");
    layout->addWidget(screens_, 1);
    choices();
    ensureView("default");
    controls();
    connect(resources_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0)
            selectResource(resources_->itemData(index).toString());
    });
    connect(create_, &QPushButton::clicked, this, &TerminalPane::createTerminalRequested);
    connect(release_, &QPushButton::clicked, this, [this] {
        const auto view = views_.value(selected_);
        if (!view || view->active || view->starting)
            return;
        // Explicit release never deletes a canvas node, persisted metadata or another live screen.
        views_.remove(view->id);
        screens_->removeWidget(view->screen);
        delete view->screen;
        if (!views_.isEmpty())
            selectResource(views_.keys().first());
        else
            selectResource("default");
        status_->setText(
            "Stopped screen released. The workspace resource and metadata are preserved.");
    });
    connect(approve_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "Approve workspace terminals",
                                  "Allow up to eight native shells for this workspace session (up "
                                  "to 8 hours)?\nEach shell runs with your OS permissions, not a "
                                  "filesystem sandbox. Nothing starts until you click Start.") !=
            QMessageBox::Yes)
            return;
        grant_ = backend_->request(
            "grant", {{"workspaceId", workspaceId_}, {"capability", "terminal.execute"}});
    });
    connect(shell_, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        if (auto v = views_.value(selected_); v && !v->active && !v->starting)
            v->shell = text;
    });
    connect(cwd_, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (auto v = views_.value(selected_); v && !v->active && !v->starting)
            v->cwd = text;
    });
    connect(start_, &QPushButton::clicked, this, [this] {
        const auto v = views_.value(selected_);
        if (!v || !developer_ || !approved_ || v->active || v->starting)
            return;
        v->shell = shell_->currentText();
        v->cwd = cwd_->text();
        v->screen->reset();
        v->run.clear();
        v->finishedRun.clear();
        v->starting = true;
        v->status = "STARTING";
        send("pty-start", v,
             {{"shell", v->shell},
              {"cwd", v->cwd},
              {"flowControl", true},
              {"columns", v->screen->columns()},
              {"rows", v->screen->rows()}});
        controls();
    });
    connect(stop_, &QPushButton::clicked, this, [this] {
        const auto v = views_.value(selected_);
        if (v && v->active)
            send("pty-stop", v);
    });
    connect(backend_, &Backend::terminalSessionOutput, this,
            [this](const QString &workspace, const QString &id, const QString &run,
                   const QByteArray &bytes) {
                const auto v = views_.value(id);
                if (workspace != workspaceId_ || !v || v->run != run)
                    return;
                v->screen->feed(bytes);
                // Acknowledge only after parsing the immutable run's bytes; stale final-exit ack is
                // harmless.
                backend_->request("pty-ack", {{"workspaceId", workspace},
                                              {"terminalId", id},
                                              {"runId", run},
                                              {"bytes", bytes.size()}});
            });
    connect(backend_, &Backend::terminalSessionChanged, this,
            [this](const QString &workspace, const QString &id, const QString &run,
                   const QJsonObject &value) {
                if (workspace == workspaceId_)
                    applyMetadata(id, run, value);
            });
    connect(backend_, &Backend::response, this,
            [this](quint64 id, const QString &, const QJsonObject &result, const QString &error) {
                if (id == grant_ && grant_) {
                    grant_ = 0;
                    approved_ = error.isEmpty();
                    controls();
                    if (!error.isEmpty())
                        status_->setText(error);
                    return;
                }
                if (id == listing_ && listing_) {
                    listing_ = 0;
                    if (error.isEmpty()) {
                        for (const auto &entry : result["items"].toArray()) {
                            const auto m = entry.toObject();
                            const auto key = m["terminalId"].toString();
                            metadata_[key] = m;
                            if (auto v = views_.value(key);
                                v && !v->active && !v->starting && v->run.isEmpty()) {
                                v->shell = m["shell"].toString("pwsh");
                                v->cwd = m["cwd"].toString(".");
                                v->status = m["status"].toString("STOPPED");
                            }
                        }
                        choices();
                        selectResource(selected_);
                    }
                    return;
                }
                if (!pending_.contains(id))
                    return;
                const auto request = pending_.take(id);
                const auto v = views_.value(request.resource);
                if (!v)
                    return;
                if (request.method == "pty-start") {
                    v->starting = false;
                    if (error.isEmpty())
                        applyMetadata(v->id, result["runId"].toString(), result);
                    else {
                        v->status = "FAILED";
                        v->active = false;
                    }
                    controls();
                    if (error.isEmpty() && selected_ == v->id && v->active)
                        v->screen->setFocus();
                }
                if (!error.isEmpty() && selected_ == request.resource) {
                    controls();
                    status_->setText(error);
                }
            });
}
std::shared_ptr<TerminalPane::View> TerminalPane::ensureView(const QString &id) {
    if (auto existing = views_.value(id))
        return existing;
    if (views_.size() >= MaxViews) {
        status_->setText("Eight terminal screens are retained. Close a stopped view before opening "
                         "another; running shells are untouched.");
        return {};
    }
    auto v = std::make_shared<View>();
    v->id = id;
    v->title = "Workspace terminal";
    for (const auto &n : nodes_)
        if (n.toObject()["id"] == id)
            v->title = n.toObject()["title"].toString("Terminal");
    if (metadata_.contains(id)) {
        auto m = metadata_.value(id);
        v->shell = m["shell"].toString("pwsh");
        v->cwd = m["cwd"].toString(".");
        v->status = m["status"].toString("STOPPED");
    }
    v->screen = new TerminalWidget(screens_);
    v->screen->setProperty("terminalResourceId", id);
    v->screen->setObjectName(id == "default" ? "terminal-screen" : "terminal-screen-" + id);
    views_.insert(id, v);
    screens_->addWidget(v->screen);
    connect(v->screen, &TerminalWidget::input, this,
            [this, id](const QByteArray &bytes) { input(id, bytes); });
    connect(v->screen, &TerminalWidget::dimensionsChanged, this, [this, id](int columns, int rows) {
        if (auto current = views_.value(id); current && current->active)
            send("pty-resize", current, {{"columns", columns}, {"rows", rows}});
    });
    connect(v->screen, &TerminalWidget::parsingFailed, this, [this, id](const QString &error) {
        if (auto current = views_.value(id); current && current->active)
            send("pty-stop", current);
        if (selected_ == id)
            status_->setText(error);
    });
    return v;
}
quint64 TerminalPane::send(const QString &method, const std::shared_ptr<View> &v,
                           QJsonObject args) {
    args["workspaceId"] = workspaceId_;
    args["terminalId"] = v->id;
    if (method != "pty-start")
        args["runId"] = v->run;
    const auto id = backend_->request(method, args);
    pending_[id] = {v->id, method, v->run};
    return id;
}
void TerminalPane::input(const QString &id, const QByteArray &bytes) {
    if (auto v = views_.value(id); v && v->active && developer_ && approved_)
        send("pty-write", v, {{"dataBase64", QString::fromLatin1(bytes.toBase64())}});
}
void TerminalPane::choices() {
    QSignalBlocker block(resources_);
    resources_->clear();
    const auto add = [this](const QString &id, const QString &title) {
        QString state = views_.contains(id) ? views_.value(id)->status
                                            : metadata_.value(id)["status"].toString("IDLE");
        resources_->addItem(ui::icon("terminal"), title + " · " + state, id);
    };
    add("default", "Workspace terminal");
    for (const auto &value : nodes_) {
        const auto n = value.toObject();
        if (n["kind"] == "terminal")
            add(n["id"].toString(), n["title"].toString("Terminal"));
    }
    // A removed card never makes its still-owned live shell impossible to stop.
    for (auto it = views_.begin(); it != views_.end(); ++it)
        if (resources_->findData(it.key()) < 0 && (it.value()->active || it.value()->starting))
            add(it.key(), it.value()->title + " (removed card)");
    resources_->setCurrentIndex(qMax(0, resources_->findData(selected_)));
}
void TerminalPane::selectResource(const QString &id) {
    if (resources_->findData(id) < 0)
        return;
    auto v = ensureView(id);
    if (!v) {
        QSignalBlocker blocker(resources_);
        resources_->setCurrentIndex(resources_->findData(selected_));
        return;
    }
    selected_ = id;
    screens_->setCurrentWidget(v->screen);
    {
        QSignalBlocker a(resources_), b(shell_), c(cwd_);
        resources_->setCurrentIndex(resources_->findData(id));
        shell_->setCurrentText(v->shell);
        cwd_->setText(v->cwd);
    }
    controls();
}
void TerminalPane::applyMetadata(const QString &id, const QString &run, const QJsonObject &value) {
    const auto v = views_.value(id);
    if (!v)
        return;
    const auto state = value["status"].toString();
    if (state == "STARTING" && v->starting) {
        v->run = run;
        v->finishedRun.clear();
    }
    if (v->run != run)
        return;
    // A quick exit can arrive before start's reply. Never turn its badge back to RUNNING.
    if (v->finishedRun == run && (state == "STARTING" || state == "RUNNING"))
        return;
    metadata_[id] = value;
    v->status = state;
    v->active = state == "RUNNING";
    if (state == "STOPPED" || state == "EXITED" || state == "FAILED") {
        v->finishedRun = run;
        v->active = false;
    }
    choices();
    controls();
}
void TerminalPane::controls() {
    const auto v = views_.value(selected_);
    const bool busy = v && (v->active || v->starting);
    start_->setEnabled(developer_ && approved_ && v && !busy);
    stop_->setEnabled(v && v->active);
    shell_->setEnabled(v && !busy);
    cwd_->setEnabled(v && !busy);
    release_->setEnabled(v && !busy);
    create_->setEnabled(developer_);
    approve_->setEnabled(developer_ && !approved_ && !grant_);
    if (!developer_)
        status_->setText("Observe mode. Enable Developer and approve execution to start a shell.");
    else if (v && v->active)
        status_->setText("Native shell running · " + v->title +
                         " · other terminals remain independent.");
    else if (v && v->starting)
        status_->setText("Starting this native terminal…");
    else if (!approved_)
        status_->setText(
            "Approve this workspace's terminal session. Selecting a card never starts a process.");
    else if (v)
        status_->setText(
            v->status == "IDLE"
                ? "Terminal approved. Choose the shell and working directory, then Start terminal."
                : v->status + " · metadata preserved; no detached process is claimed to survive.");
}
void TerminalPane::setWorkspace(const QJsonObject &state) {
    const auto id = state["workspaceId"].toString(), epoch = state["sessionEpoch"].toString();
    if (id != workspaceId_ || epoch != epoch_) {
        workspaceId_ = id;
        epoch_ = epoch;
        approved_ = false;
        grant_ = 0;
        listing_ = 0;
        pending_.clear();
        metadata_.clear();
        selected_ = "default";
        for (const auto &v : views_) {
            screens_->removeWidget(v->screen);
            delete v->screen;
        }
        views_.clear();
        listing_ = backend_->request("pty-list", {{"workspaceId", workspaceId_}});
    }
    developer_ = state["profile"] == "developer";
    if (!developer_)
        approved_ = false;
    nodes_ = state["canvas"].toObject()["nodes"].toArray();
    for (const auto &n : nodes_)
        if (auto v = views_.value(n.toObject()["id"].toString()))
            v->title = n.toObject()["title"].toString();
    choices();
    selectResource(resources_->findData(selected_) < 0 ? "default" : selected_);
}
} // namespace mterm
