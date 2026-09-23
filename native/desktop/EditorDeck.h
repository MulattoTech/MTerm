// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro)
#pragma once
#include "services/Backend.h"
#include <QHash>
#include <QJsonObject>
#include <QTextCursor>
#include <QWidget>
#include <memory>
#include <vector>
class QPlainTextEdit;
class QLineEdit;
class QTabBar;
class QTreeWidget;
class QLabel;
class QPushButton;
class QTextDocument;
namespace mterm {
/// Workspace-scoped, lazily created editor deck. UI thread only; all file I/O uses Backend.
/// At most 12 open file documents. Each file has its own undo/cursor/version state.
/// Read/save responses carry document identity and generation, never the active-tab assumption.
class EditorDeck final : public QWidget {
    Q_OBJECT
  public:
    explicit EditorDeck(Backend *backend, QWidget *parent = nullptr);
    QPlainTextEdit *editor() const {
        return editor_;
    }
    void setWorkspace(const QJsonObject &state);
    void refreshDirectory();
    bool hasUnsavedChanges() const;
    bool hasPendingWrites() const;
    QByteArray dirtySignature() const;
    QStringList dirtyPaths() const;
    static constexpr int MaxBuffers = 12;
  signals:
    void message(QString text);

  private:
    struct Buffer {
        QString key, path, version;
        QTextDocument *document = nullptr;
        QTextCursor cursor;
        int vertical = 0, horizontal = 0;
        bool saving = false;
    };
    struct Pending {
        QString method, key, path;
        quint64 generation = 0, selection = 0;
        QString submitted;
        int expectedRevision = -1;
    };
    Backend *backend_;
    QPlainTextEdit *editor_;
    QLineEdit *path_;
    QTabBar *tabs_;
    QTreeWidget *files_;
    QLabel *directoryLabel_, *status_;
    QPushButton *save_, *new_;
    QTextDocument *scratch_;
    std::vector<std::unique_ptr<Buffer>> buffers_;
    QHash<quint64, Pending> pending_;
    QString workspace_, directory_ = ".", active_;
    bool developer_ = false, activating_ = false;
    quint64 generation_ = 0, selection_ = 0, pendingDirectory_ = 0;
    Buffer *buffer(const QString &key) const;
    static QString keyFor(const QString &path);
    static QString cleanRelative(QString path);
    QTextDocument *makeDocument(const QString &text);
    void activate(const QString &key);
    void updateLabels();
    void openFile(bool reload = false);
    void createDraft();
    void saveBuffer(const QString &key);
    void closeBuffer(int index);
    void listDirectory(const QString &path);
    void receive(quint64, const QString &, const QJsonObject &, const QString &);
};
} // namespace mterm
