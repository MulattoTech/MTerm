// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "terminal/PtySession.h"
#include "terminal/TerminalScreen.h"
#include <QDir>
#include <QtTest>
#include <stdexcept>
using namespace mterm;
class TerminalTests : public QObject {
    Q_OBJECT
  private slots:
    void gridAndText() {
        TerminalScreen s(10, 40);
        QCOMPARE(s.rows(), 10);
        QCOMPARE(s.columns(), 40);
        s.feed("hello");
        QVERIFY(s.text().contains("hello"));
        QCOMPARE(s.cell(0, 0).text, QString("h"));
    }
    void cursorAndErase() {
        TerminalScreen s(10, 40);
        s.feed("hello\rX");
        QVERIFY(s.text().contains("Xello"));
        s.feed("\x1b[2J\x1b[Hfresh");
        QVERIFY(!s.text().contains("ello"));
        QCOMPARE(s.cell(0, 0).text, QString("f"));
    }
    void color() {
        TerminalScreen s(10, 40);
        s.feed("\x1b[38;2;10;20;30mA");
        QCOMPARE(s.cell(0, 0).foreground, quint32(0xff0a141e));
    }
    void utf8Chunks() {
        TerminalScreen s(10, 40);
        s.feed("caf\xc3");
        s.feed("\xa9");
        QCOMPARE(s.cell(0, 3).text, QString::fromUtf8("\xc3\xa9"));
    }
    void keyEncoding() {
        TerminalScreen s(10, 40);
        QCOMPARE(s.key(VTERM_KEY_ENTER), QByteArray("\r"));
        QCOMPARE(s.unicode('c', VTERM_MOD_CTRL), QByteArray(1, '\x03'));
    }
    void replyToCursorQuery() {
        TerminalScreen s(10, 40);
        s.feed("abc");
        auto reply = s.feed("\x1b[6n");
        QCOMPARE(reply, QByteArray("\x1b[1;4R"));
    }
    void resizeBounds() {
        TerminalScreen s;
        QVERIFY_EXCEPTION_THROWN(s.resize(0, 100), std::runtime_error);
        s.resize(20, 60);
        QCOMPARE(s.rows(), 20);
        QCOMPARE(s.columns(), 60);
    }
    void ptyEchoAndStop() {
#ifndef Q_OS_WIN
        QSKIP("ConPTY is Windows-only; Unix PTY adapter is tracked separately.");
#else
        PtySession p;
        QByteArray captured;
        connect(&p, &PtySession::dataReady, this,
                [&](const QByteArray &data) { captured += data; });
        QSignalSpy exit(&p, &PtySession::exited);
        const auto cmd = qEnvironmentVariable("SystemRoot") + "/System32/cmd.exe";
        QVERIFY2(p.start(cmd, {"/D", "/Q"}, QDir::currentPath()), qPrintable(p.error()));
        QVERIFY(p.running());
        QVERIFY(p.resize(120, 35));
        QVERIFY(p.write("set MTERM_TEST_MARK=PTY_OK\r\necho MTERM_%MTERM_TEST_MARK%\r\n"));
        QTRY_VERIFY2_WITH_TIMEOUT(captured.contains("MTERM_PTY_OK"), captured.toHex().constData(),
                                  8000);
        p.stop();
        QVERIFY(!p.running());
        QCOMPARE(exit.size(), 1);
#endif
    }
    void ptyInvalidBounds() {
        PtySession p;
        QVERIFY(!p.start("absent", {}, QDir::currentPath(), 0, 0));
        QVERIFY(!p.write("x"));
    }
};
QTEST_GUILESS_MAIN(TerminalTests)
#include "TerminalTests.moc"
