// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-native-ux; exercise real visible navigation and terminal continuity.
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "desktop/MainWindow.h"
#include "desktop/TerminalWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>
using namespace mterm;
class DesktopTests : public QObject {
    Q_OBJECT
  private slots:
    void terminalUiRoundtrip() {
#ifndef Q_OS_WIN
        QSKIP("Windows-only PTY UI");
#else
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *profile = w.findChild<QCheckBox *>("developer-profile");
        QVERIFY(profile);
        profile->setChecked(true);
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        for (int i = 0; i < tabs->count(); ++i)
            if (tabs->tabText(i) == "Terminal")
                tabs->setCurrentIndex(i);
        auto *shell = w.findChild<QComboBox *>("terminal-shell");
        QVERIFY(shell);
        shell->setCurrentText("cmd");
        auto *approve = w.findChild<QPushButton *>("approve-pty");
        QVERIFY(approve);
        QTRY_VERIFY(approve->isEnabled());
        QTimer::singleShot(0, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                box->button(QMessageBox::Yes)->click();
        });
        approve->click();
        auto *status = w.findChild<QLabel *>("terminal-status");
        QVERIFY(status);
        QTRY_VERIFY(status->text().contains("Terminal approved"));
        auto *start = w.findChild<QPushButton *>("start-pty");
        QVERIFY(start);
        start->click();
        QTRY_VERIFY(status->text().contains("Native shell running"));
        auto *screen = w.findChild<TerminalWidget *>("terminal-screen");
        QVERIFY(screen);
        screen->setFocus();
        QTest::keyClicks(screen, "set MTERM_UI_TEST=UI_OK");
        QTest::keyClick(screen, Qt::Key_Return);
        QTest::keyClicks(screen, "echo MTERM_%MTERM_UI_TEST%");
        QTest::keyClick(screen, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(screen->screenText().contains("MTERM_UI_OK"), 8000);
        // The same live terminal is retained across both navigation and view changes.
        auto *editorNav = w.findChild<QPushButton *>("nav-editor");
        QVERIFY(editorNav);
        editorNav->click();
        auto *project = w.findChild<QPushButton *>("view-project");
        QVERIFY(project);
        project->click();
        auto *back = w.findChild<QPushButton *>("view-canvas");
        QVERIFY(back);
        back->click();
        auto *terminalNav = w.findChild<QPushButton *>("nav-terminal");
        QVERIFY(terminalNav);
        terminalNav->click();
        QCOMPARE(w.findChild<TerminalWidget *>("terminal-screen"), screen);
        screen->setFocus();
        QTest::keyClicks(screen, "echo PERSIST_%MTERM_UI_TEST%");
        QTest::keyClick(screen, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(screen->screenText().contains("PERSIST_UI_OK"), 8000);
#endif
    }
    void saveKeepsNewerEditorChanges() {
        QTemporaryDir d;
        QFile f(d.filePath("late.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("before");
        f.close();
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        auto *editorNav = w.findChild<QPushButton *>("nav-editor");
        QVERIFY(editorNav);
        editorNav->click();
        auto *path = w.findChild<QLineEdit *>("file-path");
        path->setText("late.txt");
        w.findChild<QPushButton *>("open-file")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QTRY_COMPARE(editor->toPlainText(), QString("before"));
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *save = w.findChild<QPushButton *>("save-file");
        QTRY_VERIFY(save->isEnabled());
        editor->setPlainText("saved");
        save->click();
        editor->insertPlainText("unsaved");
        QTest::qWait(150);
        QVERIFY(editor->toPlainText().contains("unsaved"));
        QVERIFY(editor->document()->isModified());
    }
    void nativeShellAndWorkspace() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
        QVERIFY(w.windowTitle().contains("MTerm"));
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        QVERIFY(tabs->count() >= 7);
        auto *profile = w.findChild<QCheckBox *>("developer-profile");
        QVERIFY(profile);
        QVERIFY(!profile->isChecked());
    }
    void noteAndTaskFlow() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
        auto *profile = w.findChild<QCheckBox *>("developer-profile");
        QVERIFY(profile);
        profile->setChecked(true);
        w.findChild<QPushButton *>("nav-notes")->click();
        auto *note = w.findChild<QPushButton *>("add-note");
        QVERIFY(note);
        QTRY_VERIFY(note->isEnabled());
        auto *title = w.findChild<QLineEdit *>("note-title");
        QVERIFY(title);
        auto *notesNav = w.findChild<QPushButton *>("nav-notes");
        QVERIFY(notesNav);
        notesNav->click();
        title->setText("Native note");
        note->click();
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        auto *taskNav = w.findChild<QPushButton *>("nav-tasks");
        QVERIFY(taskNav);
        taskNav->click();
        auto *task = w.findChild<QLineEdit *>("task-title");
        QVERIFY(task);
        task->setText("Native test task");
        auto *add = w.findChild<QPushButton *>("add-task");
        QVERIFY(add);
        add->click();
        auto *list = w.findChild<QTreeWidget *>("tasks-list");
        QVERIFY(list);
        QTRY_COMPARE_WITH_TIMEOUT(list->topLevelItemCount(), 1, 5000);
        QCOMPARE(list->topLevelItem(0)->text(1), QString("Native test task"));
    }
    void editorFlow() {
        QTemporaryDir d;
        QFile f(d.filePath("hello.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("before");
        f.close();
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        auto *editorNav = w.findChild<QPushButton *>("nav-editor");
        QVERIFY(editorNav);
        editorNav->click();
        auto *path = w.findChild<QLineEdit *>("file-path");
        QVERIFY(path);
        path->setText("hello.txt");
        auto *open = w.findChild<QPushButton *>("open-file");
        QVERIFY(open);
        open->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QVERIFY(editor);
        QTRY_COMPARE_WITH_TIMEOUT(editor->toPlainText(), QString("before"), 5000);
        auto *profile = w.findChild<QCheckBox *>("developer-profile");
        QVERIFY(profile);
        profile->setChecked(true);
        auto *save = w.findChild<QPushButton *>("save-file");
        QVERIFY(save);
        QTRY_VERIFY(save->isEnabled());
        editor->setPlainText("after");
        save->click();
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                     QFile read(d.filePath("hello.txt"));
                                     return read.open(QIODevice::ReadOnly) &&
                                            read.readAll() == "after";
                                 })(),
                                 5000);
    }
};
QTEST_MAIN(DesktopTests)
#include "DesktopTests.moc"
