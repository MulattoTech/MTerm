// Modified: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro); see docs/ai/changes/.
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "WorkspaceShell.h"
#include "ui/Theme.h"
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
namespace mterm {
namespace {
QLabel *label(const QString &text, const char *role, QWidget *parent) {
    auto *l = new QLabel(text, parent);
    l->setProperty("role", role);
    return l;
}
QPushButton *action(const QString &text, const QString &id, const char *role, QWidget *parent) {
    auto *b = new QPushButton(text, parent);
    b->setObjectName(id);
    b->setProperty("role", role);
    b->setCursor(Qt::PointingHandCursor);
    b->setAccessibleName(text);
    return b;
}
} // namespace
WorkspaceShell::WorkspaceShell(QWidget *parent) : QWidget(parent) {
    setObjectName("app-root");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto *top = new QFrame(this);
    top->setObjectName("topbar");
    top->setFixedHeight(68);
    auto *topRow = new QHBoxLayout(top);
    topRow->setContentsMargins(16, 10, 16, 10);
    topRow->setSpacing(14);
    auto *brand = new QWidget(top);
    brand->setFixedWidth(176);
    auto *br = new QHBoxLayout(brand);
    br->setContentsMargins(0, 0, 0, 0);
    br->setSpacing(10);
    auto *mark = new QLabel("M", brand);
    mark->setObjectName("brand-mark");
    mark->setAlignment(Qt::AlignCenter);
    mark->setFixedSize(36, 36);
    br->addWidget(mark);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    brandText->addWidget(label("MTerm", "brand", brand));
    brandText->addWidget(label("Your AI workspace", "muted", brand));
    br->addLayout(brandText);
    br->addStretch();
    topRow->addWidget(brand);
    path_ = new QLineEdit(top);
    path_->setObjectName("workspace-path");
    path_->setReadOnly(true);
    path_->setPlaceholderText("Open a workspace to begin");
    path_->setMinimumWidth(140);
    topRow->addWidget(path_, 1);
    auto *native = label("NATIVE", "muted", top);
    native->setObjectName("native-badge");
    native->setFixedHeight(24);
    topRow->addWidget(native);
    auto *command = action("Search commands   Ctrl K", "open-command-palette", "quiet", top);
    command->setIcon(ui::icon("search"));
    topRow->addWidget(command);
    connect(command, &QPushButton::clicked, this, &WorkspaceShell::paletteRequested);
    canvasMode_ = action("Canvas", "view-canvas", "view", top);
    projectMode_ = action("Project", "view-project", "view", top);
    for (auto *b : {canvasMode_, projectMode_}) {
        b->setCheckable(true);
        topRow->addWidget(b);
    }
    canvasMode_->setChecked(true);
    canvasMode_->setIcon(ui::icon("canvas"));
    projectMode_->setIcon(ui::icon("project"));
    connect(canvasMode_, &QPushButton::clicked, this, [this] { emit projectRequested(false); });
    connect(projectMode_, &QPushButton::clicked, this, [this] { emit projectRequested(true); });
    root->addWidget(top);
    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    root->addLayout(body, 1);
    auto *side = new QFrame(this);
    side->setObjectName("app-sidebar");
    side->setFixedWidth(196);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(12, 16, 12, 12);
    sideLayout->setSpacing(6);
    auto *choose = action("Open workspace", "choose-workspace", "navigation", side);
    choose->setIcon(ui::icon("folder", ui::Accent));
    sideLayout->addWidget(choose);
    connect(choose, &QPushButton::clicked, this, &WorkspaceShell::openWorkspaceRequested);
    auto *scroll = new QScrollArea(side);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *navigation = new QWidget(scroll);
    auto *navLayout = new QVBoxLayout(navigation);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(3);
    navLayout->addWidget(label("CREATE", "section", navigation));
    for (const auto &c : QList<QPair<QString, QString>>{{"note", "Note"},
                                                        {"agent", "Agent"},
                                                        {"terminal", "Terminal"},
                                                        {"editor", "Editor"}}) {
        auto *b = action("+  " + c.second, "create-" + c.first, "create", navigation);
        navLayout->addWidget(b);
        connect(b, &QPushButton::clicked, this,
                [this, kind = c.first] { emit createRequested(kind); });
    }
    navLayout->addWidget(label("WORKSPACE", "section", navigation));
    for (const auto &c : QList<QPair<QString, QString>>{{"agent", "Agents"},
                                                        {"terminal", "Terminal"},
                                                        {"editor", "Editor"},
                                                        {"tasks", "Tasks"},
                                                        {"git", "Git / Diff"},
                                                        {"processes", "Processes"},
                                                        {"audit", "Audit"},
                                                        {"commands", "Commands"},
                                                        {"notes", "Notes"}}) {
        auto *b = action(c.second, "nav-" + c.first, "navigation", navigation);
        b->setIcon(ui::icon(c.first == "tasks"       ? "task"
                            : c.first == "processes" ? "process"
                                                     : c.first));
        b->setCheckable(true);
        navigation_[c.first] = b;
        navLayout->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, id = c.first] { emit toolRequested(id); });
    }
    navLayout->addStretch();
    scroll->setWidget(navigation);
    sideLayout->addWidget(scroll, 1);
    auto *permissions = new QFrame(side);
    permissions->setObjectName("permission-card");
    auto *pl = new QVBoxLayout(permissions);
    pl->setContentsMargins(12, 12, 12, 12);
    pl->setSpacing(8);
    profile_ = new QLabel("OBSERVE", permissions);
    profile_->setObjectName("profile-label");
    pl->addWidget(profile_, 0, Qt::AlignLeft);
    permission_ = label("Read-only workspace\nExecution requires approval.", "muted", permissions);
    permission_->setWordWrap(true);
    pl->addWidget(permission_);
    developer_ = new QCheckBox("Enable Developer", permissions);
    developer_->setObjectName("developer-profile");
    developer_->setToolTip(
        "Enable workspace editing. Terminal and agent execution still require separate approval.");
    pl->addWidget(developer_);
    sideLayout->addWidget(permissions);
    auto *sources = action("About & source projects", "source-projects", "quiet", side);
    sideLayout->addWidget(sources);
    connect(sources, &QPushButton::clicked, this, &WorkspaceShell::sourcesRequested);
    body->addWidget(side);
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setObjectName("workspace-splitter");
    splitter_->setHandleWidth(1);
    splitter_->setChildrenCollapsible(false);
    body->addWidget(splitter_, 1);
    auto *workspace = new QWidget(splitter_);
    auto *wl = new QVBoxLayout(workspace);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(0);
    auto *bar = new QFrame(workspace);
    bar->setObjectName("canvas-bar");
    auto *barRow = new QHBoxLayout(bar);
    barRow->setContentsMargins(20, 10, 14, 10);
    auto *title = label("Workspace", "title", bar);
    title->setObjectName("workspace-heading");
    barRow->addWidget(title);
    count_ = label("0 resources", "muted", bar);
    barRow->addWidget(count_);
    auto *resourceSearch = new QLineEdit(bar);
    resourceSearch->setObjectName("workspace-resource-search");
    resourceSearch->setPlaceholderText("Find resources…");
    resourceSearch->setClearButtonEnabled(true);
    resourceSearch->setMaximumWidth(200);
    resourceSearch->setMinimumWidth(95);
    resourceSearch->setAccessibleName("Find workspace resources");
    barRow->addWidget(resourceSearch);
    connect(resourceSearch, &QLineEdit::textChanged, this, &WorkspaceShell::resourceFilterChanged);
    barRow->addStretch();
    saveState_ = label("Local workspace", "muted", bar);
    barRow->addWidget(saveState_);
    auto *save = action("Save layout", "save-layout", "quiet", bar);
    barRow->addWidget(save);
    connect(save, &QPushButton::clicked, this, &WorkspaceShell::saveRequested);
    wl->addWidget(bar);
    primary_ = new QStackedWidget(workspace);
    primary_->setObjectName("workspace-view-stack");
    wl->addWidget(primary_, 1);
    workspace->setMinimumWidth(360);
    splitter_->addWidget(workspace);
    inspectorHost_ = new QWidget(splitter_);
    inspectorHost_->setMinimumWidth(380);
    auto *il = new QVBoxLayout(inspectorHost_);
    il->setContentsMargins(0, 0, 0, 0);
    il->setSpacing(0);
    auto *ih = new QFrame(inspectorHost_);
    ih->setObjectName("inspector-header");
    auto *ihl = new QVBoxLayout(ih);
    ihl->setContentsMargins(18, 15, 18, 14);
    ihl->setSpacing(4);
    inspectorTitle_ = label("Terminal", "title", ih);
    inspectorHint_ = label("A real native shell, in your workspace.", "muted", ih);
    inspectorHint_->setWordWrap(true);
    ihl->addWidget(inspectorTitle_);
    ihl->addWidget(inspectorHint_);
    il->addWidget(ih);
    splitter_->addWidget(inspectorHost_);
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    splitter_->setSizes({760, 500});
}
void WorkspaceShell::mount(QWidget *canvas, QWidget *project, QTabWidget *inspector) {
    primary_->addWidget(canvas);
    primary_->addWidget(project);
    inspector->tabBar()->hide();
    inspectorHost_->layout()->addWidget(inspector);
}
void WorkspaceShell::setWorkspace(const QString &root, bool developer, int resources) {
    path_->setText(root);
    path_->setToolTip(root);
    count_->setText(QString("%1 resources").arg(resources));
    profile_->setText(developer ? "DEVELOPER" : "OBSERVE");
    permission_->setText(developer ? "Workspace editing enabled.\nExecution still asks first."
                                   : "Read-only workspace.\nExecution requires approval.");
    QSignalBlocker blocker(developer_);
    developer_->setChecked(developer);
    for (const auto &kind : {"note", "agent", "terminal", "editor"})
        if (auto *b = findChild<QPushButton *>("create-" + QString(kind)))
            b->setEnabled(developer);
}
void WorkspaceShell::setInspector(const QString &id, const QString &title, const QString &hint) {
    for (auto it = navigation_.begin(); it != navigation_.end(); ++it)
        it.value()->setChecked(it.key() == id);
    inspectorTitle_->setText(title);
    inspectorHint_->setText(hint);
}
void WorkspaceShell::setProjectMode(bool project) {
    primary_->setCurrentIndex(project ? 1 : 0);
    canvasMode_->setChecked(!project);
    projectMode_->setChecked(project);
}
void WorkspaceShell::setSaveStatus(const QString &text) {
    saveState_->setText(text);
}
} // namespace mterm
