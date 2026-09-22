// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "JsonLines.h"
#include <QJsonDocument>
#include <stdexcept>
namespace mterm {
namespace {
QJsonObject parse(const QByteArray &line){
 QJsonParseError e;const auto doc=QJsonDocument::fromJson(line,&e);
 if(e.error!=QJsonParseError::NoError || !doc.isObject())throw std::runtime_error("Invalid JSONL provider event");
 return doc.object();
}
}
QList<QJsonObject> JsonLines::feed(const QByteArray &chunk){
 constexpr qsizetype maxLine=1048576;
 QList<QJsonObject> result;qsizetype pos=0;
 while(pos<chunk.size()){
  const auto end=chunk.indexOf('\n',pos);const auto count=end<0?chunk.size()-pos:end-pos;
  if(pending_.size()+count>maxLine){pending_.clear();throw std::runtime_error("JSONL event exceeds 1 MiB");}
  pending_.append(chunk.constData()+pos,count);
  if(end<0)break;
  if(!pending_.trimmed().isEmpty())result.append(parse(pending_));
  pending_.clear();pos=end+1;
  if(result.size()>4096)throw std::runtime_error("JSONL event burst exceeds limit");
 }
 return result;
}
QList<QJsonObject> JsonLines::finish(){auto last=std::move(pending_);pending_.clear();if(last.trimmed().isEmpty())return {};return {parse(last)};}
}
