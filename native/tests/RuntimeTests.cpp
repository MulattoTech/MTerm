// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <stdexcept>
#include "runtime/ProcessRunner.h"
#include "runtime/ProcessInventory.h"
#include "runtime/JsonLines.h"
using namespace mterm;
class RuntimeTests:public QObject{
 Q_OBJECT
 QString child()const {return QDir(QCoreApplication::applicationDirPath()).filePath("mterm-test-child"
#ifdef Q_OS_WIN
 ".exe"
#endif
 );}
 Command cmd(QStringList args={})const{return {child(),args,QDir::currentPath(),{},5000};}
private slots:
 void initTestCase(){qRegisterMetaType<ProcessResult>();}
 void capture(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);QVERIFY(r.start(cmd()));QTRY_COMPARE_WITH_TIMEOUT(s.size(),1,6000);auto result=qvariant_cast<ProcessResult>(s.at(0).at(0));QCOMPARE(result.exitCode,0);QCOMPARE(result.output.trimmed(),QByteArray("MTERM_CHILD_OK"));QVERIFY(result.error.isEmpty());}
 void stdinAndExit(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);auto c=cmd({"--stdin"});c.input="hello\n";QVERIFY(r.start(c));QTRY_COMPARE_WITH_TIMEOUT(s.size(),1,6000);QCOMPARE(qvariant_cast<ProcessResult>(s.at(0).at(0)).output,QByteArray("hello"));}
 void failedExit(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);QVERIFY(r.start(cmd({"--fail"})));QTRY_COMPARE(s.size(),1);auto x=qvariant_cast<ProcessResult>(s.at(0).at(0));QCOMPARE(x.exitCode,7);QCOMPARE(x.errors,QByteArray("deliberate"));}
 void boundedFlood(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);QVERIFY(r.start(cmd({"--flood"})));QTRY_COMPARE(s.size(),1);auto x=qvariant_cast<ProcessResult>(s.at(0).at(0));QCOMPARE(x.output.size(),262144);QCOMPARE(x.droppedBytes,600000-262144);}
 void timeout(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);auto c=cmd({"--wait"});c.timeoutMs=80;QVERIFY(r.start(c));QTRY_COMPARE(s.size(),1);QVERIFY(qvariant_cast<ProcessResult>(s.at(0).at(0)).timedOut);QVERIFY(!r.running());}
 void cancel(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);QVERIFY(r.start(cmd({"--wait"})));QTimer::singleShot(80,&r,&ProcessRunner::cancel);QTRY_COMPARE(s.size(),1);QVERIFY(qvariant_cast<ProcessResult>(s.at(0).at(0)).cancelled);}
 void noDoubleStart(){ProcessRunner r;QVERIFY(r.start(cmd({"--wait"})));QVERIFY(!r.start(cmd()));r.cancel();}
 void missingProgram(){ProcessRunner r;QSignalSpy s(&r,&ProcessRunner::completed);auto c=cmd();c.program="mterm-deliberately-absent-program";QVERIFY(r.start(c));QTRY_COMPARE(s.size(),1);QVERIFY(!qvariant_cast<ProcessResult>(s.at(0).at(0)).error.isEmpty());}
 void inventoryContainsSelf(){const auto list=processInventory(4096);bool found=false;for(const auto &v:list)if(v.toObject()["pid"].toInteger()==QCoreApplication::applicationPid())found=true;QVERIFY(found);}
 void jsonChunking(){JsonLines d;QVERIFY(d.feed("{\"a\":").isEmpty());auto x=d.feed("1}\n{\"b\":2}\n");QCOMPARE(x.size(),2);QCOMPARE(x[0]["a"].toInt(),1);QVERIFY(d.finish().isEmpty());}
 void jsonFinalLine(){JsonLines d;d.feed("{\"unicode\":\"caf\xc3\xa9\"}");auto x=d.finish();QCOMPARE(x.size(),1);QCOMPARE(x[0]["unicode"].toString(),QString::fromUtf8("caf\xc3\xa9"));}
 void jsonMalformed(){JsonLines d;QVERIFY_EXCEPTION_THROWN(d.feed("not-json\n"),std::runtime_error);}
 void jsonBounded(){JsonLines d;QVERIFY_EXCEPTION_THROWN(d.feed(QByteArray(1048577,'x')),std::runtime_error);}
};
QTEST_GUILESS_MAIN(RuntimeTests)
#include "RuntimeTests.moc"
