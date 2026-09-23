// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
#pragma once
#include "services/Backend.h"
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
#include <memory>
class QComboBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QStackedWidget;
namespace mterm {
class TerminalWidget;
/// GUI-thread terminal resource controller; no OS handles or serialized terminal text.
/// Screen instances are retained by resource identity, bounded separately from canvas size.
class TerminalPane final : public QWidget {
    Q_OBJECT
  public:
    explicit TerminalPane(Backend *backend, QWidget *parent = nullptr);
    void setWorkspace(const QJsonObject &state);
    void selectResource(const QString &id);
  signals:
    void createTerminalRequested();

  private:
    struct View {
        QString id, title, run, finishedRun, shell = "pwsh", cwd = ".", status = "IDLE";
        TerminalWidget *screen = nullptr;
        bool active = false, starting = false;
    };
    struct Pending {
        QString resource, method, run;
    };
    static constexpr int MaxViews = 8;
    Backend *backend_;
    QComboBox *resources_, *shell_;
    QLineEdit *cwd_;
    QStackedWidget *screens_;
    QLabel *status_;
    QPushButton *start_, *approve_, *stop_, *release_, *create_;
    QString workspaceId_, epoch_, selected_ = "default";
    bool developer_ = false, approved_ = false;
    quint64 grant_ = 0, listing_ = 0;
    QJsonArray nodes_;
    QHash<QString, std::shared_ptr<View>> views_;
    QHash<QString, QJsonObject> metadata_;
    QHash<quint64, Pending> pending_;
    std::shared_ptr<View> ensureView(const QString &id);
    void choices();
    void controls();
    void applyMetadata(const QString &resource, const QString &run, const QJsonObject &value);
    quint64 send(const QString &method, const std::shared_ptr<View> &view, QJsonObject args = {});
    void input(const QString &id, const QByteArray &bytes);
};
} // namespace mterm
