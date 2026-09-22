// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "core/BoundedBuffer.h"
#include "core/Canvas.h"
#include "core/FileService.h"
#include "core/Policy.h"
#include "core/Store.h"
#include <QFile>
#include <QJsonArray>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#include <filesystem>
#include <stdexcept>
using namespace mterm;
class CoreTests : public QObject {
    Q_OBJECT
  private slots:
    void incompleteUtf8Rejected() {
        QTemporaryDir d;
        QFile file(d.filePath("bad"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray("text\xc3"));
        file.close();
        FileService f(d.path());
        QVERIFY_EXCEPTION_THROWN(f.read("bad"), std::runtime_error);
    }
    void rootVolumeRead() {
        QTemporaryDir d;
        QFile file(d.filePath("ok"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("ok");
        file.close();
        const QString root = QDir(d.path()).rootPath();
        try {
            FileService f(root);
            QCOMPARE(f.read(QDir(root).relativeFilePath(file.fileName())).text, QString("ok"));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void hardLinkDenied() {
        QTemporaryDir d;
        QFile file(d.filePath("original"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
        file.close();
        std::error_code ec;
#ifdef Q_OS_WIN
        std::filesystem::create_hard_link(std::filesystem::path(file.fileName().toStdWString()),
                                          std::filesystem::path(d.filePath("alias").toStdWString()),
                                          ec);
#else
        std::filesystem::create_hard_link(file.fileName().toStdString(),
                                          d.filePath("alias").toStdString(), ec);
#endif
        if (ec)
            QSKIP("Filesystem does not permit hardlinks");
        FileService f(d.path());
        QVERIFY_EXCEPTION_THROWN(f.read("alias"), std::runtime_error);
    }
    void recordKindCannotChange() {
        QTemporaryDir d;
        Store s(d.filePath("db"));
        s.putRecord("task", "w", {{"id", "same"}});
        QVERIFY_EXCEPTION_THROWN(s.putRecord("evidence", "w", {{"id", "same"}}),
                                 std::runtime_error);
    }
    void futureDatabaseRefused() {
        QTemporaryDir d;
        const auto file = d.filePath("db"), name = QUuid::createUuid().toString();
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", name);
            db.setDatabaseName(file);
            QVERIFY(db.open());
            {
                QSqlQuery q(db);
                QVERIFY(q.exec("PRAGMA user_version=999"));
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(name);
        QVERIFY_EXCEPTION_THROWN(Store{file}, std::runtime_error);
    }
    void legacyMigrationPreservesRows() {
        QTemporaryDir d;
        const auto file = d.filePath("db"), name = QUuid::createUuid().toString();
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", name);
            db.setDatabaseName(file);
            QVERIFY(db.open());
            {
                QSqlQuery q(db);
                QVERIFY(q.exec("CREATE TABLE settings(key TEXT PRIMARY KEY,value TEXT NOT NULL)"));
                QVERIFY(q.exec("CREATE TABLE records(id TEXT PRIMARY KEY,kind TEXT NOT "
                               "NULL,updatedAt TEXT NOT NULL,payload TEXT NOT NULL)"));
                QVERIFY(q.exec("CREATE TABLE audit(id TEXT PRIMARY KEY,timestamp TEXT NOT "
                               "NULL,payload TEXT NOT NULL)"));
                QVERIFY(q.exec("INSERT INTO settings VALUES('legacy','{\"value\":42}')"));
                QVERIFY(q.exec(
                    "INSERT INTO records "
                    "VALUES('t','task','2026-01-01','{\"id\":\"t\",\"workspaceId\":\"w\"}')"));
                QVERIFY(q.exec("PRAGMA user_version=2"));
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(name);
        Store s(file);
        QCOMPARE(s.schemaVersion(), Store::SchemaVersion);
        QCOMPARE(s.setting("legacy")["value"].toInt(), 42);
        QCOMPARE(s.records("task", "w").size(), 1);
        QVERIFY(s.integrity());
    }
    void observeReads() {
        Policy p("w");
        QVERIFY(p.allows("filesystem.read", 0));
        QVERIFY(!p.allows("filesystem.write", 0));
    }
    void explicitExecution() {
        Policy p("w");
        p.setDeveloper(true);
        QVERIFY(p.allows("filesystem.write", 0));
        QVERIFY(!p.allows("terminal.execute", 0));
        QVERIFY(p.grant("terminal.execute", 100, 1000));
        QVERIFY(p.allows("terminal.execute", 101));
        QVERIFY(!p.allows("terminal.execute", 1100));
    }
    void revokeOnSwitch() {
        Policy p("a");
        p.setDeveloper(true);
        p.grant("agent.execute", 1, 100);
        p.setWorkspace("b");
        QVERIFY(!p.allows("agent.execute", 2));
        QVERIFY(!p.developer());
    }
    void revokeOnObserve() {
        Policy p("a");
        p.setDeveloper(true);
        p.grant("agent.execute", 1, 100);
        p.setDeveloper(false);
        p.setDeveloper(true);
        QVERIFY(!p.allows("agent.execute", 2));
    }
    void failClosed() {
        Policy p("w");
        p.setDeveloper(true);
        QVERIFY(!p.allows("made.up", 0));
        QVERIFY(!p.grant("made.up", 0, 10));
        QVERIFY(!p.grant("terminal.execute", 0, -1));
    }
    void emptyWorkspaceDenied() {
        Policy p;
        p.setDeveloper(true);
        QVERIFY(!p.allows("filesystem.read", 0));
    }
    void tailBound() {
        BoundedBuffer b(5);
        b.append("abc");
        b.append("defgh");
        QCOMPARE(b.bytes(), QByteArray("defgh"));
        QCOMPARE(b.droppedBytes(), 3);
        b.append("0123456789");
        QCOMPARE(b.bytes(), QByteArray("56789"));
        QCOMPARE(b.droppedBytes(), 13);
        b.clear();
        QVERIFY(b.bytes().isEmpty());
    }
    void zeroBuffer() {
        BoundedBuffer b(0);
        b.append("abc");
        QVERIFY(b.bytes().isEmpty());
        QCOMPARE(b.droppedBytes(), 3);
    }
    void rejectPaths_data() {
        QTest::addColumn<QString>("p");
        for (auto p : QStringList{"../secret", "/absolute", "C:\\file", "C:relative",
                                  "\\\\host\\file", ".git/config", ".env", "sub/.ssh/key",
                                  "a:stream", "trailing.", "CON", "nul.txt", "a/../b"})
            QTest::newRow(qPrintable(p)) << p;
    }
    void rejectPaths() {
        QFETCH(QString, p);
        QTemporaryDir d;
        FileService f(d.path());
        QVERIFY_EXCEPTION_THROWN(f.resolve(p, true), std::runtime_error);
    }
    void filesReadWrite() {
        QTemporaryDir d;
        QFile file(d.filePath("a.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("before");
        file.close();
        FileService f(d.path());
        try {
            auto s = f.read("a.txt");
            QCOMPARE(s.text, QString("before"));
            auto n = f.write("a.txt", "after", s.version);
            QCOMPARE(n.text, QString("after"));
            QVERIFY(n.version != s.version);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void staleWrite() {
        QTemporaryDir d;
        QFile file(d.filePath("a"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("aa");
        file.close();
        FileService f(d.path());
        try {
            auto old = f.read("a");
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("bb");
            file.close();
            QVERIFY_EXCEPTION_THROWN(f.write("a", "cc", old.version), std::runtime_error);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void createFile() {
        QTemporaryDir d;
        FileService f(d.path());
        try {
            auto n = f.write("new.txt", "hello", "missing");
            QCOMPARE(n.text, QString("hello"));
            QVERIFY_EXCEPTION_THROWN(f.write("new.txt", "lost", "missing"), std::runtime_error);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void oversizedRead() {
        QTemporaryDir d;
        QFile file(d.filePath("big"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray(FileService::MaxBytes + 1, 'x'));
        file.close();
        FileService f(d.path());
        QVERIFY_EXCEPTION_THROWN(f.read("big"), std::runtime_error);
    }
    void canvasValid() {
        auto c = initialCanvas();
        QVERIFY2(validateCanvas(c).isEmpty(), qPrintable(validateCanvas(c)));
    }
    void canvasDangling() {
        auto c = initialCanvas();
        c["edges"] =
            QJsonArray{QJsonObject{{"id", "e"}, {"source", "missing"}, {"target", "missing"}}};
        QVERIFY(!validateCanvas(c).isEmpty());
    }
    void canvasDuplicate() {
        auto c = initialCanvas();
        auto n = newNode("note", "n", "text");
        c["nodes"] = QJsonArray{n, n};
        QVERIFY(!validateCanvas(c).isEmpty());
    }
    void canvasBounds() {
        auto c = initialCanvas();
        auto n = newNode("note", "n", "text");
        n["width"] = -1;
        c["nodes"] = QJsonArray{n};
        QVERIFY(!validateCanvas(c).isEmpty());
    }
    void storeRoundtrip() {
        QTemporaryDir d;
        const auto path = d.filePath("test.sqlite");
        try {
            {
                Store s(path);
                QCOMPARE(s.schemaVersion(), Store::SchemaVersion);
                QVERIFY(s.integrity());
                s.setSetting("w", {{"root", "x"}});
                s.putRecord("task", "w", {{"id", "t"}, {"title", "hello"}});
                s.audit("w", "test", "ALLOW", "ok");
            }
            {
                Store s(path);
                QCOMPARE(s.setting("w")["root"].toString(), QString("x"));
                QCOMPARE(s.records("task", "w").size(), 1);
                QCOMPARE(s.records("task", "other").size(), 0);
                QCOMPARE(s.audits("w").size(), 1);
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void storeRollback() {
        QTemporaryDir d;
        try {
            Store s(d.filePath("db"));
            QVERIFY_EXCEPTION_THROWN(s.transaction([&] {
                s.putRecord("task", "w", {{"id", "t"}});
                throw std::runtime_error("rollback");
            }),
                                     std::runtime_error);
            QCOMPARE(s.records("task", "w").size(), 0);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void pagination() {
        QTemporaryDir d;
        try {
            Store s(d.filePath("db"));
            for (int i = 0; i < 5; ++i)
                s.putRecord("task", "w", {{"id", QString::number(i)}});
            QCOMPARE(s.records("task", "w", 2).size(), 2);
            QCOMPARE(s.records("task", "w", 2, 4).size(), 1);
            QVERIFY_EXCEPTION_THROWN(s.records("task", "w", 0), std::runtime_error);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "CoreTests.moc"
