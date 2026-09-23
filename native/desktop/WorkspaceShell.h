// Modified: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro); see docs/ai/changes/.
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QHash>
#include <QWidget>
class QFrame;
class QStackedWidget;
class QTabWidget;
class QSplitter;
class QCheckBox;
class QLineEdit;
class QLabel;
class QPushButton;
namespace mterm {
/// Presentation-only shell; emits named actions and never owns native jobs or data.
class WorkspaceShell final : public QWidget {
    Q_OBJECT
  public:
    explicit WorkspaceShell(QWidget *parent = nullptr);
    void mount(QWidget *canvas, QWidget *project, QTabWidget *inspector);
    void setWorkspace(const QString &root, bool developer, int resources);
    void setInspector(const QString &id, const QString &title, const QString &hint);
    void setProjectMode(bool enabled);
    void setSaveStatus(const QString &text);
    QCheckBox *developerControl() const {
        return developer_;
    }
  signals:
    void toolRequested(QString id);
    void resourceFilterChanged(QString text);
    void createRequested(QString kind);
    void projectRequested(bool project);
    void openWorkspaceRequested();
    void paletteRequested();
    void sourcesRequested();
    void saveRequested();

  private:
    QStackedWidget *primary_;
    QSplitter *splitter_;
    QWidget *inspectorHost_;
    QCheckBox *developer_;
    QLineEdit *path_;
    QLabel *profile_, *permission_, *count_, *saveState_, *inspectorTitle_, *inspectorHint_;
    QPushButton *canvasMode_, *projectMode_;
    QHash<QString, QPushButton *> navigation_;
};
} // namespace mterm
