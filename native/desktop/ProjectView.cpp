// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "ProjectView.h"
#include "ResourceCard.h"
#include <QJsonArray>
#include <QPainter>
#include <QStyledItemDelegate>
namespace mterm {
class ResourceDelegate final : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *p, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        auto node = index.data(Qt::UserRole).toJsonObject();
        node["collapsed"] = false;
        paintResourceCard(*p, option.rect.adjusted(6, 6, -6, -6), node,
                          option.state & QStyle::State_Selected,
                          option.state & QStyle::State_MouseOver);
    }
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {308, 248};
    }
};
ProjectView::ProjectView(QWidget *parent) : QListWidget(parent) {
    setObjectName("project-view");
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(4);
    setUniformItemSizes(true);
    setMouseTracking(true);
    setItemDelegate(new ResourceDelegate(this));
    setGridSize({308, 248});
    connect(this, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit nodeActivated(item->data(Qt::UserRole).toJsonObject());
    });
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        emit nodeActivated(item->data(Qt::UserRole).toJsonObject());
    });
}
void ProjectView::setCanvas(const QJsonObject &canvas) {
    QString selected;
    if (currentItem())
        selected = currentItem()->data(Qt::UserRole).toJsonObject()["id"].toString();
    clear();
    for (const auto &value : canvas["nodes"].toArray()) {
        auto node = value.toObject();
        auto *item = new QListWidgetItem(node["title"].toString(), this);
        item->setData(Qt::UserRole, node);
        item->setSizeHint({308, 248});
        if (node["id"] == selected)
            setCurrentItem(item);
    }
}
} // namespace mterm
