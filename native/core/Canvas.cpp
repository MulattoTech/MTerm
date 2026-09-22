// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "Canvas.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QUuid>
#include <cmath>
namespace mterm {
namespace {
bool bounded(const QJsonValue &v,double low,double high){return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=low && v.toDouble()<=high;}
bool text(const QJsonValue &v,qsizetype min,qsizetype max){return v.isString() && v.toString().size()>=min && v.toString().size()<=max;}
}
QString validateCanvas(const QJsonObject &c){
 if(QJsonDocument(c).toJson(QJsonDocument::Compact).size()>2000000) return "Canvas exceeds 2 MB";
 if(!c["nodes"].isArray() || !c["edges"].isArray() || !c["viewport"].isObject()) return "Missing canvas arrays/viewport";
 const auto nodes=c["nodes"].toArray(),edges=c["edges"].toArray();
 if(nodes.size()>100 || edges.size()>500) return "Canvas capacity exceeded";
 const QSet<QString> kinds{"note","task","agent","evidence","terminal","editor","diff","process","audit","file"};
 QSet<QString> ids,edgeIds;
 for(const auto &v:nodes){
  if(!v.isObject()) return "Node must be an object";
  const auto n=v.toObject(); const auto id=n["id"].toString();
  if(!text(n["id"],1,100) || ids.contains(id)) return "Invalid or duplicate node ID";
  ids.insert(id);
  if(!kinds.contains(n["kind"].toString()) || !text(n["title"],0,160) || !text(n["content"],0,100000) || !text(n["response"],0,100000)) return "Invalid node content";
  if(!bounded(n["x"],-1000000,1000000) || !bounded(n["y"],-1000000,1000000) || !bounded(n["width"],280,1400) || !bounded(n["height"],240,1400)) return "Invalid node geometry";
  if(!QSet<QString>{"draft","active","waiting","done"}.contains(n["status"].toString()) || !n["pinned"].isBool() || !n["collapsed"].isBool() || !n["items"].isArray() || n["items"].toArray().size()>100) return "Invalid node state";
  for(const auto &item:n["items"].toArray()){
   const auto o=item.toObject(); if(!text(o["id"],1,100) || !text(o["label"],0,300) || !o["done"].isBool()) return "Invalid node checklist";
  }
 }
 for(const auto &v:edges){
  const auto e=v.toObject();const auto id=e["id"].toString();
  if(!text(e["id"],1,100) || edgeIds.contains(id) || !ids.contains(e["source"].toString()) || !ids.contains(e["target"].toString())) return "Invalid or dangling canvas edge";
  edgeIds.insert(id);
 }
 const auto p=c["viewport"].toObject();
 if(!bounded(p["x"],-1000000,1000000) || !bounded(p["y"],-1000000,1000000) || !bounded(p["zoom"],0.15,3)) return "Invalid viewport";
 if(c["view"]!="canvas" && c["view"]!="project") return "Invalid canvas view";
 return {};
}
QJsonObject newNode(const QString &kind,const QString &title,const QString &content){
 return {{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"kind",kind},{"title",title},{"content",content},
 {"x",80},{"y",80},{"width",360},{"height",260},{"response",""},{"status","draft"},{"pinned",false},{"collapsed",false},{"items",QJsonArray{}}};
}
QJsonObject initialCanvas(){
 return {{"nodes",QJsonArray{newNode("note","MTerm","Native workspace. Add notes, tasks and tools from the toolbar.")}},{"edges",QJsonArray{}},
 {"viewport",QJsonObject{{"x",0},{"y",0},{"zoom",1}}},{"view","canvas"}};
}
}
