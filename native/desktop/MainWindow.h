// Modified: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro); see docs/ai/changes/.
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (original implementation)
// Modified: 2026-09-22-native-ux; docs/ai/changes/2026-09-22-native-ux.json
#pragma once
#include "services/Backend.h"
#include <QHash>
#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>
#include <functional>
class QTabWidget;
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QTreeWidget;
class QLabel;
namespace mterm {
class CanvasView;
class ProjectView;
class WorkspaceShell;
class CommandPalette;
class JobPane;
class EditorDeck;
class TerminalPane;
/// Native presentation coordinator. Business operations remain named scoped Backend requests.
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
    void paintEvent(QPaintEvent *) override;
    void closeEvent(QCloseEvent *) override;

  private:
    Backend *backend_;
    EditorDeck *editorDeck_ = nullptr;
    QString uiSettingsFile_;
    QJsonObject state_;
    WorkspaceShell *shell_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    CanvasView *canvas_ = nullptr;
    ProjectView *project_ = nullptr;
    CommandPalette *palette_ = nullptr;
    QLineEdit *noteTitle_ = nullptr, *taskTitle_ = nullptr;
    QPlainTextEdit *noteBody_ = nullptr, *taskObjective_ = nullptr, *editor_ = nullptr;
    QTreeWidget *tasks_ = nullptr, *processes_ = nullptr, *audit_ = nullptr;
    QCheckBox *developer_ = nullptr;
    QString selectedNodeId_;
    QList<JobPane *> jobs_;
    TerminalPane *terminal_ = nullptr;
    QHash<QString, int> toolPages_;
    QHash<quint64, QString> requestScopes_;
    bool painted_ = false, projectMode_ = false, layoutDirty_ = false, closeAfterSave_ = false;
    quint64 pendingLayout_ = 0, pendingCreate_ = 0;
    QByteArray approvedCloseSignature_;
    QTimer saveTimer_;
    quint64 send(const QString &, QJsonObject args = {});
    void createUi();
    void createInspectorPanels();
    void ensureInspector(const QString &id);
    QWidget *createTasksPanel();
    QWidget *createEditorPanel();
    QWidget *createNotesPanel();
    QHash<QString, std::function<QWidget *()>> panelFactories_;
    void selectTool(const QString &);
    void createResource(const QString &);
    void inspectNode(const QJsonObject &);
    void applyState(const QJsonObject &);
    void onResponse(quint64, const QString &, const QJsonObject &, const QString &);
    void refreshCurrentTab();
    void saveWindowPreferences();
    void scheduleLayoutSave();
    void flushLayout();
    void setProjectMode(bool);
    void saveNote();
    void showSources();
};
} // namespace mterm
