// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <functional>
namespace mterm {
/// Thread-confined SQLite connection. Construct/use/destroy on the same thread.
/// Migrations preserve legacy settings/audit/records. UI uses a worker thread.
class Store {
public:
 explicit Store(const QString &file);
 ~Store();
 Store(const Store &) = delete;
 Store &operator=(const Store &) = delete;
 int schemaVersion() const;
 bool integrity() const;
 QJsonObject setting(const QString &key) const;
 void setSetting(const QString &key,const QJsonObject &value);
 void putRecord(const QString &kind,const QString &workspace,QJsonObject value);
 QJsonArray records(const QString &kind,const QString &workspace,int limit=100,int offset=0) const;
 void audit(const QString &workspace,const QString &operation,const QString &decision,const QString &result);
 QJsonArray audits(const QString &workspace,int limit=100) const;
 void transaction(const std::function<void()> &operation);
 static constexpr int SchemaVersion = 3;
private:
 QString connection_;
 QSqlDatabase db_;
 void exec(const QString &sql) const;
 void migrate();
};
}
