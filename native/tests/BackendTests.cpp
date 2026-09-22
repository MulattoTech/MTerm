// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-resume-native (see docs/ai/changes/)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "services/Backend.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>
using namespace mterm;
struct Reply {
    QJsonObject result;
    QString error;
};
static Reply call(Backend &b, const QString &method, QJsonObject args = {}) {
    QSignalSpy spy(&b, &Backend::response);
    const auto id = b.request(method, args);
    if (!spy.wait(5000))
        return {{}, "response timeout"};
    for (const auto &x : spy)
        if (x[0].toULongLong() == id)
            return {x[2].toJsonObject(), x[3].toString()};
    return {{}, "wrong response"};
}
class BackendTests : public QObject {
    Q_OBJECT
  private slots:
    void malformedWriteIsRejected_data() {
        QTest::addColumn<QJsonValue>("text");
        QTest::newRow("number") << QJsonValue(42);
        QTest::newRow("boolean") << QJsonValue(false);
        QTest::newRow("null") << QJsonValue(QJsonValue::Null);
        QTest::newRow("array") << QJsonValue(QJsonArray{"not-text"});
        QTest::newRow("object") << QJsonValue(QJsonObject{{"value", "not-text"}});
        QTest::newRow("missing") << QJsonValue(QJsonValue::Undefined);
    }
    void malformedWriteIsRejected() {
        QFETCH(QJsonValue, text);
        QTemporaryDir d;
        QFile f(d.filePath("keep.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("preserved");
        f.close();
        Backend b(d.filePath("data/db"));
        const auto opened = call(b, "open", {{"root", d.path()}});
        QVERIFY(opened.error.isEmpty());
        const auto id = opened.result["workspaceId"];
        QVERIFY(call(b, "profile", {{"workspaceId", id}, {"developer", true}}).error.isEmpty());
        const auto before = call(b, "read-file", {{"workspaceId", id}, {"path", "keep.txt"}});
        QVERIFY(before.error.isEmpty());
        const auto result = call(b, "write-file",
                                 {{"workspaceId", id},
                                  {"path", "keep.txt"},
                                  {"version", before.result["version"]},
                                  {"text", text}});
        QVERIFY2(!result.error.isEmpty(),
                 "Malformed text must fail, never become an empty overwrite");
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("preserved"));
    }
    void malformedTaskAndNodeRejected() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto state = call(b, "open", {{"root", d.path()}});
        QVERIFY(state.error.isEmpty());
        const auto id = state.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        QVERIFY(!call(b, "create-task", {{"workspaceId", id}, {"title", "test"}, {"objective", 42}})
                     .error.isEmpty());
        QVERIFY(!call(b, "add-node", {{"workspaceId", id}, {"kind", 42}}).error.isEmpty());
        QVERIFY(
            !call(b, "add-node", {{"workspaceId", id}, {"content", QJsonArray{}}}).error.isEmpty());
        state = call(b, "state", {{"workspaceId", id}});
        QCOMPARE(state.result["tasks"].toArray().size(), 0);
        QCOMPARE(state.result["canvas"].toObject()["nodes"].toArray().size(), 1);
    }
    void malformedWorkspaceOpenRejected() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto state = call(b, "open", {{"root", d.path()}});
        QVERIFY(state.error.isEmpty());
        const auto id = state.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        QVERIFY(!call(b, "open", {{"root", true}}).error.isEmpty());
        state = call(b, "state", {{"workspaceId", id}});
        QCOMPARE(state.result["profile"].toString(), QString("developer"));
    }
    void ptyRequiresApprovalAndCaptures() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY adapter is Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        QVERIFY(!call(b, "pty-start", {{"workspaceId", id}, {"shell", "cmd"}}).error.isEmpty());
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        QVERIFY(call(b, "grant", {{"workspaceId", id}, {"capability", "terminal.execute"}})
                    .error.isEmpty());
        QByteArray output;
        connect(&b, &Backend::terminalOutput, this,
                [&](const QString &, const QByteArray &bytes) { output += bytes; });
        auto start = call(b, "pty-start",
                          {{"workspaceId", id}, {"shell", "cmd"}, {"columns", 100}, {"rows", 30}});
        QVERIFY2(start.error.isEmpty(), qPrintable(start.error));
        QByteArray input = "set MTERM_T=BACKEND_OK\r\necho MTERM_%MTERM_T%\r\n";
        QVERIFY(call(b, "pty-write",
                     {{"workspaceId", id}, {"dataBase64", QString::fromLatin1(input.toBase64())}})
                    .error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(output.contains("MTERM_BACKEND_OK"), 8000);
        QVERIFY(!call(b, "pty-write", {{"workspaceId", "wrong"}, {"dataBase64", "eA=="}})
                     .error.isEmpty());
        QVERIFY(call(b, "pty-stop", {{"workspaceId", id}}).error.isEmpty());
#endif
    }
    void restoresLastWorkspace() {
        QTemporaryDir d;
        const auto db = d.filePath("data/db");
        {
            Backend b(db);
            auto s = call(b, "open", {{"root", d.path()}});
            QVERIFY(s.error.isEmpty());
        }
        {
            Backend b(db);
            auto s = call(b, "open", {{"root", ""}});
            QVERIFY2(s.error.isEmpty(), qPrintable(s.error));
            QCOMPARE(s.result["root"].toString(), QFileInfo(d.path()).canonicalFilePath());
        }
    }
    void providerFinalFrames_data() {
        QTest::addColumn<QString>("prompt");
        QTest::newRow("provider-error-final") << QString("FAIL_FINAL");
        QTest::newRow("malformed-final") << QString("MALFORMED_FINAL");
        QTest::newRow("no-completion") << QString("NO_COMPLETION");
    }
    void providerFinalFrames() {
        QFETCH(QString, prompt);
        QTemporaryDir d;
        auto old = qgetenv("MTERM_CODEX_EXECUTABLE");
        auto restore = qScopeGuard([&] {
            if (old.isNull())
                qunsetenv("MTERM_CODEX_EXECUTABLE");
            else
                qputenv("MTERM_CODEX_EXECUTABLE", old);
        });
        auto exe = QDir(QCoreApplication::applicationDirPath())
                       .filePath("mterm-test-child"
#ifdef Q_OS_WIN
                                 ".exe"
#endif
                       );
        qputenv("MTERM_CODEX_EXECUTABLE", exe.toUtf8());
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        call(b, "grant", {{"workspaceId", id}, {"capability", "agent.execute"}});
        QSignalSpy done(&b, &Backend::runFinished);
        auto run = call(b, "run", {{"workspaceId", id}, {"kind", "codex"}, {"prompt", prompt}});
        QVERIFY2(run.error.isEmpty(), qPrintable(run.error));
        if (done.isEmpty())
            QVERIFY(done.wait(5000));
        auto state = call(b, "state", {{"workspaceId", id}});
        QVERIFY(state.error.isEmpty());
        auto sessions = state.result["sessions"].toArray();
        QCOMPARE(sessions.size(), 1);
        QCOMPARE(sessions[0].toObject()["status"].toString(), QString("FAILED"));
    }
    void providerFreshAndResumeFixture() {
        QTemporaryDir d;
        auto old = qgetenv("MTERM_CODEX_EXECUTABLE");
        auto restore = qScopeGuard([&] {
            if (old.isNull())
                qunsetenv("MTERM_CODEX_EXECUTABLE");
            else
                qputenv("MTERM_CODEX_EXECUTABLE", old);
        });
        auto exe = QDir(QCoreApplication::applicationDirPath())
                       .filePath("mterm-test-child"
#ifdef Q_OS_WIN
                                 ".exe"
#endif
                       );
        qputenv("MTERM_CODEX_EXECUTABLE", exe.toUtf8());
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        call(b, "grant", {{"workspaceId", id}, {"capability", "agent.execute"}});
        QString output;
        connect(&b, &Backend::stream, this,
                [&](const QString &, const QString &, const QString &t) { output += t; });
        QSignalSpy done(&b, &Backend::runFinished);
        auto run = call(b, "run", {{"workspaceId", id}, {"kind", "codex"}, {"prompt", "OK_FINAL"}});
        QVERIFY(run.error.isEmpty());
        if (done.isEmpty())
            QVERIFY(done.wait(5000));
        QVERIFY(output.contains("MTERM_FIXTURE_OK"));
        auto state = call(b, "state", {{"workspaceId", id}});
        auto session = state.result["sessions"].toArray()[0].toObject();
        QCOMPARE(session["status"].toString(), QString("DONE"));
        QCOMPARE(session["resumabilityData"].toObject()["threadId"].toString(),
                 QString("mterm-fixture-thread"));
        done.clear();
        run = call(b, "run",
                   {{"workspaceId", id},
                    {"kind", "codex"},
                    {"prompt", "OK_FINAL"},
                    {"resumeId", session["id"]}});
        QVERIFY(run.error.isEmpty());
        if (done.isEmpty())
            QVERIFY(done.wait(5000));
        QVERIFY(output.contains("MTERM_FIXTURE_RESUMED"));
        state = call(b, "state", {{"workspaceId", id}});
        QCOMPARE(state.result["sessions"].toArray().size(), 1);
        QCOMPARE(state.result["evidence"].toArray().size(), 2);
    }
    void openObserve() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY2(s.error.isEmpty(), qPrintable(s.error));
        QCOMPARE(s.result["profile"].toString(), QString("observe"));
        QVERIFY(s.result["workspaceId"].isString());
        QVERIFY(s.result["canvas"].isObject());
    }
    void deniedWrites() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        auto x = call(b, "add-node", {{"workspaceId", id}, {"title", "No"}, {"content", "denied"}});
        QVERIFY(!x.error.isEmpty());
        auto g = call(b, "grant", {{"workspaceId", id}, {"capability", "terminal.execute"}});
        QVERIFY(!g.error.isEmpty());
    }
    void persistentNodesAndTasks() {
        QTemporaryDir d;
        QString id;
        {
            Backend b(d.filePath("data/db"));
            auto s = call(b, "open", {{"root", d.path()}});
            QVERIFY(s.error.isEmpty());
            id = s.result["workspaceId"].toString();
            QVERIFY(call(b, "profile", {{"workspaceId", id}, {"developer", true}}).error.isEmpty());
            auto n =
                call(b, "add-node", {{"workspaceId", id}, {"title", "Saved"}, {"content", "note"}});
            QVERIFY2(n.error.isEmpty(), qPrintable(n.error));
            QCOMPARE(n.result["canvas"].toObject()["nodes"].toArray().size(), 2);
            auto t =
                call(b, "create-task",
                     {{"workspaceId", id}, {"title", "Test"}, {"objective", "Native persistence"}});
            QVERIFY(t.error.isEmpty());
            QCOMPARE(t.result["tasks"].toArray().size(), 1);
        }
        {
            Backend b(d.filePath("data/db"));
            auto s = call(b, "open", {{"root", d.path()}});
            QVERIFY(s.error.isEmpty());
            QCOMPARE(s.result["workspaceId"].toString(), id);
            QCOMPARE(s.result["canvas"].toObject()["nodes"].toArray().size(), 2);
            QCOMPARE(s.result["tasks"].toArray().size(), 1);
            QCOMPARE(s.result["profile"].toString(), QString("observe"));
        }
    }
    void staleCanvas() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        call(b, "add-node", {{"workspaceId", id}, {"title", "New"}});
        auto x = call(b, "save-canvas",
                      {{"workspaceId", id},
                       {"canvas", s.result["canvas"]},
                       {"revision", s.result["revision"]}});
        QVERIFY(!x.error.isEmpty());
    }
    void wrongWorkspaceDenied() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto x = call(b, "profile", {{"workspaceId", "other"}, {"developer", true}});
        QVERIFY(!x.error.isEmpty());
    }
    void runApproval() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        auto s = call(b, "open", {{"root", d.path()}});
        QVERIFY(s.error.isEmpty());
        auto id = s.result["workspaceId"];
        call(b, "profile", {{"workspaceId", id}, {"developer", true}});
        QString program = QDir(QCoreApplication::applicationDirPath())
                              .filePath("mterm-test-child"
#ifdef Q_OS_WIN
                                        ".exe"
#endif
                              );
        QJsonObject args{{"workspaceId", id},
                         {"kind", "command"},
                         {"program", program},
                         {"arguments", QJsonArray{}}};
        QVERIFY(!call(b, "run", args).error.isEmpty());
        QVERIFY(call(b, "grant", {{"workspaceId", id}, {"capability", "terminal.execute"}})
                    .error.isEmpty());
        QSignalSpy done(&b, &Backend::runFinished);
        auto run = call(b, "run", args);
        QVERIFY2(run.error.isEmpty(), qPrintable(run.error));
        QVERIFY(run.result["runId"].isString());
        if (done.isEmpty())
            QVERIFY(done.wait(5000));
        QCOMPARE(done[0][2].toJsonObject()["exitCode"].toInt(), 0);
    }
};
QTEST_GUILESS_MAIN(BackendTests)
#include "BackendTests.moc"
