// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "CanvasView.h"
#include "JobPane.h"
#include "MainWindow.h"
#include "TerminalPane.h"
#include "ui/Theme.h"
#include <QCheckBox>
#include <QDir>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
namespace mterm {
namespace {
QPushButton *button(const QString &text, const char *name, QWidget *parent, bool primary = false) {
    auto *b = new QPushButton(text, parent);
    b->setObjectName(name);
    b->setCursor(Qt::PointingHandCursor);
    if (primary)
        b->setProperty("role", "primary");
    return b;
}
QLabel *caption(const QString &text, QWidget *parent) {
    auto *l = new QLabel(text, parent);
    l->setProperty("role", "muted");
    l->setWordWrap(true);
    return l;
}
QTreeWidget *tree(QStringList headers, const char *name, QWidget *parent) {
    auto *t = new QTreeWidget(parent);
    t->setObjectName(name);
    t->setHeaderLabels(headers);
    t->setRootIsDecorated(false);
    t->setAlternatingRowColors(true);
    t->setUniformRowHeights(true);
    t->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    t->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    return t;
}
QVBoxLayout *panelLayout(QWidget *parent) {
    auto *l = new QVBoxLayout(parent);
    l->setContentsMargins(16, 16, 16, 16);
    l->setSpacing(10);
    return l;
}
} // namespace
void MainWindow::createInspectorPanels() {
    const auto add = [this](const QString &id, const QString &title,
                            std::function<QWidget *()> factory) {
        toolPages_[id] = tabs_->addTab(new QWidget(tabs_), title);
        panelFactories_.insert(id, std::move(factory));
    };
    add("tasks", "Tasks", [this] { return createTasksPanel(); });
    add("editor", "Editor", [this] { return createEditorPanel(); });
    add("notes", "Notes", [this] { return createNotesPanel(); });
    for (const auto &entry : QList<QPair<QString, QString>>{
             {"command", "Commands"}, {"git", "Git"}, {"codex", "Agents"}}) {
        const auto id = entry.first == "command" ? QString("commands")
                        : entry.first == "codex" ? QString("agent")
                                                 : entry.first;
        add(id, entry.second, [this, kind = entry.first] {
            auto *pane = new JobPane(kind, backend_, tabs_);
            pane->setObjectName("inspector-" + kind);
            jobs_.append(pane);
            return pane;
        });
    }
    add("terminal", "Terminal", [this] {
        terminal_ = new TerminalPane(backend_, tabs_);
        terminal_->setObjectName("terminal-panel");
        return terminal_;
    });
    add("processes", "Processes", [this] {
        processes_ =
            tree({"PID", "Name", "RAM MiB", "Private MiB", "CPU sec"}, "process-list", tabs_);
        processes_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        return processes_;
    });
    add("audit", "Audit", [this] {
        audit_ = tree({"UTC time", "Operation", "Decision", "Result"}, "audit-list", tabs_);
        audit_->header()->setSectionResizeMode(3, QHeaderView::Stretch);
        return audit_;
    });
}
void MainWindow::ensureInspector(const QString &id) {
    if (!panelFactories_.contains(id))
        return;
    const auto factory = panelFactories_.take(id);
    const int index = toolPages_.value(id);
    auto *placeholder = tabs_->widget(index);
    const auto label = tabs_->tabText(index);
    auto *panel = factory();
    {
        // Materialize once; changing visible tools must never recreate a live session or editor.
        QSignalBlocker blocker(tabs_);
        tabs_->removeTab(index);
        tabs_->insertTab(index, panel, label);
    }
    placeholder->deleteLater();
    if (!state_.isEmpty())
        applyState(state_);
}
QWidget *MainWindow::createTasksPanel() {
    auto *taskPage = new QWidget(tabs_);
    taskPage->setObjectName("tasks-panel");
    auto *taskLayout = panelLayout(taskPage);
    taskLayout->addWidget(caption("TITLE", taskPage));
    taskTitle_ = new QLineEdit(taskPage);
    taskTitle_->setObjectName("task-title");
    taskTitle_->setPlaceholderText("What needs to be done?");
    taskLayout->addWidget(taskTitle_);
    taskObjective_ = new QPlainTextEdit(taskPage);
    taskObjective_->setObjectName("task-objective");
    taskObjective_->setPlaceholderText("Describe the outcome and how to verify it…");
    taskObjective_->setMaximumHeight(104);
    taskLayout->addWidget(taskObjective_);
    auto *addTask = button("Create task", "add-task", taskPage, true);
    addTask->setEnabled(false);
    taskLayout->addWidget(addTask);
    taskLayout->addWidget(caption("WORKSPACE TASKS", taskPage));
    tasks_ = tree({"State", "Title", "Objective"}, "tasks-list", taskPage);
    tasks_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    taskLayout->addWidget(tasks_, 1);
    auto *done = button("Mark selected task done", "task-done", taskPage);
    done->setEnabled(false);
    taskLayout->addWidget(done);

    connect(addTask, &QPushButton::clicked, this, [this] {
        send("create-task",
             {{"title", taskTitle_->text()}, {"objective", taskObjective_->toPlainText()}});
    });
    connect(done, &QPushButton::clicked, this, [this] {
        if (auto *item = tasks_->currentItem())
            send("task-status",
                 {{"id", item->data(0, Qt::UserRole).toString()}, {"status", "DONE"}});
    });
    return taskPage;
}
QWidget *MainWindow::createEditorPanel() {
    auto *filePage = new QWidget(tabs_);
    filePage->setObjectName("editor-panel");
    auto *fl = panelLayout(filePage);
    filePath_ = new QLineEdit(filePage);
    filePath_->setObjectName("file-path");
    filePath_->setPlaceholderText("Workspace-relative path, for example README.md");
    fl->addWidget(filePath_);
    auto *fileButtons = new QHBoxLayout;
    auto *open = button("Load", "open-file", filePage),
         *save = button("Save", "save-file", filePage, true),
         *create = button("New file", "new-file", filePage);
    save->setEnabled(false);
    create->setEnabled(false);
    fileButtons->addWidget(open);
    fileButtons->addWidget(save);
    fileButtons->addWidget(create);
    fileButtons->addStretch();
    fl->addLayout(fileButtons);
    auto *split = new QSplitter(Qt::Horizontal, filePage);
    split->setChildrenCollapsible(false);
    split->setHandleWidth(1);
    auto *browserHost = new QWidget(split);
    auto *bl = new QVBoxLayout(browserHost);
    bl->setContentsMargins(0, 0, 8, 0);
    bl->setSpacing(7);
    auto *dirButtons = new QHBoxLayout;
    auto *up = button("↑", "directory-up", browserHost);
    up->setFixedWidth(33);
    directoryLabel_ = caption(".", browserHost);
    directoryLabel_->setToolTip("Current workspace directory");
    auto *refresh = button("↻", "directory-refresh", browserHost);
    refresh->setFixedWidth(33);
    dirButtons->addWidget(up);
    dirButtons->addWidget(directoryLabel_, 1);
    dirButtons->addWidget(refresh);
    bl->addLayout(dirButtons);
    fileBrowser_ = tree({"Files"}, "file-browser", browserHost);
    fileBrowser_->setHeaderHidden(true);
    fileBrowser_->setMinimumWidth(95);
    bl->addWidget(fileBrowser_, 1);
    browserHost->setMaximumWidth(185);
    split->addWidget(browserHost);
    editor_ = new QPlainTextEdit(split);
    editor_->setObjectName("file-editor");
    editor_->setPlaceholderText(
        "Choose a file to begin.\n\nYour editor stays open when you switch tools.");
    editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor_->setReadOnly(true);
    split->addWidget(editor_);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({135, 345});
    fl->addWidget(split, 1);
    fl->addWidget(caption("UTF-8 · conflict-safe saves · native editor", filePage));

    connect(open, &QPushButton::clicked, this, &MainWindow::loadFile);
    connect(filePath_, &QLineEdit::returnPressed, this, &MainWindow::loadFile);
    connect(save, &QPushButton::clicked, this, [this] {
        if (filePath_->text() != loadedFile_) {
            statusBar()->showMessage("Load the selected file, or choose New file, before saving.");
            return;
        }
        pendingFile_ = send(
            "write-file",
            {{"path", loadedFile_}, {"text", editor_->toPlainText()}, {"version", fileVersion_}});
    });
    connect(create, &QPushButton::clicked, this, [this] {
        if (filePath_->text().isEmpty())
            return;
        if (editor_->document()->isModified() &&
            QMessageBox::question(this, "Unsaved file", "Discard unsaved editor changes?",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes)
            return;
        loadedFile_ = filePath_->text();
        fileVersion_ = "missing";
        editor_->clear();
        editor_->document()->setModified(false);
        statusBar()->showMessage("New file draft · Save refuses to replace an existing target");
    });
    connect(up, &QPushButton::clicked, this, [this] {
        const auto slash = directory_.lastIndexOf('/');
        listDirectory(slash < 0 ? "." : directory_.left(slash));
    });
    connect(refresh, &QPushButton::clicked, this, [this] { listDirectory(directory_); });
    connect(fileBrowser_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        const auto entry = item->data(0, Qt::UserRole).toJsonObject();
        if (entry["directory"].toBool())
            listDirectory(entry["path"].toString());
        else {
            filePath_->setText(entry["path"].toString());
            loadFile();
        }
    });
    return filePage;
}
QWidget *MainWindow::createNotesPanel() {
    auto *notes = new QWidget(tabs_);
    notes->setObjectName("notes-panel");
    auto *nl = panelLayout(notes);
    nl->addWidget(caption("NOTE TITLE", notes));
    noteTitle_ = new QLineEdit("New note", notes);
    noteTitle_->setObjectName("note-title");
    nl->addWidget(noteTitle_);
    noteBody_ = new QPlainTextEdit(notes);
    noteBody_->setObjectName("note-content");
    noteBody_->setPlaceholderText("Keep decisions, context and next steps beside your work.");
    nl->addWidget(noteBody_, 1);
    auto *saveNoteButton = button("Save note", "save-note", notes, true),
         *addNote = button("Add as new note", "add-note", notes);
    nl->addWidget(saveNoteButton);
    nl->addWidget(addNote);

    connect(saveNoteButton, &QPushButton::clicked, this, &MainWindow::saveNote);
    connect(addNote, &QPushButton::clicked, this, [this] {
        pendingCreate_ = send("add-node", {{"kind", "note"},
                                           {"title", noteTitle_->text()},
                                           {"content", noteBody_->toPlainText()}});
    });
    return notes;
}
} // namespace mterm
