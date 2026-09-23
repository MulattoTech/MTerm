// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#pragma once
#include <QDialog>
#include <QList>
class QLineEdit;
class QListWidget;
namespace mterm {
struct PaletteCommand {
    QString id;
    QString title;
    QString detail;
};
/// Keyboard-first named-action router. It never evaluates arbitrary commands.
class CommandPalette final : public QDialog {
    Q_OBJECT
  public:
    explicit CommandPalette(QList<PaletteCommand> commands, QWidget *parent = nullptr);
    void present();
  signals:
    void commandChosen(QString id);

  protected:
    bool eventFilter(QObject *, QEvent *) override;

  private:
    QList<PaletteCommand> commands_;
    QLineEdit *search_;
    QListWidget *list_;
    void filter();
    void choose();
};
} // namespace mterm
