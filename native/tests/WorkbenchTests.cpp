// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-workbench-roadmap (OpenAI / GPT-6 Astra Pro)
#include "desktop/CanvasView.h"
#include "desktop/MainWindow.h"
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QGraphicsItem>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSignalSpy>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTemporaryDir>
#include <QtTest>
using namespace mterm;
class WorkbenchTests final : public QObject {
    Q_OBJECT
    static void write(const QString &path, const QByteArray &bytes) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            qFatal("fixture write failed");
        f.write(bytes);
    }
    static void load(MainWindow &w, const QString &path) {
        w.findChild<QLineEdit *>("file-path")->setText(path);
        w.findChild<QPushButton *>("open-file")->click();
    }
  private slots:
    void multipleFilesKeepDirtyTextAndUndo() {
        QTemporaryDir d;
        write(d.filePath("a.txt"), "alpha");
        write(d.filePath("b.txt"), "beta");
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *tabs = w.findChild<QTabBar *>("editor-buffer-tabs");
        QVERIFY2(tabs, "Opening another file needs retained native tabs");
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QTRY_VERIFY(!editor->isReadOnly());
        load(w, "a.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("alpha"));
        auto *first = editor->document();
        editor->moveCursor(QTextCursor::End);
        QTest::keyClicks(editor, " edited");
        load(w, "b.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("beta"));
        QCOMPARE(tabs->count(), 2);
        tabs->setCurrentIndex(0);
        QCOMPARE(editor->document(), first);
        QCOMPARE(editor->toPlainText(), QString("alpha edited"));
        QVERIFY(first->isModified());
        QVERIFY(tabs->tabText(0).contains('*'));
        editor->undo();
        QVERIFY(editor->toPlainText() != QString("alpha edited"));
    }
    void delayedSaveNeverReplacesAnotherBuffer() {
        QTemporaryDir d;
        write(d.filePath("a.txt"), "alpha");
        write(d.filePath("b.txt"), "beta");
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *tabs = w.findChild<QTabBar *>("editor-buffer-tabs");
        QVERIFY(tabs);
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        QTRY_VERIFY(!editor->isReadOnly());
        load(w, "a.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("alpha"));
        load(w, "b.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("beta"));
        tabs->setCurrentIndex(0);
        editor->setPlainText("saved alpha");
        w.findChild<QPushButton *>("save-file")->click();
        tabs->setCurrentIndex(1);
        QTRY_VERIFY(([&] {
            QFile f(d.filePath("a.txt"));
            return f.open(QIODevice::ReadOnly) && f.readAll() == "saved alpha";
        })());
        QCOMPARE(editor->toPlainText(), QString("beta"));
        QCOMPARE(w.findChild<QLineEdit *>("file-path")->text(), QString("b.txt"));
        tabs->setCurrentIndex(0);
        QTRY_VERIFY(!editor->document()->isModified());
    }
    void dirtyBackgroundBufferCanCancelClose() {
        QTemporaryDir d;
        write(d.filePath("a.txt"), "alpha");
        write(d.filePath("b.txt"), "beta");
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *tabs = w.findChild<QTabBar *>("editor-buffer-tabs");
        QVERIFY(tabs);
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        QTRY_VERIFY(!editor->isReadOnly());
        load(w, "a.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("alpha"));
        editor->insertPlainText("dirty");
        load(w, "b.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("beta"));
        int prompts = 0;
        QTimer timer;
        timer.setInterval(5);
        connect(&timer, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++prompts;
                box->button(QMessageBox::No)->click();
            }
        });
        timer.start();
        w.close();
        timer.stop();
        QVERIFY(w.isVisible());
        QCOMPARE(prompts, 1);
        tabs->setCurrentIndex(0);
        QVERIFY(editor->toPlainText().contains("dirty"));
    }
    void windowGeometryRestoresFromItsOwnDataLocation() {
        QTemporaryDir d;
        const auto db = d.filePath("data/db");
        const auto available = QGuiApplication::primaryScreen()->availableGeometry().size();
        const QSize target(qMax(1024, qMin(1234, available.width() - 32)),
                           qMax(720, qMin(810, available.height() - 64)));
        {
            MainWindow w(db);
            w.show();
            w.resize(target);
            QTest::qWait(50);
            w.close();
        }
        {
            MainWindow w(db);
            w.show();
            QTRY_COMPARE(w.size(), target);
        }
    }
    void reloadDoesNotDiscardEditsMadeAfterRequest() {
        QTemporaryDir d;
        write(d.filePath("a.txt"), "alpha");
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        QTRY_VERIFY(!editor->isReadOnly());
        load(w, "a.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("alpha"));
        w.findChild<QPushButton *>("reload-file")->click();
        editor->moveCursor(QTextCursor::End);
        editor->insertPlainText(" NEWER");
        QTest::qWait(150);
        QCOMPARE(editor->toPlainText(), QString("alpha NEWER"));
        QVERIFY(editor->document()->isModified());
    }
    void explicitTabSelectionWinsOverPendingRead() {
        QTemporaryDir d;
        write(d.filePath("a.txt"), "alpha");
        write(d.filePath("b.txt"), "beta");
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        load(w, "a.txt");
        QTRY_COMPARE(editor->toPlainText(), QString("alpha"));
        load(w, "b.txt");
        auto *tabs = w.findChild<QTabBar *>("editor-buffer-tabs");
        QVERIFY(tabs);
        QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(0).center());
        QTRY_COMPARE(tabs->count(), 2);
        QCOMPARE(editor->toPlainText(), QString("alpha"));
    }
    void boundedBufferDeckSupportsKeyboardClose() {
        QTemporaryDir d;
        for (int i = 0; i < 13; ++i)
            write(d.filePath(QString("f%1.txt").arg(i)), QByteArray::number(i));
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *tabs = w.findChild<QTabBar *>("editor-buffer-tabs");
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QVERIFY(tabs);
        for (int i = 0; i < 12; ++i) {
            load(w, QString("f%1.txt").arg(i));
            QTRY_COMPARE(tabs->count(), i + 1);
        }
        load(w, "f12.txt");
        QTest::qWait(50);
        QCOMPARE(tabs->count(), 12);
        QVERIFY(w.statusBar()->currentMessage().contains("12-file limit"));
        w.activateWindow();
        editor->setFocus();
        QTest::qWait(40);
        QTest::keyClick(editor, Qt::Key_W, Qt::ControlModifier);
        QTRY_COMPARE_WITH_TIMEOUT(tabs->count(), 11, 1200);
        load(w, "f12.txt");
        QTRY_COMPARE(tabs->count(), 12);
        QTRY_COMPARE(editor->toPlainText(), QString("12"));
    }
    void optionalWorkbenchVisualEvidence() {
        const auto out = qEnvironmentVariable("MTERM_WORKBENCH_CAPTURE_DIR");
        if (out.isEmpty())
            QSKIP("Opt-in real Windows screenshot fixture");
        QTemporaryDir d;
        write(d.filePath("workspace.md"),
              "# MTerm workbench\n\nNative speed. Familiar workflow.\n\nEach file keeps its own "
              "edits, undo and save version.\n");
        write(d.filePath("settings.json"),
              "{\n  \"theme\": \"native-dark\",\n  \"workspace\": \"isolated test fixture\"\n}\n");
        MainWindow w(d.filePath("data/db"));
        w.resize(1500, 940);
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QTRY_VERIFY(!editor->isReadOnly());
        load(w, "workspace.md");
        QTRY_VERIFY(editor->toPlainText().contains("Familiar workflow"));
        editor->moveCursor(QTextCursor::End);
        editor->insertPlainText("\nUnsaved work remains in this tab.\n");
        load(w, "settings.json");
        QTRY_VERIFY(editor->toPlainText().contains("native-dark"));
        w.findChild<QTabBar *>("editor-buffer-tabs")->setCurrentIndex(0);
        QTest::qWait(120);
        w.findChild<QLineEdit *>("workspace-path")
            ->setText("MTerm / isolated multi-buffer validation");
        QDir().mkpath(out);
        QVERIFY(w.grab().save(QDir(out).filePath("native-workbench.png")));
    }
    void resourceSearchDoesNotDeleteResources() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *search = w.findChild<QLineEdit *>("workspace-resource-search");
        QVERIFY2(search, "Canvas needs a real resource filter");
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        const auto original = canvas->canvas()["nodes"].toArray();
        search->setText("certainly absent resource");
        QTest::qWait(50);
        int visible = 0;
        for (auto *i : canvas->scene()->items())
            visible += i->isVisible();
        QCOMPARE(visible, 0);
        QCOMPARE(canvas->canvas()["nodes"].toArray(), original);
        search->clear();
        QTest::qWait(50);
        visible = 0;
        for (auto *i : canvas->scene()->items())
            visible += i->isVisible();
        QCOMPARE(visible, original.size());
    }
};
QTEST_MAIN(WorkbenchTests)
#include "WorkbenchTests.moc"
