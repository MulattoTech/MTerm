// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include "services/Backend.h"
using namespace mterm;
struct Reply{QJsonObject result;QString error;};
static Reply call(Backend &b,const QString &method,QJsonObject args={}){
 QSignalSpy spy(&b,&Backend::response);const auto id=b.request(method,args);
 if(!spy.wait(5000))return {{},"response timeout"};
 for(const auto &x:spy)if(x[0].toULongLong()==id)return {x[2].toJsonObject(),x[3].toString()};
 return {{},"wrong response"};
}
class BackendTests:public QObject{
 Q_OBJECT
private slots:
 void openObserve(){QTemporaryDir d;Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY2(s.error.isEmpty(),qPrintable(s.error));QCOMPARE(s.result["profile"].toString(),QString("observe"));QVERIFY(s.result["workspaceId"].isString());QVERIFY(s.result["canvas"].isObject());}
 void deniedWrites(){QTemporaryDir d;Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());auto id=s.result["workspaceId"];auto x=call(b,"add-node",{{"workspaceId",id},{"title","No"},{"content","denied"}});QVERIFY(!x.error.isEmpty());auto g=call(b,"grant",{{"workspaceId",id},{"capability","terminal.execute"}});QVERIFY(!g.error.isEmpty());}
 void persistentNodesAndTasks(){QTemporaryDir d;QString id;{Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());id=s.result["workspaceId"].toString();QVERIFY(call(b,"profile",{{"workspaceId",id},{"developer",true}}).error.isEmpty());auto n=call(b,"add-node",{{"workspaceId",id},{"title","Saved"},{"content","note"}});QVERIFY2(n.error.isEmpty(),qPrintable(n.error));QCOMPARE(n.result["canvas"].toObject()["nodes"].toArray().size(),2);auto t=call(b,"create-task",{{"workspaceId",id},{"title","Test"},{"objective","Native persistence"}});QVERIFY(t.error.isEmpty());QCOMPARE(t.result["tasks"].toArray().size(),1);} {Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());QCOMPARE(s.result["workspaceId"].toString(),id);QCOMPARE(s.result["canvas"].toObject()["nodes"].toArray().size(),2);QCOMPARE(s.result["tasks"].toArray().size(),1);QCOMPARE(s.result["profile"].toString(),QString("observe"));}}
 void staleCanvas(){QTemporaryDir d;Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());auto id=s.result["workspaceId"];call(b,"profile",{{"workspaceId",id},{"developer",true}});call(b,"add-node",{{"workspaceId",id},{"title","New"}});auto x=call(b,"save-canvas",{{"workspaceId",id},{"canvas",s.result["canvas"]},{"revision",s.result["revision"]}});QVERIFY(!x.error.isEmpty());}
 void wrongWorkspaceDenied(){QTemporaryDir d;Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());auto x=call(b,"profile",{{"workspaceId","other"},{"developer",true}});QVERIFY(!x.error.isEmpty());}
 void runApproval(){QTemporaryDir d;Backend b(d.filePath("data/db"));auto s=call(b,"open",{{"root",d.path()}});QVERIFY(s.error.isEmpty());auto id=s.result["workspaceId"];call(b,"profile",{{"workspaceId",id},{"developer",true}});
 QString program=QDir(QCoreApplication::applicationDirPath()).filePath("mterm-test-child"
#ifdef Q_OS_WIN
 ".exe"
#endif
 );
 QJsonObject args{{"workspaceId",id},{"kind","command"},{"program",program},{"arguments",QJsonArray{}}};QVERIFY(!call(b,"run",args).error.isEmpty());QVERIFY(call(b,"grant",{{"workspaceId",id},{"capability","terminal.execute"}}).error.isEmpty());QSignalSpy done(&b,&Backend::runFinished);auto run=call(b,"run",args);QVERIFY2(run.error.isEmpty(),qPrintable(run.error));QVERIFY(run.result["runId"].isString());if(done.isEmpty())QVERIFY(done.wait(5000));QCOMPARE(done[0][2].toJsonObject()["exitCode"].toInt(),0);}
};
QTEST_GUILESS_MAIN(BackendTests)
#include "BackendTests.moc"
