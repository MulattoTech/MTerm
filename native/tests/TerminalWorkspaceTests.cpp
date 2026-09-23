// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-terminal-candidate (OpenAI / GPT-6 Astra Pro)
#include "desktop/CanvasView.h"
#include "desktop/MainWindow.h"
#include "desktop/TerminalWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGraphicsItem>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest>
using namespace mterm;
namespace {
void approve(MainWindow &w) {
    QTimer::singleShot(20, [] {
        if (auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
            dialog->button(QMessageBox::Yes)->click();
    });
    w.findChild<QPushButton *>("approve-pty")->click();
}
TerminalWidget *screen(MainWindow &w) {
    auto *stack = w.findChild<QStackedWidget *>("terminal-screens");
    return stack ? qobject_cast<TerminalWidget *>(stack->currentWidget()) : nullptr;
}
} // namespace
class TerminalWorkspaceTests final : public QObject {
    Q_OBJECT
  private slots:
    void resourceSelectionNeverStartsAProcess() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *selector = w.findChild<QComboBox *>("terminal-resource");
        QVERIFY2(selector, "Native terminal needs resource identities, not a singleton view");
        QCOMPARE(selector->count(), 1);
        QVERIFY(!w.findChild<QPushButton *>("start-pty")->isEnabled());
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *create = w.findChild<QPushButton *>("create-terminal");
        QTRY_VERIFY(create->isEnabled());
        create->click();
        QTRY_COMPARE(selector->count(), 2);
        QTRY_COMPARE(selector->currentIndex(), 1);
        QVERIFY(!w.findChild<QPushButton *>("start-pty")->isEnabled());
        auto *native = screen(w);
        QVERIFY(native);
        QVERIFY(native->screenText().trimmed().isEmpty());
        QVERIFY(w.findChild<QLineEdit *>("terminal-cwd"));
        create->click();
        QTRY_COMPARE(selector->count(), 3);
        QVERIFY2(selector->itemText(1).section(" · ", 0, 0) !=
                     selector->itemText(2).section(" · ", 0, 0),
                 "New terminal names must be distinguishable");
    }
    void twoLiveTerminalCardsKeepDifferentShellState() {
#ifndef Q_OS_WIN
        QSKIP("Windows ConPTY UI fixture");
#else
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.resize(1500, 940);
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *selector = w.findChild<QComboBox *>("terminal-resource");
        QVERIFY(selector);
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *create = w.findChild<QPushButton *>("create-terminal");
        QTRY_VERIFY(create->isEnabled());
        create->click();
        QTRY_COMPARE(selector->count(), 2);
        QTRY_COMPARE(selector->currentIndex(), 1);
        const auto firstId = selector->currentData().toString();
        auto *first = screen(w);
        QVERIFY(first);
        approve(w);
        auto *start = w.findChild<QPushButton *>("start-pty");
        QTRY_VERIFY(start->isEnabled());
        w.findChild<QComboBox *>("terminal-shell")->setCurrentText("cmd");
        start->click();
        auto *status = w.findChild<QLabel *>("terminal-status");
        QTRY_VERIFY(status->text().contains("running", Qt::CaseInsensitive));
        first->setFocus();
        QTest::keyClicks(first, "set MTERM_UI_SLOT=BUILD");
        QTest::keyClick(first, Qt::Key_Return);
        QTest::keyClicks(first, "echo PROOF_%MTERM_UI_SLOT%");
        QTest::keyClick(first, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(first->screenText().contains("PROOF_BUILD"), 8000);
        create->click();
        QTRY_COMPARE(selector->count(), 3);
        QTRY_COMPARE(selector->currentIndex(), 2);
        auto *second = screen(w);
        QVERIFY(second && second != first);
        w.findChild<QComboBox *>("terminal-shell")->setCurrentText("cmd");
        QTRY_VERIFY(start->isEnabled());
        start->click();
        QTRY_VERIFY(status->text().contains("running", Qt::CaseInsensitive));
        second->setFocus();
        QTest::keyClicks(second, "set MTERM_UI_SLOT=TEST");
        QTest::keyClick(second, Qt::Key_Return);
        QTest::keyClicks(second, "echo PROOF_%MTERM_UI_SLOT%");
        QTest::keyClick(second, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(second->screenText().contains("PROOF_TEST"), 8000);
        QVERIFY(!first->screenText().contains("PROOF_TEST"));
        w.findChild<QPushButton *>("nav-editor")->click();
        w.findChild<QPushButton *>("view-project")->click();
        w.findChild<QPushButton *>("nav-terminal")->click();
        selector->setCurrentIndex(selector->findData(firstId));
        QCOMPARE(screen(w), first);
        QVERIFY(first->screenText().contains("PROOF_BUILD"));
        auto *stop = w.findChild<QPushButton *>("stop-pty");
        QVERIFY(stop);
        stop->click();
        QTRY_VERIFY(!stop->isEnabled());
        selector->setCurrentIndex(2);
        QCOMPARE(screen(w), second);
        QVERIFY(stop->isEnabled());
        second->setFocus();
        QTest::keyClicks(second, "echo LIVE_%MTERM_UI_SLOT%");
        QTest::keyClick(second, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(second->screenText().contains("LIVE_TEST"), 8000);
        if (const auto dir = qEnvironmentVariable("MTERM_TERMINAL_CAPTURE_DIR"); !dir.isEmpty()) {
            // Real fixture shell commands remove private temp paths from the published screenshot.
            second->setFocus();
            QTest::keyClicks(second, "prompt MTerm$G");
            QTest::keyClick(second, Qt::Key_Return);
            QTest::keyClicks(second, "cls");
            QTest::keyClick(second, Qt::Key_Return);
            QTest::keyClicks(second, "echo Independent native terminal - TEST session");
            QTest::keyClick(second, Qt::Key_Return);
            QTRY_VERIFY(second->screenText().contains("Independent native terminal"));

            w.findChild<QPushButton *>("view-canvas")->click();
            w.findChild<CanvasView *>("native-canvas")->arrangeResources();
            QTest::qWait(400);
            w.findChild<QLineEdit *>("workspace-path")
                ->setText("MTerm / isolated native terminal validation");
            QDir().mkpath(dir);
            QVERIFY(w.grab().save(QDir(dir).filePath("native-terminal-sessions.png")));
        }
        w.findChild<QCheckBox *>("developer-profile")->setChecked(false);
        QTRY_VERIFY(!stop->isEnabled());
        QVERIFY(!start->isEnabled());
#endif
    }
    void liveBadgesAreSharedButNotWrittenIntoCanvas() {
#ifndef Q_OS_WIN
        QSKIP("Windows ConPTY runtime badge regression");
#else
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *create = w.findChild<QPushButton *>("create-terminal");
        QTRY_VERIFY(create->isEnabled());
        create->click();
        auto *selector = w.findChild<QComboBox *>("terminal-resource");
        QVERIFY(selector);
        QTRY_COMPARE(selector->count(), 2);
        QTRY_COMPARE(selector->currentIndex(), 1);
        const auto id = selector->currentData().toString();
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        const auto before = canvas->canvas()["nodes"].toArray().last().toObject();
        QGraphicsItem *card = nullptr;
        for (auto *item : canvas->scene()->items())
            if (item->data(0).toString() == id)
                card = item;
        QVERIFY(card);
        approve(w);
        auto *start = w.findChild<QPushButton *>("start-pty");
        QTRY_VERIFY(start->isEnabled());
        w.findChild<QComboBox *>("terminal-shell")->setCurrentText("cmd");
        start->click();
        QTRY_COMPARE_WITH_TIMEOUT(card->data(1).toString(), QString("RUNNING"), 3000);
        QCOMPARE(canvas->canvas()["nodes"].toArray().last().toObject()["status"], before["status"]);
        w.findChild<QPushButton *>("view-project")->click();
        auto *project = w.findChild<QListWidget *>("project-view");
        QVERIFY(project);
        QListWidgetItem *projectCard = nullptr;
        for (int i = 0; i < project->count(); ++i)
            if (project->item(i)->data(Qt::UserRole).toJsonObject()["id"] == id)
                projectCard = project->item(i);
        QVERIFY(projectCard);
        QCOMPARE(projectCard->data(Qt::UserRole + 1).toString(), QString("RUNNING"));
        w.findChild<QPushButton *>("stop-pty")->click();
        QTRY_COMPARE(card->data(1).toString(), QString("STOPPED"));
        QCOMPARE(projectCard->data(Qt::UserRole + 1).toString(), QString("STOPPED"));
        QCOMPARE(projectCard->data(Qt::UserRole).toJsonObject()["status"], before["status"]);
#endif
    }
    void terminalViewCapacityIsBoundedWithoutDeletingCanvas() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *selector = w.findChild<QComboBox *>("terminal-resource");
        QVERIFY(selector);
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *create = w.findChild<QPushButton *>("create-terminal");
        QTRY_VERIFY(create->isEnabled());
        for (int i = 0; i < 10; ++i) {
            create->click();
            QTRY_COMPARE(selector->count(), i + 2);
        }
        auto *stack = w.findChild<QStackedWidget *>("terminal-screens");
        QVERIFY(stack);
        QVERIFY(stack->count() <= 8);
        QCOMPARE(w.findChild<CanvasView *>("native-canvas")->canvas()["nodes"].toArray().size(),
                 11);
    }
};
QTEST_MAIN(TerminalWorkspaceTests)
#include "TerminalWorkspaceTests.moc"
