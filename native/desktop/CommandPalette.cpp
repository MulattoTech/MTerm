// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "CommandPalette.h"
#include "ui/Theme.h"
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
namespace mterm {
CommandPalette::CommandPalette(QList<PaletteCommand> commands, QWidget *parent)
    : QDialog(parent), commands_(std::move(commands)) {
    setObjectName("command-palette");
    setWindowTitle("MTerm commands");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    resize(600, 440);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    search_ = new QLineEdit(this);
    search_->setObjectName("command-search");
    search_->setPlaceholderText("What would you like to do?");
    search_->setAccessibleName("Search MTerm commands");
    layout->addWidget(search_);
    list_ = new QListWidget(this);
    list_->setObjectName("command-results");
    layout->addWidget(list_, 1);
    auto *hint = new QLabel("Type to filter    ↑ ↓ navigate    Enter open    Esc close", this);
    hint->setProperty("role", "muted");
    layout->addWidget(hint);
    connect(search_, &QLineEdit::textChanged, this, &CommandPalette::filter);
    connect(search_, &QLineEdit::returnPressed, this, &CommandPalette::choose);
    connect(list_, &QListWidget::itemActivated, this, [this] { choose(); });
    search_->installEventFilter(this);
    filter();
}
void CommandPalette::present() {
    search_->clear();
    filter();
    open();
    search_->setFocus();
}
void CommandPalette::filter() {
    list_->clear();
    const auto query = search_->text().trimmed();
    for (const auto &c : commands_) {
        if (!query.isEmpty() && !(c.title + " " + c.detail).contains(query, Qt::CaseInsensitive))
            continue;
        auto *item = new QListWidgetItem(c.title + "\n" + c.detail, list_);
        item->setData(Qt::UserRole, c.id);
    }
    if (list_->count())
        list_->setCurrentRow(0);
}
void CommandPalette::choose() {
    if (auto *item = list_->currentItem()) {
        const auto id = item->data(Qt::UserRole).toString();
        accept();
        emit commandChosen(id);
    }
}
bool CommandPalette::eventFilter(QObject *o, QEvent *event) {
    if (o == search_ && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up) {
            const int delta = key->key() == Qt::Key_Down ? 1 : -1;
            list_->setCurrentRow(
                qBound(0, list_->currentRow() + delta, qMax(0, list_->count() - 1)));
            return true;
        }
    }
    return QDialog::eventFilter(o, event);
}
} // namespace mterm
