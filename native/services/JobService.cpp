// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "JobService.h"
#include "runtime/JsonLines.h"
#include <QStandardPaths>
#include <QDirIterator>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QTimer>
#include <QStringDecoder>
#include <QUuid>
#include <memory>
#include <stdexcept>
namespace mterm {
namespace {
[[noreturn]] void fail(const QString &s){throw std::runtime_error(s.toStdString());}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
QString now(){return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);}
struct Capture {BoundedBuffer pending{65536};JsonLines json;QJsonObject session;QString parseError;};
}
JobService::~JobService(){const auto active=jobs_;jobs_.clear();for(auto *job:active)delete job;}
void JobService::cancelAll(){const auto active=jobs_;for(auto *job:active)job->cancel();}
QString JobService::findCodex(){
 auto configured=qEnvironmentVariable("MTERM_CODEX_EXECUTABLE");if(!configured.isEmpty() && QFileInfo(configured).isExecutable())return configured;
#ifdef Q_OS_WIN
 const auto base=qEnvironmentVariable("APPDATA")+"/npm/node_modules/@openai";
 QDirIterator it(base,{"codex.exe"},QDir::Files,QDirIterator::Subdirectories);
 if(it.hasNext())return it.next();
 return QStandardPaths::findExecutable("codex.exe");
#else
 return QStandardPaths::findExecutable("codex");
#endif
}
QJsonObject JobService::run(const QJsonObject &args){
 const auto kind=args["kind"].toString();
 const auto cap=kind=="git"?"git.read":kind=="codex"?"agent.execute":"terminal.execute";
 session_.authorize(cap,args);if(jobs_.size()>=4)fail("At most four native jobs may run concurrently");
 const auto workspace=session_.id(),root=session_.root(),runId=uuid();
 Command command;command.cwd=root;command.timeoutMs=kind=="codex"?600000:60000;
 auto capture=std::make_shared<Capture>();
 if(kind=="git"){
  command.program=QStandardPaths::findExecutable("git");
  command.arguments={"--no-pager","--no-optional-locks","-c","core.fsmonitor=false","--literal-pathspecs"};
  const auto action=args["action"].toString("status");
  if(action=="status")command.arguments<<"status"<<"--short"<<"--branch";
  else if(action=="diff")command.arguments<<"diff"<<"--no-ext-diff"<<"--no-textconv"<<"--";
  else if(action=="log")command.arguments<<"log"<<"-20"<<"--oneline";
  else if(action=="worktrees")command.arguments<<"worktree"<<"list"<<"--porcelain";
  else fail("Unsupported native Git action");
 }else if(kind=="codex"){
  command.program=findCodex();const auto prompt=args["prompt"].toString();
  if(prompt.trimmed().isEmpty() || prompt.size()>50000)fail("Prompt must contain 1..50000 characters");
  command.arguments={"exec","--json","--sandbox","read-only","--skip-git-repo-check","-C",root,"--color","never"};
  const auto resume=args["resumeId"].toString();
  if(!resume.isEmpty()){
   for(const auto &v:session_.store().records("agent-session",workspace,200))if(v.toObject()["id"].toString()==resume)capture->session=v.toObject();
   const auto thread=capture->session["resumabilityData"].toObject()["threadId"].toString();
   if(thread.isEmpty() || capture->session["providerId"]!="codex-cli" || capture->session["status"]=="RUNNING")fail("Session cannot resume");
   command.arguments<<"resume"<<thread;
  }else capture->session={{"id",runId},{"workspaceId",workspace},{"providerId","codex-cli"},{"createdAt",now()},{"contextRefs",QJsonArray{}}};
  command.arguments<<"-";command.input=prompt.toUtf8()+"\n";
  capture->session["status"]="RUNNING";
 }else if(kind=="command"){
  command.program=args["program"].toString();if(!args["arguments"].isArray())fail("Arguments must be an array");
  for(const auto &v:args["arguments"].toArray()){if(!v.isString() || v.toString().size()>4096)fail("Invalid argument");command.arguments<<v.toString();}
  if(command.arguments.size()>64)fail("Too many command arguments");
 }else fail("Unknown native job kind");
 if(command.program.isEmpty())fail("Executable not found; configure/install the selected tool");
 auto *runner=new ProcessRunner(this);jobs_[runId]=runner;
 auto *flush=new QTimer(runner);flush->setInterval(33);
 const auto drain=[this,workspace,runId,capture]{
  const auto bytes=capture->pending.bytes();if(bytes.isEmpty())return;
  const auto dropped=capture->pending.droppedBytes();capture->pending.clear();
  emit stream(workspace,runId,(dropped?QString("[live output coalesced: %1 bytes omitted]\n").arg(dropped):QString{})+QString::fromUtf8(bytes));
 };
 connect(flush,&QTimer::timeout,this,drain);
 connect(runner,&ProcessRunner::outputReady,this,[this,capture,runner,kind,workspace](const QByteArray &data,bool err){
  if(kind!="codex" || err){capture->pending.append(data);return;}
  try{for(const auto &event:capture->json.feed(data)){
   if(event["type"]=="thread.started"){
    capture->session["resumabilityData"]=QJsonObject{{"threadId",event["thread_id"]}};session_.store().putRecord("agent-session",workspace,capture->session);
   }
   const auto item=event["item"].toObject();if(item["type"]=="agent_message" && item["text"].isString())capture->pending.append(item["text"].toString().toUtf8()+"\n");
   if(event["type"]=="turn.failed" || event["type"]=="error")capture->parseError="Provider reported an error";
  }}catch(const std::exception &e){capture->parseError=QString::fromUtf8(e.what());runner->cancel();}
 });
 connect(runner,&ProcessRunner::completed,this,[this,workspace,runId,capture,runner,drain,kind](const ProcessResult &result){
  drain();jobs_.remove(runId);
  const bool ok=result.exitCode==0 && result.error.isEmpty() && !result.cancelled && !result.timedOut && capture->parseError.isEmpty();
  QJsonObject summary{{"exitCode",result.exitCode},{"cancelled",result.cancelled},{"timedOut",result.timedOut},
   {"durationMs",result.durationMs},{"droppedBytes",result.droppedBytes},{"error",capture->parseError.isEmpty()?result.error:capture->parseError}};
  try{session_.store().transaction([&]{
   session_.store().audit(workspace,"job."+kind,ok?"ALLOW":"FAILED",QString("exit=%1; durationMs=%2; droppedBytes=%3").arg(result.exitCode).arg(result.durationMs).arg(result.droppedBytes));
   if(kind=="codex"){
    capture->session["status"]=result.cancelled?"STOPPED":ok?"DONE":"FAILED";
    session_.store().putRecord("agent-session",workspace,capture->session);
    session_.store().putRecord("evidence",workspace,{{"id",uuid()},{"kind","command"},{"label","Native Codex process outcome (not task acceptance)"},{"status",ok?"PASS":"FAIL"},{"summary",QString("exit=%1").arg(result.exitCode)},{"reference",capture->session["id"]},{"createdAt",now()}});
   }
  });}catch(const std::exception &e){summary["error"]=QString("Outcome persistence failed: ")+e.what();}
  emit runFinished(workspace,runId,summary);runner->deleteLater();
 });
 if(!capture->session.isEmpty())session_.store().putRecord("agent-session",workspace,capture->session);
 session_.store().audit(workspace,"job."+kind,"ALLOW","Explicit native job started; prompt/output not persisted");
 if(!runner->start(command)){jobs_.remove(runId);delete runner;fail("Native process start rejected");}
 flush->start();return {{"runId",runId},{"workspaceId",workspace},{"kind",kind}};
}
QJsonObject JobService::cancel(const QJsonObject &args){session_.requireScope(args);auto *job=jobs_.value(args["runId"].toString(),nullptr);if(!job)fail("Owned active job not found");job->cancel();return {{"cancelRequested",true}};}
}
