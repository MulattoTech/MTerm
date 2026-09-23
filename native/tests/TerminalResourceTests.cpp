// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-terminal-candidate (OpenAI / GPT-6 Astra Pro)
#include "core/Store.h"
#include "services/Backend.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
using namespace mterm;
namespace {
struct Reply {
    QJsonObject value;
    QString error;
};
Reply call(Backend &b, const QString &method, QJsonObject args = {}) {
    QSignalSpy spy(&b, &Backend::response);
    const auto id = b.request(method, args);
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        for (const auto &r : spy)
            if (r[0].toULongLong() == id)
                return {r[2].toJsonObject(), r[3].toString()};
        spy.wait(50);
    }
    return {{}, "response timeout"};
}
QString setup(Backend &b, const QString &root) {
    auto r = call(b, "open", {{"root", root}});
    if (!r.error.isEmpty())
        return {};
    const auto id = r.value["workspaceId"].toString();
    if (!call(b, "profile", {{"workspaceId", id}, {"developer", true}}).error.isEmpty())
        return {};
    if (!call(b, "grant", {{"workspaceId", id}, {"capability", "terminal.execute"}})
             .error.isEmpty())
        return {};
    return id;
}
QString node(Backend &b, const QString &w, const QString &title = "Terminal",
             const QString &kind = "terminal") {
    auto r =
        call(b, "add-node",
             {{"workspaceId", w}, {"kind", kind}, {"title", title}, {"content", "isolated test"}});
    if (!r.error.isEmpty())
        return {};
    return r.value["canvas"].toObject()["nodes"].toArray().last().toObject()["id"].toString();
}
QJsonObject args(const QString &w, const QString &id) {
    return {
        {"workspaceId", w}, {"terminalId", id}, {"shell", "cmd"}, {"columns", 100}, {"rows", 30}};
}
QByteArray bytesFor(const QSignalSpy &spy, const QString &resource) {
    QByteArray out;
    for (const auto &e : spy)
        if (e[1].toString() == resource)
            out += e[3].toByteArray();
    return out;
}
Reply input(Backend &b, QJsonObject request, const QString &run, const QByteArray &bytes) {
    request["runId"] = run;
    request["dataBase64"] = QString::fromLatin1(bytes.toBase64());
    return call(b, "pty-write", request);
}
} // namespace
class TerminalResourceTests final : public QObject {
    Q_OBJECT
  private slots:
    void twoShellsHaveDistinctStateAndStoppingOnePreservesOther() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path());
        QVERIFY(!w.isEmpty());
        const auto a = node(b, w, "Build"), c = node(b, w, "Logs");
        QVERIFY(!a.isEmpty());
        QVERIFY(!c.isEmpty());
        auto first = call(b, "pty-start", args(w, a));
        QVERIFY2(first.error.isEmpty(), qPrintable(first.error));
        auto second = call(b, "pty-start", args(w, c));
        QVERIFY2(second.error.isEmpty(), qPrintable(second.error));
        const auto runA = first.value["runId"].toString(), runC = second.value["runId"].toString();
        QVERIFY(!runA.isEmpty());
        QVERIFY(runA != runC);
        QSignalSpy output(&b, SIGNAL(terminalSessionOutput(QString, QString, QString, QByteArray)));
        QVERIFY(output.isValid());
        QVERIFY(input(b, args(w, a), runA,
                      "set MTERM_ISOLATION=FIRST\r\necho PROOF_%MTERM_ISOLATION%\r\n")
                    .error.isEmpty());
        QVERIFY(input(b, args(w, c), runC,
                      "set MTERM_ISOLATION=SECOND\r\necho PROOF_%MTERM_ISOLATION%\r\n")
                    .error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, a).contains("PROOF_FIRST"), 8000);
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, c).contains("PROOF_SECOND"), 8000);
        QVERIFY(!bytesFor(output, a).contains("PROOF_SECOND"));
        QVERIFY(!bytesFor(output, c).contains("PROOF_FIRST"));
        auto stop = args(w, a);
        stop["runId"] = runA;
        QVERIFY(call(b, "pty-stop", stop).error.isEmpty());
        QVERIFY(input(b, args(w, c), runC, "echo STILL_%MTERM_ISOLATION%\r\n").error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, c).contains("STILL_SECOND"), 8000);
#endif
    }
    void staleRunCannotStopOrWriteRestartedShell() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path()), id = node(b, w);
        auto req = args(w, id);
        auto one = call(b, "pty-start", req);
        QVERIFY(one.error.isEmpty());
        const auto old = one.value["runId"].toString();
        QVERIFY(!old.isEmpty());
        req["runId"] = old;
        QVERIFY(call(b, "pty-stop", req).error.isEmpty());
        req.remove("runId");
        auto two = call(b, "pty-start", req);
        QVERIFY(two.error.isEmpty());
        QVERIFY(two.value["runId"] != old);
        req["runId"] = old;
        QVERIFY(!call(b, "pty-stop", req).error.isEmpty());
        QVERIFY(!input(b, req, old, "exit\r\n").error.isEmpty());
        req.remove("runId");
        QVERIFY(!call(b, "pty-resize", req).error.isEmpty());
        auto list = call(b, "pty-list", {{"workspaceId", w}});
        QVERIFY(list.error.isEmpty());
        QCOMPARE(list.value["activeCount"].toInt(), 1);
#endif
    }
    void invalidResourceProfileCwdAndDimensionsAreRejected() {
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path());
        const auto note = node(b, w, "Not a terminal", "note");
        QVERIFY(!call(b, "pty-start", args(w, "not-a-resource")).error.isEmpty());
        QVERIFY(!call(b, "pty-start", args(w, note)).error.isEmpty());
        auto req = args(w, node(b, w));
        req["cwd"] = "..";
        QVERIFY(!call(b, "pty-start", req).error.isEmpty());
        req["cwd"] = 17;
        QVERIFY(!call(b, "pty-start", req).error.isEmpty());
        req.remove("cwd");
        req["columns"] = 10.5;
        QVERIFY(!call(b, "pty-start", req).error.isEmpty());
        req["columns"] = 100;
        req["shell"] = true;
        QVERIFY(!call(b, "pty-start", req).error.isEmpty());
        auto list = call(b, "pty-list", {{"workspaceId", w}});
        QVERIFY(list.error.isEmpty());
        QCOMPARE(list.value["activeCount"].toInt(), 0);
    }
    void cwdMetadataRestoresStoppedWithoutTerminalText() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        QVERIFY(QDir(d.path()).mkdir("working"));
        QString w, id;
        {
            Backend b(d.filePath("data/db"));
            w = setup(b, d.path());
            id = node(b, w, "Build terminal");
            auto req = args(w, id);
            req["cwd"] = "working";
            QSignalSpy output(&b,
                              SIGNAL(terminalSessionOutput(QString, QString, QString, QByteArray)));
            QVERIFY(output.isValid());
            auto run = call(b, "pty-start", req);
            QVERIFY(run.error.isEmpty());
            const auto runId = run.value["runId"].toString();
            QVERIFY(input(b, req, runId, "echo CHECK_%CD%\r\n").error.isEmpty());
            QTRY_VERIFY_WITH_TIMEOUT(
                bytesFor(output, id)
                    .contains(QDir::toNativeSeparators(d.filePath("working")).toUtf8()),
                8000);
        }
        {
            Backend b(d.filePath("data/db"));
            auto opened = call(b, "open", {{"root", d.path()}});
            QVERIFY(opened.error.isEmpty());
            auto list = call(b, "pty-list", {{"workspaceId", w}});
            QVERIFY(list.error.isEmpty());
            QCOMPARE(list.value["activeCount"].toInt(), 0);
            const auto records = list.value["items"].toArray();
            QCOMPARE(records.size(), 1);
            const auto entry = records[0].toObject();
            QCOMPARE(entry["terminalId"].toString(), id);
            QCOMPARE(entry["cwd"].toString(), QString("working"));
            QCOMPARE(entry["status"].toString(), QString("STOPPED"));
            QVERIFY(!QJsonDocument(entry).toJson().contains("CHECK_"));
            QVERIFY(!entry.contains("output"));
            QVERIFY(!call(b, "pty-start", args(w, id)).error.isEmpty());
        }
#endif
    }
    void fullHistoryRecoveryIsScopedNotLimitedToFirstPage() {
        QTemporaryDir d;
        const auto db = d.filePath("data/db");
        QString w;
        {
            Backend b(db);
            auto r = call(b, "open", {{"root", d.path()}});
            QVERIFY(r.error.isEmpty());
            w = r.value["workspaceId"].toString();
        }
        {
            Store s(db);
            s.transaction([&] {
                for (int i = 0; i < 250; ++i) {
                    s.putRecord("agent-session", w,
                                {{"id", QString("agent-%1").arg(i)}, {"status", "RUNNING"}});
                    s.putRecord("terminal-resource", w,
                                {{"id", QString("terminal-%1").arg(i)}, {"status", "RUNNING"}});
                }
                s.putRecord("agent-session", "other", {{"id", "other-run"}, {"status", "RUNNING"}});
                s.putRecord("agent-session", w, {{"id", "already-done"}, {"status", "DONE"}});
            });
        }
        {
            Backend b(db);
            QVERIFY(call(b, "open", {{"root", d.path()}}).error.isEmpty());
        }
        {
            Store s(db);
            for (const auto *kind : {"agent-session", "terminal-resource"})
                for (int offset = 0; offset < 300; offset += 100)
                    for (const auto &v : s.records(kind, w, 100, offset)) {
                        const auto state = v.toObject()["status"].toString();
                        QVERIFY2(state == "STOPPED" || state == "DONE",
                                 "Old active records beyond first page must not remain RUNNING");
                    }
            QCOMPARE(s.records("agent-session", "other")[0].toObject()["status"].toString(),
                     QString("RUNNING"));
        }
    }
    void invalidWorkspaceOpenKeepsCurrentShellAlive() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path()), id = node(b, w);
        auto request = args(w, id);
        auto run = call(b, "pty-start", request);
        QVERIFY(run.error.isEmpty());
        QVERIFY(!call(b, "open", {{"root", d.filePath("missing-directory")}}).error.isEmpty());
        const auto list = call(b, "pty-list", {{"workspaceId", w}});
        QVERIFY(list.error.isEmpty());
        QCOMPARE(list.value["activeCount"].toInt(), 1);
        QSignalSpy output(&b, SIGNAL(terminalSessionOutput(QString, QString, QString, QByteArray)));
        QVERIFY(input(b, request, run.value["runId"].toString(), "echo PRESERVED_WORKSPACE\r\n")
                    .error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, id).contains("PRESERVED_WORKSPACE"), 8000);
#endif
    }
    void flowControlledOutputWaitsForTheConsumer() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path()), id = node(b, w);
        auto request = args(w, id);
        request["flowControl"] = true;
        QSignalSpy output(&b, SIGNAL(terminalSessionOutput(QString, QString, QString, QByteArray)));
        const auto started = call(b, "pty-start", request);
        QVERIFY(started.error.isEmpty());
        QVERIFY(started.value["flowControl"].toBool());
        const auto run = started.value["runId"].toString();
        QVERIFY(input(b, request, run,
                      "for /L %i in (1,1,90000) do @echo bounded-terminal-output-%i\r\n")
                    .error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, id).size() >= 262144, 8000);
        const auto before = bytesFor(output, id).size();
        QVERIFY(before <= 327680);
        QTest::qWait(250);
        QCOMPARE(bytesFor(output, id).size(), before);
        auto ack = request;
        ack["runId"] = run;
        ack["bytes"] = before + 1;
        QVERIFY(!call(b, "pty-ack", ack).error.isEmpty());
        ack["bytes"] = before;
        QVERIFY(call(b, "pty-ack", ack).error.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(bytesFor(output, id).size() > before, 3000);
        request["runId"] = run;
        QVERIFY(call(b, "pty-stop", request).error.isEmpty());
#endif
    }
    void reopeningSameRootChangesEpochAndRevokesOldRun() {
#ifndef Q_OS_WIN
        QSKIP("Windows ConPTY lifecycle regression");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path()), id = node(b, w);
        const auto before = call(b, "state", {{"workspaceId", w}});
        QVERIFY(before.error.isEmpty());
        const auto started = call(b, "pty-start", args(w, id));
        QVERIFY(started.error.isEmpty());
        const auto reopened = call(b, "open", {{"root", d.path()}});
        QVERIFY(reopened.error.isEmpty());
        QCOMPARE(reopened.value["workspaceId"].toString(), w);
        QVERIFY(reopened.value["sessionEpoch"] != before.value["sessionEpoch"]);
        QCOMPARE(reopened.value["profile"].toString(), QString("observe"));
        auto request = args(w, id);
        request["runId"] = started.value["runId"];
        QVERIFY(!call(b, "pty-start", request).error.isEmpty());
        QVERIFY(!input(b, request, started.value["runId"].toString(), "echo SHOULD_NOT_RUN\r\n")
                     .error.isEmpty());
        const auto list = call(b, "pty-list", {{"workspaceId", w}});
        QVERIFY(list.error.isEmpty());
        QCOMPARE(list.value["activeCount"].toInt(), 0);
        QCOMPARE(list.value["items"].toArray().first().toObject()["status"].toString(),
                 QString("STOPPED"));
#endif
    }
    void naturalExitAndStaleAckCannotAffectRestart() {
#ifndef Q_OS_WIN
        QSKIP("Windows ConPTY lifecycle regression");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path()), id = node(b, w);
        auto request = args(w, id);
        request["flowControl"] = true;
        QSignalSpy events(&b,
                          SIGNAL(terminalSessionChanged(QString, QString, QString, QJsonObject)));
        const auto started = call(b, "pty-start", request);
        QVERIFY(started.error.isEmpty());
        const auto oldRun = started.value["runId"].toString();
        QVERIFY(input(b, request, oldRun, "exit 7\r\n").error.isEmpty());
        auto finished = [&] {
            for (const auto &e : events)
                if (e[2].toString() == oldRun && e[3].toJsonObject()["status"] == "FAILED")
                    return true;
            return false;
        };
        QTRY_VERIFY_WITH_TIMEOUT(finished(), 5000);
        auto list = call(b, "pty-list", {{"workspaceId", w}});
        QCOMPARE(list.value["activeCount"].toInt(), 0);
        QCOMPARE(list.value["items"].toArray().first().toObject()["exitCode"].toInt(), 7);
        const auto restarted = call(b, "pty-start", request);
        QVERIFY(restarted.error.isEmpty());
        QVERIFY(restarted.value["runId"] != oldRun);
        request["runId"] = oldRun;
        request["bytes"] = 1;
        QVERIFY(!call(b, "pty-ack", request).error.isEmpty());
        QVERIFY(!call(b, "pty-stop", request).error.isEmpty());
        QCOMPARE(call(b, "pty-list", {{"workspaceId", w}}).value["activeCount"].toInt(), 1);
#endif
    }
    void workspaceChangeCannotControlAnotherWorkspacesRun() {
#ifndef Q_OS_WIN
        QSKIP("Windows ConPTY scope regression");
#else
        QTemporaryDir d;
        QDir(d.path()).mkdir("A");
        QDir(d.path()).mkdir("B");
        Backend b(d.filePath("data/db"));
        const auto a = setup(b, d.filePath("A")), idA = node(b, a);
        const auto runA = call(b, "pty-start", args(a, idA));
        QVERIFY(runA.error.isEmpty());
        const auto other = setup(b, d.filePath("B")), idB = node(b, other);
        const auto runB = call(b, "pty-start", args(other, idB));
        QVERIFY(runB.error.isEmpty());
        auto stale = args(a, idA);
        stale["runId"] = runA.value["runId"];
        QVERIFY(!call(b, "pty-stop", stale).error.isEmpty());
        auto list = call(b, "pty-list", {{"workspaceId", other}});
        QVERIFY(list.error.isEmpty());
        QCOMPARE(list.value["activeCount"].toInt(), 1);
        QCOMPARE(list.value["items"].toArray().size(), 1);
        QCOMPARE(list.value["items"].toArray().first().toObject()["terminalId"].toString(), idB);
#endif
    }
    void capacityAndRevocationStopAllOwnedShells() {
#ifndef Q_OS_WIN
        QSKIP("Native PTY remains Windows-only");
#else
        QTemporaryDir d;
        Backend b(d.filePath("data/db"));
        const auto w = setup(b, d.path());
        auto listing = call(b, "pty-list", {{"workspaceId", w}});
        QVERIFY(listing.error.isEmpty());
        const int limit = listing.value["maxActive"].toInt();
        QVERIFY(limit >= 2 && limit <= 8);
        for (int i = 0; i < limit; ++i) {
            auto r = call(b, "pty-start", args(w, node(b, w, QString("T%1").arg(i))));
            QVERIFY2(r.error.isEmpty(), qPrintable(r.error));
        }
        QVERIFY(!call(b, "pty-start", args(w, node(b, w, "Overflow"))).error.isEmpty());
        QVERIFY(call(b, "profile", {{"workspaceId", w}, {"developer", false}}).error.isEmpty());
        listing = call(b, "pty-list", {{"workspaceId", w}});
        QCOMPARE(listing.value["activeCount"].toInt(), 0);
        for (const auto &v : listing.value["items"].toArray())
            QVERIFY(v.toObject()["status"] != "RUNNING");
#endif
    }
};
QTEST_GUILESS_MAIN(TerminalResourceTests)
#include "TerminalResourceTests.moc"
