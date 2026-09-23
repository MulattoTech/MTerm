// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro)
#include "EditorDeck.h"
#include "core/FileService.h"
#include "ui/Theme.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextDocumentLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace mterm {
namespace {
QPushButton *action(const QString &text, const char *name, QWidget *parent) {
    auto *b = new QPushButton(text, parent);
    b->setObjectName(name);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}
} // namespace
EditorDeck::EditorDeck(Backend *backend, QWidget *parent) : QWidget(parent), backend_(backend) {
    setObjectName("editor-panel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(9);
    path_ = new QLineEdit(this);
    path_->setObjectName("file-path");
    path_->setPlaceholderText("Open a workspace-relative file…");
    path_->setClearButtonEnabled(true);
    layout->addWidget(path_);
    auto *buttons = new QHBoxLayout;
    auto *open = action("Open", "open-file", this);
    save_ = action("Save", "save-file", this);
    save_->setProperty("role", "primary");
    new_ = action("New file", "new-file", this);
    auto *reload = action("Reload", "reload-file", this);
    buttons->addWidget(open);
    buttons->addWidget(save_);
    buttons->addWidget(new_);
    buttons->addWidget(reload);
    buttons->addStretch();
    layout->addLayout(buttons);
    tabs_ = new QTabBar(this);
    tabs_->setObjectName("editor-buffer-tabs");
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    tabs_->setExpanding(false);
    tabs_->setElideMode(Qt::ElideMiddle);
    tabs_->setDocumentMode(true);
    layout->addWidget(tabs_);
    auto *split = new QSplitter(Qt::Horizontal, this);
    split->setHandleWidth(1);
    split->setChildrenCollapsible(false);
    auto *browser = new QWidget(split);
    auto *bl = new QVBoxLayout(browser);
    bl->setContentsMargins(0, 0, 8, 0);
    bl->setSpacing(6);
    auto *row = new QHBoxLayout;
    auto *up = action("↑", "directory-up", browser);
    up->setFixedWidth(33);
    auto *refresh = action("↻", "directory-refresh", browser);
    refresh->setFixedWidth(33);
    directoryLabel_ = new QLabel(".", browser);
    directoryLabel_->setProperty("role", "muted");
    row->addWidget(up);
    row->addWidget(directoryLabel_, 1);
    row->addWidget(refresh);
    bl->addLayout(row);
    files_ = new QTreeWidget(browser);
    files_->setObjectName("file-browser");
    files_->setHeaderHidden(true);
    files_->setRootIsDecorated(false);
    files_->setUniformRowHeights(true);
    files_->setMinimumWidth(95);
    bl->addWidget(files_, 1);
    browser->setMaximumWidth(195);
    split->addWidget(browser);
    editor_ = new QPlainTextEdit(split);
    editor_->setObjectName("file-editor");
    editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor_->setPlaceholderText(
        "Open a file to start.\nEach tab retains edits, undo and cursor position.");
    editor_->setReadOnly(true);
    scratch_ = makeDocument({});
    editor_->setDocument(scratch_);
    split->addWidget(editor_);
    split->setStretchFactor(1, 1);
    split->setSizes({120, 365});
    layout->addWidget(split, 1);
    status_ = new QLabel("No file selected · UTF-8 · native editor", this);
    status_->setObjectName("editor-status");
    status_->setProperty("role", "muted");
    status_->setWordWrap(true);
    layout->addWidget(status_);
    connect(open, &QPushButton::clicked, this, [this] { openFile(); });
    connect(path_, &QLineEdit::returnPressed, this, [this] { openFile(); });
    connect(reload, &QPushButton::clicked, this, [this] { openFile(true); });
    connect(new_, &QPushButton::clicked, this, &EditorDeck::createDraft);
    connect(save_, &QPushButton::clicked, this, [this] { saveBuffer(active_); });
    connect(tabs_, &QTabBar::currentChanged, this, [this](int i) {
        if (!activating_ && i >= 0)
            activate(tabs_->tabData(i).toString());
    });
    connect(tabs_, &QTabBar::tabCloseRequested, this, &EditorDeck::closeBuffer);
    connect(up, &QPushButton::clicked, this, [this] {
        const int slash = directory_.lastIndexOf('/');
        listDirectory(slash < 0 ? "." : directory_.left(slash));
    });
    connect(refresh, &QPushButton::clicked, this, &EditorDeck::refreshDirectory);
    connect(files_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        const auto entry = item->data(0, Qt::UserRole).toJsonObject();
        if (entry["directory"].toBool())
            listDirectory(entry["path"].toString());
        else {
            path_->setText(entry["path"].toString());
            openFile();
        }
    });
    connect(tabs_, &QTabBar::tabBarClicked, this, [this](int i) {
        if (i >= 0)
            activate(tabs_->tabData(i).toString());
    });
    auto *closeTab = new QShortcut(QKeySequence("Ctrl+W"), this);
    closeTab->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeTab, &QShortcut::activated, this, [this] {
        if (tabs_->currentIndex() >= 0)
            closeBuffer(tabs_->currentIndex());
    });
    for (const auto &entry : QList<QPair<QString, int>>{{"Ctrl+Tab", 1}, {"Ctrl+Shift+Tab", -1}}) {
        auto *shortcut = new QShortcut(QKeySequence(entry.first), this);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated, this, [this, delta = entry.second] {
            if (tabs_->count())
                tabs_->setCurrentIndex((tabs_->currentIndex() + delta + tabs_->count()) %
                                       tabs_->count());
        });
    }
    connect(editor_, &QPlainTextEdit::cursorPositionChanged, this, &EditorDeck::updateLabels);
    connect(backend_, &Backend::response, this, &EditorDeck::receive);
    updateLabels();
}
QTextDocument *EditorDeck::makeDocument(const QString &text) {
    auto *doc = new QTextDocument(this);
    doc->setDocumentLayout(new QPlainTextDocumentLayout(doc));
    doc->setDefaultFont(editor_->font());
    doc->setPlainText(text);
    doc->setModified(false);
    connect(doc, &QTextDocument::modificationChanged, this, [this] { updateLabels(); });
    return doc;
}
QString EditorDeck::keyFor(const QString &path) {
#ifdef Q_OS_WIN
    return path.toCaseFolded();
#else
    return path;
#endif
}
QString EditorDeck::cleanRelative(QString path) {
    path = path.trimmed();
    path.replace('\\', '/');
    if (path.isEmpty() || path.startsWith('/') || path.contains(':') ||
        path.contains(QChar::Null) || path.split('/').contains(".."))
        return {};
    path = QDir::cleanPath(path);
    return path == "." ? QString{} : path;
}
EditorDeck::Buffer *EditorDeck::buffer(const QString &key) const {
    for (const auto &b : buffers_)
        if (b->key == key)
            return b.get();
    return nullptr;
}
void EditorDeck::activate(const QString &key) {
    auto *next = buffer(key);
    if (!next)
        return;
    if (auto *previous = buffer(active_)) {
        previous->cursor = editor_->textCursor();
        previous->vertical = editor_->verticalScrollBar()->value();
        previous->horizontal = editor_->horizontalScrollBar()->value();
    }
    ++selection_;
    active_ = key;
    activating_ = true;
    editor_->setDocument(next->document);
    if (next->cursor.isNull())
        next->cursor = QTextCursor(next->document);
    editor_->setTextCursor(next->cursor);
    editor_->verticalScrollBar()->setValue(next->vertical);
    editor_->horizontalScrollBar()->setValue(next->horizontal);
    path_->setText(next->path);
    for (int i = 0; i < tabs_->count(); ++i)
        if (tabs_->tabData(i) == key) {
            tabs_->setCurrentIndex(i);
            break;
        }
    activating_ = false;
    updateLabels();
}
void EditorDeck::updateLabels() {
    for (int i = 0; i < tabs_->count(); ++i)
        if (auto *b = buffer(tabs_->tabData(i).toString())) {
            tabs_->setTabText(i,
                              QFileInfo(b->path).fileName() + (b->saving                   ? " …"
                                                               : b->document->isModified() ? " *"
                                                                                           : ""));
            tabs_->setTabToolTip(i, b->path + (b->saving                   ? " · saving"
                                               : b->document->isModified() ? " · unsaved changes"
                                                                           : ""));
        }
    auto *b = buffer(active_);
    save_->setEnabled(developer_ && b && !b->saving);
    new_->setEnabled(developer_ && !workspace_.isEmpty());
    const auto cursor = editor_->textCursor();
    status_->setText(QString("%1 · %2/%3 files · Ln %4, Col %5 · UTF-8")
                         .arg(b ? (b->saving                   ? "Saving…"
                                   : b->document->isModified() ? "Unsaved changes"
                                                               : "Saved version")
                                : "No file selected")
                         .arg(buffers_.size())
                         .arg(MaxBuffers)
                         .arg(cursor.blockNumber() + 1)
                         .arg(cursor.positionInBlock() + 1));
}
void EditorDeck::setWorkspace(const QJsonObject &state) {
    const auto id = state["workspaceId"].toString();
    developer_ = state["profile"] == "developer";
    if (workspace_ != id) {
        ++generation_;
        ++selection_;
        workspace_ = id;
        pending_.clear();
        pendingDirectory_ = 0;
        active_.clear();
        editor_->setDocument(scratch_);
        scratch_->clear();
        scratch_->setModified(false);
        QSignalBlocker blocker(tabs_);
        while (tabs_->count())
            tabs_->removeTab(0);
        for (auto &b : buffers_)
            b->document->deleteLater();
        buffers_.clear();
        path_->clear();
        files_->clear();
        directory_ = ".";
    }
    editor_->setReadOnly(!developer_);
    updateLabels();
}
void EditorDeck::openFile(bool reload) {
    const auto path = cleanRelative(path_->text());
    if (path.isEmpty() || workspace_.isEmpty()) {
        emit message("Choose a valid workspace-relative file path.");
        return;
    }
    const auto key = keyFor(path);
    ++selection_;
    if (auto *existing = buffer(key)) {
        if (!reload) {
            activate(key);
            return;
        }
        if (existing->saving) {
            emit message("Wait for this file's save to finish before reloading.");
            return;
        }
        if (existing->document->isModified() &&
            QMessageBox::question(
                this, "Reload file", "Discard unsaved changes in " + existing->path + "?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
    } else {
        int loads = 0;
        for (const auto &p : pending_)
            if (p.method == "read-file" && !buffer(p.key))
                ++loads;
        if (int(buffers_.size()) + loads >= MaxBuffers) {
            emit message("Close an editor tab before opening another (12-file limit).");
            return;
        }
    }
    for (auto it = pending_.begin(); it != pending_.end(); ++it)
        if (it->method == "read-file" && it->key == key) {
            it->selection = selection_;
            emit message("This file is already loading.");
            return;
        }
    const auto id = backend_->request("read-file", {{"workspaceId", workspace_}, {"path", path}});
    pending_.insert(id, {"read-file",
                         key,
                         path,
                         generation_,
                         selection_,
                         {},
                         buffer(key) ? buffer(key)->document->revision() : -1});
    emit message("Loading " + path + "…");
}
void EditorDeck::createDraft() {
    if (!developer_)
        return;
    const auto path = cleanRelative(path_->text());
    if (path.isEmpty()) {
        emit message("Enter a new workspace-relative path.");
        return;
    }
    const auto key = keyFor(path);
    if (buffer(key)) {
        activate(key);
        return;
    }
    int loading = 0;
    for (const auto &pending : pending_)
        if (pending.method == "read-file") {
            if (pending.key == key) {
                emit message("Wait for the pending read before creating this draft.");
                return;
            }
            if (!buffer(pending.key))
                ++loading;
        }
    if (int(buffers_.size()) + loading >= MaxBuffers) {
        emit message("Close an editor tab before creating another.");
        return;
    }
    auto b = std::make_unique<Buffer>();
    b->key = key;
    b->path = path;
    b->version = "missing";
    b->document = makeDocument({});
    b->cursor = QTextCursor(b->document);
    b->document->setModified(true);
    buffers_.push_back(std::move(b));
    {
        QSignalBlocker blocker(tabs_);
        const int i = tabs_->addTab(QFileInfo(path).fileName());
        tabs_->setTabData(i, key);
    }
    ++selection_;
    activate(key);
    emit message("New file draft — Save will not overwrite an existing target.");
}
void EditorDeck::saveBuffer(const QString &key) {
    auto *b = buffer(key);
    if (!b || !developer_ || b->saving)
        return;
    const auto text = b->document->toPlainText();
    if (text.toUtf8().size() > FileService::MaxBytes) {
        emit message("File exceeds the 1 MB save limit. Changes remain in the editor.");
        return;
    }
    const auto id = backend_->request(
        "write-file",
        {{"workspaceId", workspace_}, {"path", b->path}, {"text", text}, {"version", b->version}});
    b->saving = true;
    pending_.insert(id, {"write-file", key, b->path, generation_, selection_, text});
    updateLabels();
}
void EditorDeck::closeBuffer(int index) {
    auto *b = buffer(tabs_->tabData(index).toString());
    if (!b)
        return;
    if (b->saving) {
        emit message("This file is saving; close it after the save finishes.");
        return;
    }
    for (const auto &p : pending_)
        if (p.key == b->key) {
            emit message("Wait for the file operation to finish before closing this tab.");
            return;
        }
    if (b->document->isModified() &&
        QMessageBox::question(this, "Unsaved file", "Discard unsaved changes in " + b->path + "?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;
    const auto key = b->key;
    const bool current = active_ == key;
    if (current) {
        active_.clear();
        editor_->setDocument(scratch_);
        path_->clear();
    }
    b->document->deleteLater();
    buffers_.erase(std::remove_if(buffers_.begin(), buffers_.end(),
                                  [&](const auto &x) { return x->key == key; }),
                   buffers_.end());
    {
        QSignalBlocker blocker(tabs_);
        tabs_->removeTab(index);
    }
    if (current && tabs_->count())
        activate(tabs_->tabData(qMin(index, tabs_->count() - 1)).toString());
    updateLabels();
}
void EditorDeck::listDirectory(const QString &path) {
    if (workspace_.isEmpty())
        return;
    directory_ = path;
    directoryLabel_->setText(path);
    pendingDirectory_ =
        backend_->request("list-files", {{"workspaceId", workspace_}, {"path", path}});
    pending_.insert(pendingDirectory_, {"list-files", {}, path, generation_, 0, {}});
}
void EditorDeck::refreshDirectory() {
    listDirectory(directory_);
}
void EditorDeck::receive(quint64 id, const QString &, const QJsonObject &result,
                         const QString &error) {
    if (!pending_.contains(id))
        return;
    const auto pending = pending_.take(id);
    if (pending.generation != generation_)
        return;
    auto *b = buffer(pending.key);
    if (pending.method == "write-file" && b)
        b->saving = false;
    if (!error.isEmpty()) {
        updateLabels();
        emit message(error + " — unsaved editor contents retained.");
        return;
    }
    if (pending.method == "list-files") {
        if (id != pendingDirectory_)
            return;
        files_->clear();
        for (const auto &v : result["items"].toArray()) {
            const auto entry = v.toObject();
            auto *item = new QTreeWidgetItem(files_, {entry["name"].toString()});
            item->setIcon(0, ui::icon(entry["directory"].toBool() ? "folder" : "note"));
            item->setData(0, Qt::UserRole, entry);
        }
        return;
    }
    if (!result["path"].isString() || !result["text"].isString() || !result["version"].isString() ||
        keyFor(cleanRelative(result["path"].toString())) != pending.key) {
        emit message("File response identity/shape mismatch; editor was not changed.");
        return;
    }
    if (pending.method == "read-file") {
        // A reload is a versioned UI operation too: a later keystroke must win over old I/O.
        if (b &&
            (pending.expectedRevision < 0 || b->document->revision() != pending.expectedRevision)) {
            emit message(
                "Reload response discarded because the buffer changed; newer edits are retained.");
            updateLabels();
            return;
        }
        if (!b) {
            auto owned = std::make_unique<Buffer>();
            owned->key = pending.key;
            owned->path = result["path"].toString();
            owned->version = result["version"].toString();
            owned->document = makeDocument(result["text"].toString());
            owned->cursor = QTextCursor(owned->document);
            b = owned.get();
            buffers_.push_back(std::move(owned));
            QSignalBlocker blocker(tabs_);
            const int i = tabs_->addTab(QFileInfo(b->path).fileName());
            tabs_->setTabData(i, b->key);
        } else {
            b->document->setPlainText(result["text"].toString());
            b->document->setModified(false);
            b->version = result["version"].toString();
            b->cursor = QTextCursor(b->document);
        }
        if (pending.selection == selection_)
            activate(b->key);
        emit message("Loaded " + b->path + " · per-file version captured");
    } else if (pending.method == "write-file" && b) {
        b->version = result["version"].toString();
        b->document->setModified(b->document->toPlainText() != pending.submitted);
        emit message("Saved " + b->path +
                     (b->document->isModified() ? " · newer edits remain unsaved" : " atomically"));
    }
    updateLabels();
}
bool EditorDeck::hasUnsavedChanges() const {
    if (scratch_->isModified())
        return true;
    for (const auto &b : buffers_)
        if (b->document->isModified())
            return true;
    return false;
}
bool EditorDeck::hasPendingWrites() const {
    for (const auto &b : buffers_)
        if (b->saving)
            return true;
    return false;
}
QStringList EditorDeck::dirtyPaths() const {
    QStringList paths;
    if (scratch_->isModified())
        paths << "Untitled draft";
    for (const auto &b : buffers_)
        if (b->document->isModified())
            paths << b->path;
    return paths;
}
QByteArray EditorDeck::dirtySignature() const {
    QByteArray data;
    if (scratch_->isModified())
        data = "scratch:" + QByteArray::number(scratch_->revision());
    for (const auto &b : buffers_)
        if (b->document->isModified())
            data += '\n' + b->key.toUtf8() + ':' + QByteArray::number(b->document->revision());
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}
} // namespace mterm
