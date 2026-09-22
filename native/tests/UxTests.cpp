// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-ux (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-ux.json.
#include "core/Canvas.h"
#include "desktop/CanvasView.h"
#include "desktop/MainWindow.h"
#include <QCheckBox>
#include <QDialog>
#include <QFile>
#include <QFrame>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
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
