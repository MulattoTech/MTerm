// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QObject>
#include <QHash>
#include <QJsonObject>
#include "WorkspaceSession.h"
#include "runtime/ProcessRunner.h"
namespace mterm {
/// Worker-thread job owner. Authorization and launch occur in the same event loop.
class JobService final: public QObject {
 Q_OBJECT
public:
 explicit JobService(WorkspaceSession &session,QObject *parent=nullptr):QObject(parent),session_(session){}
 ~JobService() override;
 QJsonObject run(const QJsonObject &args);
 QJsonObject cancel(const QJsonObject &args);
 void cancelAll();
 static QString findCodex();
signals:
 void stream(QString workspaceId,QString runId,QString text);
 void runFinished(QString workspaceId,QString runId,QJsonObject result);
private:
 WorkspaceSession &session_;
 QHash<QString,ProcessRunner*> jobs_;
};
}
