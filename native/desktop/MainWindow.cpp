// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-22-native-ux; docs/ai/changes/2026-09-22-native-ux.json
#include "MainWindow.h"
#include "CanvasView.h"
#include "CommandPalette.h"
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
#include <QFileDialog>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
namespace mterm {
MainWindow::MainWindow(QString databaseFile, QWidget *parent)
    : QMainWindow(parent), backend_(new Backend(std::move(databaseFile), this)) {
    ui::applyTheme(*qApp);
    setWindowTitle("MTerm — Your AI workspace");
    resize(1500, 940);
    setMinimumSize(1024, 720);
    createUi();
    connect(backend_, &Backend::response, this, &MainWindow::onResponse);
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
void MainWindow::closeEvent(QCloseEvent *event) {
    if (editor_->document()->isModified() &&
        QMessageBox::question(
            this, "Unsaved editor",
            "Discard unsaved editor changes and close MTerm? Active owned processes will stop.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        event->ignore();
        return;
    }
    if ((layoutDirty_ || pendingLayout_) && state_["profile"] == "developer") {
        closeAfterSave_ = true;
        flushLayout();
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
quint64 MainWindow::send(const QString &method, QJsonObject args) {
    args["workspaceId"] = workspaceId();
    const auto id = backend_->request(method, args);
    requestScopes_[id] = workspaceId();
    return id;
}
void MainWindow::openWorkspace(const QString &root) {
    if (editor_ && editor_->document()->isModified() &&
        QMessageBox::question(
            this, "Unsaved file", "Discard unsaved editor changes and open another workspace?",
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
    shell_ = new WorkspaceShell(this);
    developer_ = shell_->developerControl();
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName("workspace-tabs");
    createInspectorPanels();
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
    connect(developer_, &QCheckBox::toggled, this, [this](bool value) {
        if (!workspaceId().isEmpty())
            send("profile", {{"developer", value}});
    });
    connect(shell_, &WorkspaceShell::toolRequested, this, &MainWindow::selectTool);
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
    pendingCreate_ = send("add-node", {{"kind", kind},
                                       {"title", titles.value(kind, "Resource")},
                                       {"content", bodies.value(kind)}});
}
void MainWindow::inspectNode(const QJsonObject &node) {
    const auto kind = node["kind"].toString();
    if (kind == "note") {
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
    state_ = state;
    const bool dev = state["profile"] == "developer";
    shell_->setWorkspace(state["root"].toString(), dev,
                         state["canvas"].toObject()["nodes"].toArray().size());
    setWindowTitle("MTerm — " + state["root"].toString());
    for (const auto *name :
         {"add-note", "save-note", "save-layout", "add-task", "task-done", "save-file", "new-file"})
        if (auto *b = findChild<QPushButton *>(name))
            b->setEnabled(dev);
    editor_->setReadOnly(!dev);
    noteBody_->setReadOnly(!dev);
    noteTitle_->setReadOnly(!dev);
    if (switched) {
        editor_->clear();
        editor_->document()->setModified(false);
        fileVersion_.clear();
        loadedFile_.clear();
        filePath_->clear();
        selectedNodeId_.clear();
        directory_ = ".";
        fileBrowser_->clear();
        layoutDirty_ = false;
        pendingLayout_ = 0;
        projectMode_ = state["canvas"].toObject()["view"] == "project";
        shell_->setProjectMode(projectMode_);
    }
    if (layoutChanged && !layoutDirty_)
        canvas_->setCanvas(state["canvas"].toObject(), dev);
    project_->setCanvas(canvas_->canvas());
    const auto selectedTask =
        tasks_->currentItem() ? tasks_->currentItem()->data(0, Qt::UserRole).toString() : QString{};
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
    for (auto *pane : jobs_)
        pane->setWorkspace(state);
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
    } else if ((method == "read-file" || method == "write-file") && id == pendingFile_) {
        pendingFile_ = 0;
        loadedFile_ = result["path"].toString();
        filePath_->setText(loadedFile_);
        fileVersion_ = result["version"].toString();
        const auto text = result["text"].toString();
        if (method == "read-file") {
            editor_->setPlainText(text);
            editor_->document()->setModified(false);
        } else
            editor_->document()->setModified(editor_->toPlainText() != text);
        statusBar()->showMessage(method == "read-file" ? "File loaded · SHA-256 version captured"
                                 : editor_->document()->isModified()
                                     ? "Saved · newer editor changes remain unsaved"
                                     : "File saved atomically");
    } else if (method == "list-files" && id == pendingDirectory_) {
        fileBrowser_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto entry = v.toObject();
            auto *item = new QTreeWidgetItem(fileBrowser_, {entry["name"].toString()});
            item->setIcon(0, ui::icon(entry["directory"].toBool() ? "folder" : "note"));
            item->setData(0, Qt::UserRole, entry);
        }
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
void MainWindow::loadFile() {
    if (editor_->document()->isModified() &&
        QMessageBox::question(this, "Unsaved file", "Discard unsaved changes and open this file?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;
    pendingFile_ = send("read-file", {{"path", filePath_->text()}});
}
void MainWindow::listDirectory(const QString &path) {
    if (workspaceId().isEmpty())
        return;
    directory_ = path;
    directoryLabel_->setText(path);
    pendingDirectory_ = send("list-files", {{"path", path}});
}
void MainWindow::refreshCurrentTab() {
    if (workspaceId().isEmpty())
        return;
    if (tabs_->currentWidget() == processes_)
        send("processes");
    else if (tabs_->currentWidget() == audit_)
        send("audit");
    else if (tabs_->currentWidget()->objectName() == "editor-panel")
        listDirectory(directory_);
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
