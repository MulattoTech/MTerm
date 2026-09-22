// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "core/Canvas.h"
#include "desktop/CanvasView.h"
#include "desktop/MainWindow.h"
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFrame>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>
using namespace mterm;
class UxTests final : public QObject {
    Q_OBJECT
  private slots:
    void canvasAndInspectorVisibleTogether() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.resize(1500, 900);
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *sidebar = w.findChild<QFrame *>("app-sidebar");
        QVERIFY2(sidebar, "Native app must restore left navigation, not full-window tool tabs");
        QVERIFY(sidebar->isVisible());
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        auto *inspect = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(inspect);
        QVERIFY(canvas->isVisible());
        QVERIFY(inspect->isVisible());
        QVERIFY(canvas->width() >= 400);
        QVERIFY(inspect->width() >= 320);
        QVERIFY(w.findChild<QPushButton *>("nav-terminal"));
        QVERIFY(w.findChild<QPushButton *>("view-project"));
    }
    void commandPaletteFiltersAndRoutes() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        auto *button = w.findChild<QPushButton *>("open-command-palette");
        QVERIFY2(button, "Header must expose a searchable command palette");
        button->click();
        auto *search = w.findChild<QLineEdit *>("command-search");
        QTRY_VERIFY(search && search->isVisible());
        QTest::keyClicks(search, "Git");
        QTest::keyClick(search, Qt::Key_Return);
        auto *tabs = w.findChild<QTabWidget *>("workspace-tabs");
        QVERIFY(tabs);
        QTRY_COMPARE(tabs->tabText(tabs->currentIndex()), QString("Git"));
        QVERIFY(!search->isVisible());
    }
    void projectAndCanvasUseTheSameResources() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *project = w.findChild<QPushButton *>("view-project");
        QVERIFY2(project, "The reference Canvas/Project switch must exist");
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        const auto before = canvas->canvas()["nodes"].toArray();
        project->click();
        auto *cards = w.findChild<QListWidget *>("project-view");
        QVERIFY(cards);
        QTRY_VERIFY(cards->isVisible());
        QCOMPARE(cards->count(), before.size());
        auto *back = w.findChild<QPushButton *>("view-canvas");
        QVERIFY(back);
        back->click();
        QTRY_VERIFY(canvas->isVisible());
        QCOMPARE(canvas->canvas()["nodes"].toArray(), before);
    }
    void nativeFileBrowserLoadsFile() {
        QTemporaryDir d;
        QFile f(d.filePath("hello.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("native UX reference");
        f.close();
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *nav = w.findChild<QPushButton *>("nav-editor");
        QVERIFY(nav);
        nav->click();
        auto *tree = w.findChild<QTreeWidget *>("file-browser");
        QVERIFY(tree);
        QTRY_VERIFY(tree->topLevelItemCount() > 0);
        QTreeWidgetItem *target = nullptr;
        for (int i = 0; i < tree->topLevelItemCount(); ++i)
            if (tree->topLevelItem(i)->text(0) == "hello.txt")
                target = tree->topLevelItem(i);
        QVERIFY(target);
        QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier,
                          tree->visualItemRect(target).center());
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QVERIFY(editor);
        QTRY_COMPARE(editor->toPlainText(), QString("native UX reference"));
        w.findChild<QPushButton *>("nav-terminal")->click();
        nav->click();
        QCOMPARE(editor->toPlainText(), QString("native UX reference"));
    }
    void canvasOverlaysStayAtViewportEdges() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        auto *mini = w.findChild<QWidget *>("canvas-minimap");
        auto *controls = w.findChild<QWidget *>("canvas-controls");
        QVERIFY(mini);
        QVERIFY(controls);
        QTest::qWait(80);
        qInfo() << "viewport" << canvas->viewport()->size() << "mini" << mini->geometry()
                << "controls" << controls->geometry();
        QVERIFY(controls->isVisible());
        QVERIFY(qAbs(mini->geometry().right() - (canvas->viewport()->width() - 15)) < 4);
        QVERIFY(qAbs(mini->geometry().bottom() - (canvas->viewport()->height() - 15)) < 4);
    }
    void observeTerminalIsNotActionable() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        auto *start = w.findChild<QPushButton *>("start-pty");
        QVERIFY(start);
        QVERIFY2(!start->isEnabled(),
                 "Observe must show disabled execution controls, not let clicks fail later");
    }
    void noteChangesAutosaveAndRecover() {
        QTemporaryDir d;
        const auto db = d.filePath("data/db");
        QString identity;
        {
            MainWindow w(db);
            w.show();
            QSignalSpy ready(&w, &MainWindow::workspaceReady);
            w.openWorkspace(d.path());
            QTRY_VERIFY(!ready.isEmpty());
            w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
            auto *create = w.findChild<QPushButton *>("create-note");
            QVERIFY(create);
            QTRY_VERIFY(create->isEnabled());
            create->click();
            auto *canvas = w.findChild<CanvasView *>("native-canvas");
            QVERIFY(canvas);
            QTRY_COMPARE(canvas->canvas()["nodes"].toArray().size(), 2);
            identity = canvas->canvas()["nodes"].toArray().last().toObject()["id"].toString();
            auto *title = w.findChild<QLineEdit *>("note-title");
            auto *body = w.findChild<QPlainTextEdit *>("note-content");
            QVERIFY(title);
            QVERIFY(body);
            QTRY_VERIFY(title->isVisible());
            title->setText("Durable UX decision");
            body->setPlainText("Native speed must retain the familiar workflow.");
            w.findChild<QPushButton *>("save-note")->click();
            QTest::qWait(650);
            QCOMPARE(canvas->canvas()["nodes"].toArray().last().toObject()["title"].toString(),
                     QString("Durable UX decision"));
            w.close();
            QTRY_VERIFY(!w.isVisible());
        }
        {
            MainWindow w(db);
            w.show();
            QSignalSpy ready(&w, &MainWindow::workspaceReady);
            w.openWorkspace(d.path());
            QTRY_VERIFY(!ready.isEmpty());
            auto *canvas = w.findChild<CanvasView *>("native-canvas");
            const auto n = canvas->canvas()["nodes"].toArray().last().toObject();
            QCOMPARE(n["id"].toString(), identity);
            QCOMPARE(n["title"].toString(), QString("Durable UX decision"));
            QCOMPARE(n["content"].toString(),
                     QString("Native speed must retain the familiar workflow."));
        }
    }
    void responsiveWorkspaceAndOptionalVisualEvidence() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        int expected = 1;
        for (const auto *kind : {"agent", "terminal", "editor"}) {
            auto *create = w.findChild<QPushButton *>("create-" + QString(kind));
            QVERIFY(create);
            QTRY_VERIFY(create->isEnabled());
            create->click();
            ++expected;
            QTRY_COMPARE(canvas->canvas()["nodes"].toArray().size(), expected);
        }
        w.findChild<QPushButton *>("nav-terminal")->click();
        auto *path = w.findChild<QLineEdit *>("workspace-path");
        QVERIFY(path);
        path->setText("MTerm / isolated UX validation workspace");
        const auto capture = qEnvironmentVariable("MTERM_UX_CAPTURE_DIR");
        for (const auto size : {QSize(1500, 940), QSize(1100, 780)}) {
            w.resize(size);
            QTest::qWait(80);
            canvas->arrangeResources();
            QTest::qWait(400);
            const auto *inspect = w.findChild<QTabWidget *>("workspace-tabs");
            QVERIFY(inspect);
            QVERIFY(w.width() <= size.width() + 20);
            QVERIFY(canvas->isVisible());
            QVERIFY(inspect->isVisible());
            QVERIFY(canvas->width() >= 320);
            QVERIFY(inspect->width() >= 320);
            const auto *command = w.findChild<QPushButton *>("open-command-palette");
            QVERIFY(command);
            QVERIFY(w.rect().contains(command->mapTo(&w, command->rect().center())));
            if (!capture.isEmpty()) {
                QDir().mkpath(capture);
                path->setText("MTerm / isolated UX validation workspace");
                QVERIFY(w.grab().save(
                    QDir(capture).filePath(QString("native-canvas-%1.png").arg(size.width()))));
            }
        }
        w.resize(1500, 940);
        w.findChild<QPushButton *>("view-project")->click();
        QTest::qWait(400);
        if (!capture.isEmpty()) {
            path->setText("MTerm / isolated UX validation workspace");
            QVERIFY(w.grab().save(QDir(capture).filePath("native-project.png")));
        }
        w.findChild<QPushButton *>("open-command-palette")->click();
        QTest::qWait(80);
        auto *search = w.findChild<QLineEdit *>("command-search");
        QVERIFY(search);
        QTest::keyClicks(search, "Git");
        if (!capture.isEmpty()) {
            auto *dialog = w.findChild<QDialog *>("command-palette");
            QVERIFY(dialog);
            QVERIFY(dialog->grab().save(QDir(capture).filePath("native-command-palette.png")));
        }
        QTest::keyClick(search, Qt::Key_Escape);
    }
    void arrangeKeepsCardsReadableAndPreservesIdentity() {
        CanvasView view;
        view.resize(800, 720);
        view.show();
        auto state = initialCanvas();
        QJsonArray nodes;
        for (int i = 0; i < 4; ++i) {
            auto n = newNode("note", QString("Arrange %1").arg(i), "Test arrangement");
            n["x"] = 1000 + i * 400;
            n["y"] = 1000;
            nodes.append(n);
        }
        state["nodes"] = nodes;
        view.setCanvas(state, true);
        auto *arrange = view.findChild<QPushButton *>("canvas-arrange");
        QVERIFY2(
            arrange,
            "Native canvas needs a readable arrange action, not zooming all text to tiny sizes");
        arrange->click();
        const auto result = view.canvas()["nodes"].toArray();
        QCOMPARE(result.size(), nodes.size());
        QCOMPARE(view.transform().m11(), 1.0);
        for (int i = 0; i < result.size(); ++i) {
            QCOMPARE(result[i].toObject()["id"], nodes[i].toObject()["id"]);
            QVERIFY(result[i].toObject()["x"].toDouble() < 800);
        }
    }
    void unusedInspectorsAreLazyAndRetainedAfterFirstUse() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        QVERIFY2(!w.findChild<QPlainTextEdit *>("file-editor"),
                 "Unused editor must not load at startup");
        QVERIFY2(!w.findChild<QPlainTextEdit *>("task-objective"),
                 "Unused task editor must not load at startup");
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QVERIFY(editor);
        editor->setPlainText("retained");
        editor->document()->setModified(false);
        w.findChild<QPushButton *>("nav-terminal")->click();
        w.findChild<QPushButton *>("nav-editor")->click();
        QCOMPARE(w.findChild<QPlainTextEdit *>("file-editor"), editor);
        QCOMPARE(editor->toPlainText(), QString("retained"));
    }
    void closingDirtyEditorDuringLayoutSaveConfirmsOnce() {
        QTemporaryDir d;
        MainWindow w(d.filePath("data/db"));
        w.show();
        QSignalSpy ready(&w, &MainWindow::workspaceReady);
        w.openWorkspace(d.path());
        QTRY_VERIFY(!ready.isEmpty());
        w.findChild<QCheckBox *>("developer-profile")->setChecked(true);
        w.findChild<QPushButton *>("nav-editor")->click();
        auto *editor = w.findChild<QPlainTextEdit *>("file-editor");
        QVERIFY(editor);
        QTRY_VERIFY(!editor->isReadOnly());
        editor->setPlainText("Unsaved draft");
        editor->document()->setModified(true);
        auto *canvas = w.findChild<CanvasView *>("native-canvas");
        QVERIFY(canvas);
        canvas->arrangeResources();
        int confirmations = 0;
        QTimer responder;
        responder.setInterval(10);
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++confirmations;
                box->button(QMessageBox::Yes)->click();
            }
        });
        responder.start();
        w.close();
        QTRY_VERIFY_WITH_TIMEOUT(!w.isVisible(), 3000);
        responder.stop();
        QCOMPARE(confirmations, 1);
    }
    void canvasUsesBoundedLightweightSceneItems() {
        CanvasView view;
        auto data = initialCanvas();
        QJsonArray nodes;
        for (int i = 0; i < 500; ++i) {
            auto n = newNode("note", QString("Node %1").arg(i), "Bounded native card");
            n["x"] = (i % 20) * 400;
            n["y"] = (i / 20) * 280;
            nodes.append(n);
        }
        data["nodes"] = nodes;
        view.setCanvas(data, true);
        QVERIFY2(view.scene()->items().size() <= 501,
                 "Use one native-painted card per resource, not a nested text/widget tree");
        QCOMPARE(view.canvas()["nodes"].toArray().size(), 500);
    }
};
QTEST_MAIN(UxTests)
#include "UxTests.moc"
