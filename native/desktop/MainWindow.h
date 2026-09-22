// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "services/Backend.h"
#include <QJsonObject>
#include <QMainWindow>
class QTabWidget;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QLabel;
namespace mterm {
class CanvasView;
class JobPane;
class TerminalPane;
/// Native presentation only. Business operations are named Backend requests.
class MainWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QString databaseFile, QWidget *parent = nullptr);
    void openWorkspace(const QString &root);
    QString workspaceId() const {
        return state_["workspaceId"].toString();
    }
  signals:
    void workspaceReady();
    void firstPaint();

  protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    Backend *backend_;
    QJsonObject state_;
    QTabWidget *tabs_ = nullptr;
    CanvasView *canvas_ = nullptr;
    QLineEdit *noteTitle_ = nullptr, *noteText_ = nullptr, *taskTitle_ = nullptr,
              *filePath_ = nullptr;
    QPlainTextEdit *taskObjective_ = nullptr, *editor_ = nullptr;
    QTreeWidget *tasks_ = nullptr, *processes_ = nullptr, *audit_ = nullptr;
    QCheckBox *developer_ = nullptr;
    QLabel *workspaceLabel_ = nullptr;
    QString fileVersion_, loadedFile_;
    QList<JobPane *> jobs_;
    TerminalPane *terminal_ = nullptr;
    bool painted_ = false, loadingEditor_ = false;
    quint64 send(const QString &method, QJsonObject args = {});
    void applyState(const QJsonObject &state);
    void onResponse(quint64 id, const QString &method, const QJsonObject &result,
                    const QString &error);
    void createUi();
    void refreshCurrentTab();
};
} // namespace mterm
