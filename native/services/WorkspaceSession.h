// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include "core/Store.h"
#include "core/Policy.h"
#include "core/FileService.h"
#include <memory>
namespace mterm {
/// Thread-confined application services. Never construct the Store on the GUI thread.
class WorkspaceSession {
public:
 explicit WorkspaceSession(QString databaseFile):databaseFile_(std::move(databaseFile)){}
 QJsonObject open(const QString &root);
 QJsonObject call(const QString &method,const QJsonObject &args);
 void authorize(const QString &capability,const QJsonObject &args);
 void requireScope(const QJsonObject &args) const;
 QString id()const{return workspaceId_;}
 QString root()const{return files_?files_->root():QString{};}
 Store &store(){return *store_;}
 FileService &files(){return *files_;}
private:
 QString databaseFile_,workspaceId_;
 std::unique_ptr<Store> store_;
 std::unique_ptr<FileService> files_;
 Policy policy_;
 QJsonObject snapshot() const;
 void saveCanvas(const QJsonObject &canvas,int expectedRevision);
};
}
