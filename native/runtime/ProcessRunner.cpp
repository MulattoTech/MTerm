// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "ProcessRunner.h"
#include <QDir>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
namespace mterm {
ProcessRunner::ProcessRunner(QObject *parent):QObject(parent){
 qRegisterMetaType<ProcessResult>();timer_.setSingleShot(true);
 connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{const auto b=process_.readAllStandardOutput();output_.append(b);emit outputReady(b,false);});
 connect(&process_,&QProcess::readyReadStandardError,this,[this]{const auto b=process_.readAllStandardError();errors_.append(b);emit outputReady(b,true);});
 connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
  error_=process_.errorString();if(e==QProcess::FailedToStart)finish(-1);
 });
 connect(&process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){finish(code);});
 connect(&process_,&QProcess::started,this,[this]{
#ifdef Q_OS_WIN
  HANDLE child=OpenProcess(PROCESS_SET_QUOTA|PROCESS_TERMINATE,FALSE,DWORD(process_.processId()));
  const bool owned=child && AssignProcessToJobObject(static_cast<HANDLE>(job_),child);
  if(child)CloseHandle(child);
  if(!owned){error_="Cannot establish owned process job";process_.kill();return;}
#endif
  if(cancelled_ || timedOut_)process_.kill();
 });
 connect(&timer_,&QTimer::timeout,this,[this]{timedOut_=true;closeJob();process_.kill();});
#ifdef Q_OS_WIN
 process_.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a){a->flags|=CREATE_NO_WINDOW;});
#endif
}
ProcessRunner::~ProcessRunner(){
 disconnect(&process_,nullptr,this,nullptr);timer_.stop();closeJob();
 if(process_.state()!=QProcess::NotRunning){process_.kill();process_.waitForFinished(1000);}
}
bool ProcessRunner::start(const Command &command){
 if(active_ || command.program.isEmpty() || command.program.contains(QChar::Null) ||
    !QDir(command.cwd).exists() || command.timeoutMs<1 || command.timeoutMs>3600000 || command.input.size()>1000000)return false;
 for(const auto &arg:command.arguments)if(arg.contains(QChar::Null))return false;
#ifdef Q_OS_WIN
 job_=CreateJobObjectW(nullptr,nullptr);
 if(!job_)return false;
 JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};info.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!SetInformationJobObject(static_cast<HANDLE>(job_),JobObjectExtendedLimitInformation,&info,sizeof(info))){closeJob();return false;}
#endif
 output_.clear();errors_.clear();cancelled_=false;timedOut_=false;error_.clear();active_=true;elapsed_.start();
 process_.setWorkingDirectory(command.cwd);process_.setProgram(command.program);process_.setArguments(command.arguments);
 process_.start(QIODevice::ReadWrite);
 if(!command.input.isEmpty())process_.write(command.input);
 process_.closeWriteChannel();timer_.start(command.timeoutMs);return true;
}
void ProcessRunner::cancel(){if(!active_)return;cancelled_=true;closeJob();process_.kill();}
void ProcessRunner::closeJob(){
#ifdef Q_OS_WIN
 if(job_){CloseHandle(static_cast<HANDLE>(job_));job_=nullptr;}
#endif
}
void ProcessRunner::finish(int code){
 if(!active_)return;timer_.stop();
 const auto out=process_.readAllStandardOutput(),err=process_.readAllStandardError();
 if(!out.isEmpty()){output_.append(out);emit outputReady(out,false);}
 if(!err.isEmpty()){errors_.append(err);emit outputReady(err,true);}
 closeJob();active_=false;
 emit completed({code,cancelled_,timedOut_,error_,output_.bytes(),errors_.bytes(),output_.droppedBytes()+errors_.droppedBytes(),elapsed_.elapsed()});
}
}
