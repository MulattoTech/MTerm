// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QObject>
#include <QThread>
#include <QJsonObject>
namespace mterm {
class Worker;
/// GUI-thread facade. All database, filesystem and process work runs on its worker.
/// Every mutating request carries the workspaceId from the last open response.
class Backend final:public QObject {
 Q_OBJECT
public:
 explicit Backend(QString databaseFile,QObject *parent=nullptr);
 ~Backend() override;
 quint64 request(const QString &method,const QJsonObject &arguments={});
signals:
 void response(quint64 id,QString method,QJsonObject result,QString error);
 void stream(QString workspaceId,QString runId,QString text);
 void runFinished(QString workspaceId,QString runId,QJsonObject result);
private:
 QThread workerThread_;Worker *worker_=nullptr;quint64 nextId_=0;
};
}
