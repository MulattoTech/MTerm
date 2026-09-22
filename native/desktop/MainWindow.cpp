// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "MainWindow.h"
#include "CanvasView.h"
#include "JobPane.h"
#include "TerminalPane.h"
#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
namespace mterm {
namespace {
QPushButton *button(const QString &title, const QString &name, QWidget *parent) {
    auto *b = new QPushButton(title, parent);
    b->setObjectName(name);
    return b;
}
QTreeWidget *tree(QStringList headers, const QString &name, QWidget *parent) {
    auto *t = new QTreeWidget(parent);
    t->setObjectName(name);
    t->setHeaderLabels(headers);
    t->setRootIsDecorated(false);
    t->setAlternatingRowColors(true);
    t->setUniformRowHeights(true);
    return t;
}
} // namespace
MainWindow::MainWindow(QString databaseFile, QWidget *parent)
    : QMainWindow(parent), backend_(new Backend(std::move(databaseFile), this)) {
    setWindowTitle("MTerm — Native preview");
    resize(1220, 800);
    setMinimumSize(800, 520);
    createUi();
    connect(backend_, &Backend::response, this, &MainWindow::onResponse);
    statusBar()->showMessage("Open a trusted workspace. Observe is the default.");
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (editor_->document()->isModified() &&
        QMessageBox::question(
            this, "Unsaved editor",
            "Discard unsaved editor changes and close MTerm? Active owned processes will stop.") !=
            QMessageBox::Yes) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);
    if (!painted_) {
        painted_ = true;
        emit firstPaint();
    }
}
quint64 MainWindow::send(const QString &method, QJsonObject args) {
    args["workspaceId"] = workspaceId();
    return backend_->request(method, args);
}
void MainWindow::openWorkspace(const QString &root) {
    if (editor_ && editor_->document()->isModified() &&
        QMessageBox::question(this, "Unsaved file",
                              "Discard the unsaved editor changes and open another workspace?") !=
            QMessageBox::Yes)
        return;
    backend_->request("open", {{"root", root}});
}
void MainWindow::createUi() {
    auto *toolbar = addToolBar("Workspace");
    toolbar->setMovable(false);
    auto *open = toolbar->addAction("Open workspace");
    connect(open, &QAction::triggered, this, [this] {
        const auto root = QFileDialog::getExistingDirectory(this, "Choose trusted workspace");
        if (!root.isEmpty())
            openWorkspace(root);
    });
    developer_ = new QCheckBox("Developer", this);
    developer_->setObjectName("developer-profile");
    toolbar->addWidget(developer_);
    connect(developer_, &QCheckBox::toggled, this, [this](bool value) {
        if (!workspaceId().isEmpty())
            send("profile", {{"developer", value}});
    });
    auto *refresh = toolbar->addAction("Refresh");
    connect(refresh, &QAction::triggered, this, [this] {
        send("state");
        refreshCurrentTab();
    });
    workspaceLabel_ = new QLabel("  No workspace", this);
    workspaceLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toolbar->addWidget(workspaceLabel_);
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName("workspace-tabs");
    setCentralWidget(tabs_);
    // Canvas: lightweight resource cards, native scene indexing, no embedded browser engine.
    auto *canvasPage = new QWidget;
    auto *canvasLayout = new QVBoxLayout(canvasPage);
    auto *noteForm = new QHBoxLayout;
    noteTitle_ = new QLineEdit("Note", canvasPage);
    noteTitle_->setObjectName("note-title");
    noteTitle_->setMaximumWidth(200);
    noteText_ = new QLineEdit(canvasPage);
    noteText_->setPlaceholderText("Note text");
    auto *addNote = button("Add note", "add-note", canvasPage);
    addNote->setEnabled(false);
    auto *saveLayout = button("Save layout", "save-layout", canvasPage);
    saveLayout->setEnabled(false);
    noteForm->addWidget(noteTitle_);
    noteForm->addWidget(noteText_);
    noteForm->addWidget(addNote);
    noteForm->addWidget(saveLayout);
    canvasLayout->addLayout(noteForm);
    canvas_ = new CanvasView(canvasPage);
    canvasLayout->addWidget(canvas_);
    canvasLayout->addWidget(new QLabel(
        "Drag cards to arrange. Ctrl+wheel zooms. Save layout persists positions and viewport. "
        "Existing edge data is preserved; edge editing is not yet implemented.",
        canvasPage));
    tabs_->addTab(canvasPage, "Canvas");
    connect(addNote, &QPushButton::clicked, this, [this] {
        send("add-node", {{"title", noteTitle_->text()}, {"content", noteText_->text()}});
    });
    connect(saveLayout, &QPushButton::clicked, this, [this] {
        send("save-canvas", {{"canvas", canvas_->canvas()}, {"revision", state_["revision"]}});
    });
    connect(canvas_, &CanvasView::layoutEdited, this, [this] {
        statusBar()->showMessage("Layout changed — use Save layout to persist it.");
    });
    // Tasks: the native view reads the same workspace-scoped records as other services.
    auto *taskPage = new QWidget;
    auto *taskLayout = new QVBoxLayout(taskPage);
    taskTitle_ = new QLineEdit(taskPage);
    taskTitle_->setObjectName("task-title");
    taskTitle_->setPlaceholderText("Task title");
    taskObjective_ = new QPlainTextEdit(taskPage);
    taskObjective_->setPlaceholderText("Objective / acceptance intent");
    taskObjective_->setMaximumHeight(110);
    auto *addTask = button("Create task", "add-task", taskPage);
    addTask->setEnabled(false);
    taskLayout->addWidget(taskTitle_);
    taskLayout->addWidget(taskObjective_);
    taskLayout->addWidget(addTask);
    tasks_ = tree({"State", "Title", "Objective"}, "tasks-list", taskPage);
    taskLayout->addWidget(tasks_);
    auto *done = button("Mark selected task done", "task-done", taskPage);
    done->setEnabled(false);
    taskLayout->addWidget(done);
    tabs_->addTab(taskPage, "Tasks");
    connect(addTask, &QPushButton::clicked, this, [this] {
        send("create-task",
             {{"title", taskTitle_->text()}, {"objective", taskObjective_->toPlainText()}});
    });
    connect(done, &QPushButton::clicked, this, [this] {
        if (auto *item = tasks_->currentItem())
            send("task-status",
                 {{"id", item->data(0, Qt::UserRole).toString()}, {"status", "DONE"}});
    });
    // Files: bounded UTF-8 editor. Hash-based optimistic saves avoid silent overwrites.
    auto *filePage = new QWidget;
    auto *fileLayout = new QVBoxLayout(filePage);
    auto *row = new QHBoxLayout;
    filePath_ = new QLineEdit(filePage);
    filePath_->setObjectName("file-path");
    filePath_->setPlaceholderText("Workspace-relative path");
    auto *openFile = button("Open file", "open-file", filePage),
         *saveFile = button("Save file", "save-file", filePage),
         *newFile = button("New file", "new-file", filePage);
    saveFile->setEnabled(false);
    newFile->setEnabled(false);
    row->addWidget(filePath_);
    row->addWidget(openFile);
    row->addWidget(saveFile);
    row->addWidget(newFile);
    fileLayout->addLayout(row);
    editor_ = new QPlainTextEdit(filePage);
    editor_->setObjectName("file-editor");
    editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor_->setReadOnly(true);
    fileLayout->addWidget(editor_);
    fileLayout->addWidget(new QLabel("UTF-8 files up to 1 MB. Native syntax/LSP and full "
                                     "multi-buffer editing are not yet implemented.",
                                     filePage));
    tabs_->addTab(filePage, "Files");
    connect(openFile, &QPushButton::clicked, this, [this] {
        if (editor_->document()->isModified() &&
            QMessageBox::question(this, "Unsaved file", "Discard unsaved editor changes?") !=
                QMessageBox::Yes)
            return;
        send("read-file", {{"path", filePath_->text()}});
    });
    connect(saveFile, &QPushButton::clicked, this, [this] {
        if (filePath_->text() != loadedFile_) {
            statusBar()->showMessage(
                "Path changed: open the target or choose New file before saving.");
            return;
        }
        send("write-file",
             {{"path", loadedFile_}, {"text", editor_->toPlainText()}, {"version", fileVersion_}});
    });
    connect(newFile, &QPushButton::clicked, this, [this] {
        if (filePath_->text().isEmpty())
            return;
        if (editor_->document()->isModified() &&
            QMessageBox::question(this, "Unsaved file", "Discard unsaved editor changes?") !=
                QMessageBox::Yes)
            return;
        loadedFile_ = filePath_->text();
        fileVersion_ = "missing";
        editor_->clear();
        editor_->document()->setModified(false);
        statusBar()->showMessage("New file draft — Save will fail if the target already exists.");
    });
    for (const auto &entry : QList<QPair<QString, QString>>{
             {"command", "Commands"}, {"git", "Git"}, {"codex", "Agents"}}) {
        auto *pane = new JobPane(entry.first, backend_, this);
        jobs_.append(pane);
        tabs_->addTab(pane, entry.second);
    }
    terminal_ = new TerminalPane(backend_, this);
    tabs_->addTab(terminal_, "Terminal");
    processes_ = tree({"PID", "Name", "Working set MiB", "Private MiB", "CPU seconds"},
                      "process-list", this);
    tabs_->addTab(processes_, "Processes");
    audit_ = tree({"UTC time", "Operation", "Decision", "Result"}, "audit-list", this);
    tabs_->addTab(audit_, "Audit");
    connect(tabs_, &QTabWidget::currentChanged, this, [this] { refreshCurrentTab(); });
    auto *shortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(shortcut, &QShortcut::activated, this, [this] {
        QStringList commands;
        for (int i = 0; i < tabs_->count(); ++i)
            commands << tabs_->tabText(i);
        bool ok = false;
        const auto choice = QInputDialog::getItem(
            this, "MTerm command palette", "Focus workspace surface", commands, 0, false, &ok);
        if (ok)
            tabs_->setCurrentIndex(commands.indexOf(choice));
    });
}
void MainWindow::applyState(const QJsonObject &state) {
    const bool switched = workspaceId() != state["workspaceId"].toString();
    const bool changed = switched || state_["revision"] != state["revision"] ||
                         state_["profile"] != state["profile"];
    state_ = state;
    const bool developer = state["profile"] == "developer";
    {
        const QSignalBlocker blocker(developer_);
        developer_->setChecked(developer);
    }
    workspaceLabel_->setText("  " + state["root"].toString());
    setWindowTitle("MTerm — " + state["root"].toString());
    for (const auto &name :
         {"add-note", "save-layout", "add-task", "task-done", "save-file", "new-file"})
        if (auto *b = findChild<QPushButton *>(name))
            b->setEnabled(developer);
    editor_->setReadOnly(!developer);
    if (switched) {
        editor_->clear();
        editor_->document()->setModified(false);
        fileVersion_.clear();
        loadedFile_.clear();
        filePath_->clear();
    }
    if (changed)
        canvas_->setCanvas(state["canvas"].toObject(), developer);
    tasks_->clear();
    for (const auto &v : state["tasks"].toArray()) {
        const auto task = v.toObject();
        auto *item =
            new QTreeWidgetItem(tasks_, {task["status"].toString(), task["title"].toString(),
                                         task["objective"].toString()});
        item->setData(0, Qt::UserRole, task["id"].toString());
    }
    tasks_->resizeColumnToContents(0);
    tasks_->resizeColumnToContents(1);
    for (auto *pane : jobs_)
        pane->setWorkspace(state);
    terminal_->setWorkspace(state);
}
void MainWindow::onResponse(quint64, const QString &method, const QJsonObject &result,
                            const QString &error) {
    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        if (method == "profile") {
            const QSignalBlocker blocker(developer_);
            developer_->setChecked(state_["profile"] == "developer");
        }
        return;
    }
    if (result.contains("canvas"))
        applyState(result);
    if (method == "open") {
        statusBar()->showMessage(
            "Workspace ready — Observe profile. Ctrl+K opens the native command palette.");
        emit workspaceReady();
    } else if (method == "read-file" || method == "write-file") {
        loadedFile_ = result["path"].toString();
        filePath_->setText(loadedFile_);
        fileVersion_ = result["version"].toString();
        const auto saved = result["text"].toString();
        if (method == "read-file") {
            editor_->setPlainText(saved);
            editor_->document()->setModified(false);
        } else
            editor_->document()->setModified(editor_->toPlainText() != saved);
        statusBar()->showMessage(method == "write-file"
                                     ? (editor_->document()->isModified()
                                            ? "File saved; newer editor changes remain unsaved."
                                            : "File saved atomically.")
                                     : "File loaded with SHA-256 version.");
    } else if (method == "processes") {
        processes_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto p = v.toObject();
            const auto mb = [](const QJsonValue &v) {
                return v.isNull() ? QString("unavailable")
                                  : QString::number(v.toDouble() / 1048576, 'f', 1);
            };
            new QTreeWidgetItem(processes_,
                                {QString::number(p["pid"].toInteger()), p["name"].toString(),
                                 mb(p["workingSetBytes"]), mb(p["privateBytes"]),
                                 p["cpuSeconds"].isNull()
                                     ? QString("unavailable")
                                     : QString::number(p["cpuSeconds"].toDouble(), 'f', 2)});
        }
        processes_->resizeColumnToContents(0);
        processes_->resizeColumnToContents(1);
    } else if (method == "audit") {
        audit_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto a = v.toObject();
            new QTreeWidgetItem(audit_, {a["timestamp"].toString(), a["tool"].toString(),
                                         a["decision"].toString(), a["result"].toString()});
        }
        for (int i = 0; i < 3; ++i)
            audit_->resizeColumnToContents(i);
    } else if (!method.startsWith("pty-") && method != "run" && method != "provider")
        statusBar()->showMessage("Completed: " + method);
}
void MainWindow::refreshCurrentTab() {
    if (workspaceId().isEmpty())
        return;
    if (tabs_->currentWidget() == processes_)
        send("processes");
    else if (tabs_->currentWidget() == audit_)
        send("audit");
}
} // namespace mterm
