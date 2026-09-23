// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// Modified: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro); see docs/ai/changes/.
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-22-native-ux; docs/ai/changes/2026-09-22-native-ux.json
#include "MainWindow.h"
#include "CanvasView.h"
#include "CommandPalette.h"
#include "EditorDeck.h"
#include "JobPane.h"
#include "ProjectView.h"
#include "TerminalPane.h"
#include "WorkspaceShell.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
namespace mterm {
MainWindow::MainWindow(QString databaseFile, QWidget *parent)
    : QMainWindow(parent), backend_(new Backend(databaseFile, this)),
      uiSettingsFile_(databaseFile + ".window.ini") {
    QElapsedTimer phase;
    phase.start();
    ui::applyTheme(*qApp);
    setProperty("themeSetupMs", phase.elapsed());
    setWindowTitle("MTerm — Your AI workspace");
    resize(1500, 940);
    setMinimumSize(1024, 720);
    phase.restart();
    createUi();
    setProperty("uiConstructionMs", phase.elapsed());
    QSettings preferences(uiSettingsFile_, QSettings::IniFormat);
    if (preferences.contains("window/geometry"))
        restoreGeometry(preferences.value("window/geometry").toByteArray());
    if (auto *split = findChild<QSplitter *>("workspace-splitter"))
        split->restoreState(preferences.value("window/splitter").toByteArray());
    connect(backend_, &Backend::response, this, &MainWindow::onResponse);
    connect(backend_, &Backend::terminalSessionChanged, this,
            [this](const QString &workspace, const QString &id, const QString &run,
                   const QJsonObject &metadata) {
                if (workspace != workspaceId() || run.isEmpty())
                    return;
                const auto next = metadata["status"].toString();
                if (next == "STARTING" && terminalRuns_.value(id) != run)
                    terminalRuns_[id] = run;
                if (terminalRuns_.value(id) != run)
                    return;
                const auto prior = terminalStates_.value(id);
                if ((prior == "STOPPED" || prior == "EXITED" || prior == "FAILED") &&
                    next == "RUNNING")
                    return;
                terminalStates_[id] = next;
                canvas_->setRuntimeStatuses(terminalStates_);
                project_->setRuntimeStatuses(terminalStates_);
            });
    saveTimer_.setSingleShot(true);
    saveTimer_.setInterval(300);
    connect(&saveTimer_, &QTimer::timeout, this, &MainWindow::flushLayout);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->showMessage("Local workspace · Observe mode · Ctrl+K for commands");
}
void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);
    if (!painted_) {
        painted_ = true;
        emit firstPaint();
    }
}
void MainWindow::saveWindowPreferences() {
    QDir().mkpath(QFileInfo(uiSettingsFile_).absolutePath());
    QSettings preferences(uiSettingsFile_, QSettings::IniFormat);
    preferences.setValue("window/geometry", saveGeometry());
    if (auto *split = findChild<QSplitter *>("workspace-splitter"))
        preferences.setValue("window/splitter", split->saveState());
    preferences.sync();
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (editorDeck_ && editorDeck_->hasPendingWrites()) {
        statusBar()->showMessage("Wait for file saves to finish before closing.");
        event->ignore();
        return;
    }
    const auto signature = editorDeck_ ? editorDeck_->dirtySignature() : QByteArray{};
    if (editorDeck_ && editorDeck_->hasUnsavedChanges() && approvedCloseSignature_ != signature) {
        if (QMessageBox::question(this, "Unsaved editors",
                                  "Discard unsaved changes in these buffers and close?\n" +
                                      editorDeck_->dirtyPaths().join("\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes) {
            approvedCloseSignature_.clear();
            event->ignore();
            return;
        }
        approvedCloseSignature_ = signature;
    }
    if ((layoutDirty_ || pendingLayout_) && state_["profile"] == "developer") {
        closeAfterSave_ = true;
        flushLayout();
        event->ignore();
        return;
    }
    saveWindowPreferences();
    QMainWindow::closeEvent(event);
}

quint64 MainWindow::send(const QString &method, QJsonObject args) {
    args["workspaceId"] = workspaceId();
    const auto id = backend_->request(method, args);
    requestScopes_[id] = workspaceId();
    return id;
}
void MainWindow::openWorkspace(const QString &root) {
    if (editorDeck_ && editorDeck_->hasPendingWrites()) {
        statusBar()->showMessage("Wait for pending file saves before switching workspaces.");
        return;
    }
    if (editorDeck_ && editorDeck_->hasUnsavedChanges() &&
        QMessageBox::question(
            this, "Unsaved editors",
            "Discard changes in all unsaved buffers before opening another workspace?\n" +
                editorDeck_->dirtyPaths().join("\n"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    if (layoutDirty_ || pendingLayout_) {
        statusBar()->showMessage("Finish saving this layout before opening another workspace.");
        flushLayout();
        return;
    }
    backend_->request("open", {{"root", root}});
}
void MainWindow::createUi() {
    QElapsedTimer phase;
    phase.start();
    shell_ = new WorkspaceShell(this);
    developer_ = shell_->developerControl();
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName("workspace-tabs");
    setProperty("shellConstructionMs", phase.restart());
    createInspectorPanels();
    setProperty("inspectorsConstructionMs", phase.restart());
    auto *canvasPage = new QWidget(this);
    auto *cl = new QVBoxLayout(canvasPage);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);
    canvas_ = new CanvasView(canvasPage);
    cl->addWidget(canvas_, 1);
    auto *hint = new QLabel("  Drag to pan  ·  Shift-drag to select  ·  Ctrl+wheel to zoom  ·  "
                            "Double-click a card to inspect",
                            canvasPage);
    hint->setProperty("role", "muted");
    hint->setContentsMargins(10, 7, 10, 7);
    cl->addWidget(hint);
    project_ = new ProjectView(this);
    shell_->mount(canvasPage, project_, tabs_);
    setCentralWidget(shell_);
    setProperty("canvasConstructionMs", phase.restart());
    connect(developer_, &QCheckBox::toggled, this, [this](bool value) {
        if (!workspaceId().isEmpty())
            send("profile", {{"developer", value}});
    });
    connect(shell_, &WorkspaceShell::toolRequested, this, &MainWindow::selectTool);
    connect(shell_, &WorkspaceShell::resourceFilterChanged, this, [this](const QString &text) {
        canvas_->setFilter(text);
        project_->setFilter(text);
    });
    connect(shell_, &WorkspaceShell::createRequested, this, &MainWindow::createResource);
    connect(shell_, &WorkspaceShell::projectRequested, this, &MainWindow::setProjectMode);
    connect(shell_, &WorkspaceShell::saveRequested, this, &MainWindow::scheduleLayoutSave);
    connect(shell_, &WorkspaceShell::openWorkspaceRequested, this, [this] {
        const auto root = QFileDialog::getExistingDirectory(this, "Choose trusted workspace");
        if (!root.isEmpty())
            openWorkspace(root);
    });
    connect(shell_, &WorkspaceShell::sourcesRequested, this, &MainWindow::showSources);
    connect(canvas_, &CanvasView::layoutEdited, this, &MainWindow::scheduleLayoutSave);
    connect(canvas_, &CanvasView::nodeActivated, this, &MainWindow::inspectNode);
    connect(project_, &ProjectView::nodeActivated, this, &MainWindow::inspectNode);
    QList<PaletteCommand> commands;
    for (const auto &pair : QList<QPair<QString, QString>>{{"agent", "Agents"},
                                                           {"terminal", "Terminal"},
                                                           {"editor", "Editor"},
                                                           {"tasks", "Tasks"},
                                                           {"git", "Git / Diff"},
                                                           {"processes", "Processes"},
                                                           {"audit", "Audit"},
                                                           {"commands", "Commands"},
                                                           {"notes", "Notes"}})
        commands.append({"show:" + pair.first, "Show " + pair.second,
                         "Open the " + pair.second + " inspector without closing other tools"});
    for (const auto &kind : {"note", "agent", "terminal", "editor"})
        commands.append({"create:" + QString(kind), "New " + QString(kind),
                         "Add a workspace resource; execution still requires approval"});
    commands.append({"view:canvas", "Canvas mode", "Return to your spatial workspace"});
    commands.append({"view:project", "Project mode", "Browse the same resources as project cards"});
    palette_ = new CommandPalette(commands, this);
    connect(shell_, &WorkspaceShell::paletteRequested, palette_, &CommandPalette::present);
    connect(palette_, &CommandPalette::commandChosen, this, [this](const QString &id) {
        if (id.startsWith("show:"))
            selectTool(id.mid(5));
        else if (id.startsWith("create:"))
            createResource(id.mid(7));
        else
            setProjectMode(id == "view:project");
    });
    for (const auto &key : {"Ctrl+K", "Ctrl+Shift+P"})
        connect(new QShortcut(QKeySequence(key), this), &QShortcut::activated, palette_,
                &CommandPalette::present);
    connect(new QShortcut(QKeySequence::Save, this), &QShortcut::activated, this, [this] {
        if (tabs_->currentWidget()->objectName() == "editor-panel")
            findChild<QPushButton *>("save-file")->click();
        else
            scheduleLayoutSave();
    });
    connect(tabs_, &QTabWidget::currentChanged, this, [this] { refreshCurrentTab(); });
    selectTool("terminal");
}
void MainWindow::selectTool(const QString &id) {
    if (!toolPages_.contains(id))
        return;
    ensureInspector(id);
    tabs_->setCurrentIndex(toolPages_[id]);
    const QHash<QString, QString> titles{
        {"agent", "Agents"},      {"terminal", "Terminal"}, {"editor", "Editor"},
        {"tasks", "Tasks"},       {"git", "Git / Diff"},    {"processes", "Processes"},
        {"audit", "Audit trail"}, {"commands", "Commands"}, {"notes", "Note details"}};
    const QHash<QString, QString> hints{
        {"agent", "Local provider sessions. You decide when they run."},
        {"terminal", "A real native shell, kept alive when you switch views."},
        {"editor", "Workspace files, safe saves and native text editing."},
        {"tasks", "Objectives and progress, saved with your workspace."},
        {"git", "Read-only repository inspection. Nothing commits or pushes."},
        {"processes", "Native system process inventory, refreshed on demand."},
        {"audit", "A durable record of workspace operations and approvals."},
        {"commands", "Bounded command capture. Use Terminal for an interactive shell."},
        {"notes", "Persistent workspace notes. Select a card to edit it."}};
    shell_->setInspector(id, titles.value(id), hints.value(id));
    refreshCurrentTab();
}
void MainWindow::createResource(const QString &kind) {
    if (state_["profile"] != "developer") {
        statusBar()->showMessage("Enable Developer to add workspace resources.");
        return;
    }
    if (layoutDirty_ || pendingLayout_) {
        flushLayout();
        statusBar()->showMessage("Saving layout — add the resource when saved.");
        return;
    }
    const QHash<QString, QString> titles{{"note", "New note"},
                                         {"agent", "Agent session"},
                                         {"terminal", "Workspace terminal"},
                                         {"editor", "Workspace editor"}};
    const QHash<QString, QString> bodies{
        {"note",
         "Capture an idea, a decision or the next step. Select this card to edit your note."},
        {"agent", "Choose a provider session in the inspector. No prompt is sent until you "
                  "explicitly run it."},
        {"terminal",
         "A real native terminal in this workspace. Start a shell after session approval."},
        {"editor", "Read and edit workspace files with conflict-safe saves. Open the editor to "
                   "choose a file."}};
    auto title = titles.value(kind, "Resource");
    if (kind == "terminal") {
        QSet<QString> used;
        for (const auto &v : canvas_->canvas()["nodes"].toArray())
            used.insert(v.toObject()["title"].toString());
        int suffix = 1;
        while (used.contains(QString("Terminal %1").arg(suffix)))
            ++suffix;
        title = QString("Terminal %1").arg(suffix);
    }
    pendingCreate_ =
        send("add-node", {{"kind", kind}, {"title", title}, {"content", bodies.value(kind)}});
}
void MainWindow::inspectNode(const QJsonObject &node) {
    const auto kind = node["kind"].toString();
    if (kind == "terminal") {
        selectTool("terminal");
        terminal_->selectResource(node["id"].toString());
        return;
    }
    if (kind == "note") {
        ensureInspector("notes");
        selectedNodeId_ = node["id"].toString();
        noteTitle_->setText(node["title"].toString());
        noteBody_->setPlainText(node["content"].toString());
        selectTool("notes");
    } else
        selectTool(kind == "task"      ? "tasks"
                   : kind == "process" ? "processes"
                   : kind == "diff"    ? "git"
                   : kind == "file"    ? "editor"
                                       : kind);
}
void MainWindow::setProjectMode(bool project) {
    projectMode_ = project;
    project_->setCanvas(canvas_->canvas());
    canvas_->setRuntimeStatuses(terminalStates_);
    project_->setRuntimeStatuses(terminalStates_);
    shell_->setProjectMode(project);
    if (state_["profile"] == "developer")
        scheduleLayoutSave();
}
void MainWindow::scheduleLayoutSave() {
    if (state_["profile"] != "developer")
        return;
    layoutDirty_ = true;
    shell_->setSaveStatus("Unsaved layout");
    saveTimer_.start();
}
void MainWindow::flushLayout() {
    if (pendingLayout_ || !layoutDirty_ || workspaceId().isEmpty())
        return;
    saveTimer_.stop();
    auto canvas = canvas_->canvas();
    canvas["view"] = projectMode_ ? "project" : "canvas";
    layoutDirty_ = false;
    pendingLayout_ = send("save-canvas", {{"canvas", canvas}, {"revision", state_["revision"]}});
    shell_->setSaveStatus("Saving…");
}
void MainWindow::saveNote() {
    if (selectedNodeId_.isEmpty()) {
        statusBar()->showMessage("Select a note card first, or create a new note.");
        return;
    }
    auto value = canvas_->canvas();
    auto nodes = value["nodes"].toArray();
    for (qsizetype i = 0; i < nodes.size(); ++i) {
        auto n = nodes[i].toObject();
        if (n["id"] == selectedNodeId_) {
            n["title"] = noteTitle_->text();
            n["content"] = noteBody_->toPlainText();
            nodes[i] = n;
            value["nodes"] = nodes;
            canvas_->setCanvas(value, true);
            project_->setCanvas(value);
            scheduleLayoutSave();
            return;
        }
    }
}
void MainWindow::applyState(const QJsonObject &state) {
    const bool switched = workspaceId() != state["workspaceId"].toString();
    const bool layoutChanged = switched || state_["revision"] != state["revision"] ||
                               state_["profile"] != state["profile"];
    if (switched || state_["sessionEpoch"] != state["sessionEpoch"]) {
        terminalRuns_.clear();
        terminalStates_.clear();
    }
    state_ = state;
    const bool dev = state["profile"] == "developer";
    shell_->setWorkspace(state["root"].toString(), dev,
                         state["canvas"].toObject()["nodes"].toArray().size());
    setWindowTitle("MTerm — " + state["root"].toString());
    for (const auto *name : {"add-note", "save-note", "save-layout", "add-task", "task-done"})
        if (auto *b = findChild<QPushButton *>(name))
            b->setEnabled(dev);
    if (editorDeck_)
        editorDeck_->setWorkspace(state);
    if (noteBody_)
        noteBody_->setReadOnly(!dev);
    if (noteTitle_)
        noteTitle_->setReadOnly(!dev);
    if (switched) {
        selectedNodeId_.clear();
        approvedCloseSignature_.clear();
        layoutDirty_ = false;
        pendingLayout_ = 0;
        projectMode_ = state["canvas"].toObject()["view"] == "project";
        shell_->setProjectMode(projectMode_);
    }
    if (layoutChanged && !layoutDirty_)
        canvas_->setCanvas(state["canvas"].toObject(), dev);
    project_->setCanvas(canvas_->canvas());
    canvas_->setRuntimeStatuses(terminalStates_);
    project_->setRuntimeStatuses(terminalStates_);
    if (tasks_) {
        const auto selectedTask = tasks_->currentItem()
                                      ? tasks_->currentItem()->data(0, Qt::UserRole).toString()
                                      : QString{};
        tasks_->clear();
        for (const auto &v : state["tasks"].toArray()) {
            const auto task = v.toObject();
            auto *item =
                new QTreeWidgetItem(tasks_, {task["status"].toString(), task["title"].toString(),
                                             task["objective"].toString()});
            item->setData(0, Qt::UserRole, task["id"]);
            if (task["id"] == selectedTask)
                tasks_->setCurrentItem(item);
        }
        tasks_->resizeColumnToContents(0);
    }
    for (auto *pane : jobs_)
        pane->setWorkspace(state);
    if (terminal_)
        terminal_->setWorkspace(state);
}
void MainWindow::onResponse(quint64 id, const QString &method, const QJsonObject &result,
                            const QString &error) {
    const auto scope = requestScopes_.take(id);
    if (method != "open" && !scope.isEmpty() && scope != workspaceId())
        return;
    const bool savedLayout = id == pendingLayout_ && pendingLayout_ != 0;
    if (savedLayout)
        pendingLayout_ = 0;
    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        if (savedLayout) {
            layoutDirty_ = true;
            closeAfterSave_ = false;
            approvedCloseSignature_.clear();
            shell_->setSaveStatus("Save conflict — changes retained");
        }
        if (method == "profile") {
            QSignalBlocker block(developer_);
            developer_->setChecked(state_["profile"] == "developer");
        }
        return;
    }
    if (result.contains("canvas"))
        applyState(result);
    if (method == "profile")
        statusBar()->showMessage(
            state_["profile"] == "developer"
                ? "Developer mode · workspace editing enabled · execution still asks first"
                : "Observe mode · workspace read-only");
    if (savedLayout) {
        shell_->setSaveStatus(layoutDirty_ ? "Unsaved layout" : "All changes saved");
        if (layoutDirty_)
            saveTimer_.start();
        else if (closeAfterSave_) {
            closeAfterSave_ = false;
            QTimer::singleShot(0, this, &QWidget::close);
        }
    }
    if (method == "open") {
        statusBar()->showMessage("Workspace ready · Observe mode · Ctrl+K for commands");
        refreshCurrentTab();
        emit workspaceReady();
    } else if (method == "processes") {
        processes_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto p = v.toObject();
            auto mb = [](QJsonValue n) {
                return n.isNull() ? QString("—") : QString::number(n.toDouble() / 1048576, 'f', 1);
            };
            new QTreeWidgetItem(processes_,
                                {QString::number(p["pid"].toInteger()), p["name"].toString(),
                                 mb(p["workingSetBytes"]), mb(p["privateBytes"]),
                                 p["cpuSeconds"].isNull()
                                     ? "—"
                                     : QString::number(p["cpuSeconds"].toDouble(), 'f', 2)});
        }
    } else if (method == "audit") {
        audit_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto a = v.toObject();
            new QTreeWidgetItem(audit_, {a["timestamp"].toString(), a["tool"].toString(),
                                         a["decision"].toString(), a["result"].toString()});
        }
    }
    if (id == pendingCreate_ && pendingCreate_) {
        pendingCreate_ = 0;
        const auto nodes = result["canvas"].toObject()["nodes"].toArray();
        if (!nodes.isEmpty())
            inspectNode(nodes.last().toObject());
    }
}
void MainWindow::refreshCurrentTab() {
    if (workspaceId().isEmpty())
        return;
    if (tabs_->currentWidget() == processes_)
        send("processes");
    else if (tabs_->currentWidget() == audit_)
        send("audit");
    else if (tabs_->currentWidget()->objectName() == "editor-panel" && editorDeck_)
        editorDeck_->refreshDirectory();
}
void MainWindow::showSources() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("MTerm · Source projects and direction");
    dialog->resize(630, 510);
    auto *layout = new QVBoxLayout(dialog);
    auto *text = new QTextBrowser(dialog);
    text->setOpenExternalLinks(true);
    text->setHtml(
        "<h2>MTerm</h2><p>A native AI workspace, designed to become a primary interface for "
        "DevFleet. That integration is planned, not connected yet.</p><h3>Design "
        "lineage</h3><p>DesktopCommanderMCP — local tools and MCP<br>CodexPro — bounded workspace "
        "and remote bridging<br>NodeTerm — spatial terminals, agents, notes and "
        "worktrees<br>codex-chatgpt-web — optional browser-backed provider boundary<br>Rel.AI — "
        "agency, evidence, memory and observability<br>CodexFlow — project workbench, sessions and "
        "provider workflows</p><p>These are independent feature inspirations, not a claim that "
        "every capability is already implemented. See the in-repository source/license audit, "
        "source-project matrix and full backlog.</p><p>NodeTerm code/artwork is not copied. Native "
        "terminal parsing uses separately attributed MIT libvterm. Original upstream notices and "
        "AI change records are retained.</p>");
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);
    dialog->open();
}
} // namespace mterm
