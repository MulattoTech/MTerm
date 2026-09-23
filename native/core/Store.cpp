// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "Store.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <stdexcept>
namespace mterm {
namespace {
[[noreturn]] void fail(const QString &s) {
    throw std::runtime_error(s.toStdString());
}
void check(QSqlQuery &q) {
    if (!q.exec())
        fail(q.lastError().text());
}
QString encode(const QJsonObject &o) {
    const auto b = QJsonDocument(o).toJson(QJsonDocument::Compact);
    if (b.size() > 2000000)
        fail("Record exceeds 2 MB");
    return QString::fromUtf8(b);
}
QJsonObject decode(const QString &s) {
    QJsonParseError e;
    const auto d = QJsonDocument::fromJson(s.toUtf8(), &e);
    if (e.error != QJsonParseError::NoError || !d.isObject())
        fail("Invalid stored JSON; recovery required");
    return d.object();
}
const QString scope =
    "CASE WHEN json_valid(payload) THEN json_extract(payload,'$.workspaceId') ELSE NULL END";
QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
} // namespace
Store::Store(const QString &file) : connection_(QUuid::createUuid().toString()) {
    if (!QDir().mkpath(QFileInfo(file).absolutePath()))
        fail("Cannot create data directory");
    db_ = QSqlDatabase::addDatabase("QSQLITE", connection_);
    db_.setDatabaseName(file);
    db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=500");
    try {
        if (!db_.open())
            fail(db_.lastError().text());
        if (schemaVersion() > SchemaVersion)
            fail("Database schema is newer than this MTerm build");
        exec("PRAGMA journal_mode=WAL");
        exec("PRAGMA foreign_keys=ON");
        migrate();
    } catch (...) {
        db_.close();
        db_ = QSqlDatabase{};
        QSqlDatabase::removeDatabase(connection_);
        throw;
    }
}
Store::~Store() {
    db_.close();
    db_ = QSqlDatabase{};
    QSqlDatabase::removeDatabase(connection_);
}
void Store::exec(const QString &sql) const {
    QSqlQuery q(db_);
    if (!q.exec(sql))
        fail(q.lastError().text());
}
int Store::schemaVersion() const {
    QSqlQuery q(db_);
    if (!q.exec("PRAGMA user_version") || !q.next())
        fail("Cannot read schema version");
    return q.value(0).toInt();
}
void Store::migrate() {
    if (schemaVersion() == SchemaVersion)
        return;
    transaction([&] {
        exec("CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY,value TEXT NOT NULL)");
        exec("CREATE TABLE IF NOT EXISTS audit(id TEXT PRIMARY KEY,timestamp TEXT NOT NULL,payload "
             "TEXT NOT NULL)");
        exec("CREATE TABLE IF NOT EXISTS records(id TEXT PRIMARY KEY,kind TEXT NOT NULL,updatedAt "
             "TEXT NOT NULL,payload TEXT NOT NULL)");
        exec("CREATE TABLE IF NOT EXISTS schema_migrations(version INTEGER PRIMARY KEY,name TEXT "
             "NOT NULL,appliedAt TEXT NOT NULL)");
        exec("CREATE INDEX IF NOT EXISTS records_kind_updated ON records(kind,updatedAt DESC)");
        exec("CREATE INDEX IF NOT EXISTS audit_timestamp ON audit(timestamp DESC)");
        exec("CREATE INDEX IF NOT EXISTS records_scope_updated ON records(kind,(" + scope +
             "),updatedAt DESC,id)");
        exec("CREATE INDEX IF NOT EXISTS audit_scope_updated ON audit((" + scope +
             "),timestamp DESC,id)");
        QSqlQuery q(db_);
        q.prepare("INSERT OR IGNORE INTO schema_migrations(version,name,appliedAt) VALUES(?,?,?)");
        for (int i = 1; i <= SchemaVersion; ++i) {
            q.bindValue(0, i);
            q.bindValue(1, QString("mterm-compatible-schema-%1").arg(i));
            q.bindValue(2, now());
            check(q);
        }
        exec("PRAGMA user_version=3");
    });
}
bool Store::integrity() const {
    QSqlQuery q(db_);
    return q.exec("PRAGMA quick_check") && q.next() && q.value(0).toString() == "ok";
}
QJsonObject Store::setting(const QString &key) const {
    QSqlQuery q(db_);
    q.prepare("SELECT value FROM settings WHERE key=?");
    q.addBindValue(key);
    check(q);
    return q.next() ? decode(q.value(0).toString()) : QJsonObject{};
}
void Store::setSetting(const QString &key, const QJsonObject &value) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO settings(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET "
              "value=excluded.value");
    q.addBindValue(key);
    q.addBindValue(encode(value));
    check(q);
    if (q.numRowsAffected() != 1)
        fail("Setting update failed");
}
void Store::putRecord(const QString &kind, const QString &workspace, QJsonObject value) {
    const auto id = value["id"].toString();
    if (id.isEmpty() || id.size() > 160 || workspace.isEmpty() || workspace.size() > 160 ||
        kind.isEmpty() || kind.size() > 80)
        fail("Invalid record identity");
    if (value.contains("workspaceId") && value["workspaceId"].toString() != workspace)
        fail("Record workspace mismatch");
    // Globally unique IDs cannot be reused to move another workspace's record.
    QSqlQuery existing(db_);
    existing.prepare("SELECT payload,kind FROM records WHERE id=?");
    existing.addBindValue(id);
    check(existing);
    if (existing.next() &&
        (decode(existing.value(0).toString())["workspaceId"].toString() != workspace ||
         existing.value(1).toString() != kind))
        fail("Record belongs to another workspace or kind");
    value["workspaceId"] = workspace;
    value["updatedAt"] = now();
    QSqlQuery q(db_);
    q.prepare("INSERT INTO records(id,kind,updatedAt,payload) VALUES(?,?,?,?) ON CONFLICT(id) DO "
              "UPDATE SET updatedAt=excluded.updatedAt,payload=excluded.payload WHERE "
              "records.kind=excluded.kind AND "
              "json_extract(records.payload,'$.workspaceId')=json_extract(excluded.payload,'$."
              "workspaceId')");
    q.addBindValue(id);
    q.addBindValue(kind);
    q.addBindValue(value["updatedAt"].toString());
    q.addBindValue(encode(value));
    check(q);
    if (q.numRowsAffected() != 1)
        fail("Record update rejected by identity guard");
}
QJsonArray Store::records(const QString &kind, const QString &workspace, int limit,
                          int offset) const {
    if (limit < 1 || limit > 200 || offset < 0 || workspace.isEmpty())
        fail("Invalid page or workspace");
    QSqlQuery q(db_);
    q.prepare("SELECT payload FROM records WHERE kind=? AND (" + scope +
              ")=? ORDER BY updatedAt DESC,id LIMIT ? OFFSET ?");
    q.addBindValue(kind);
    q.addBindValue(workspace);
    q.addBindValue(limit);
    q.addBindValue(offset);
    check(q);
    QJsonArray out;
    while (q.next())
        out.append(decode(q.value(0).toString()));
    return out;
}
void Store::audit(const QString &workspace, const QString &operation, const QString &decision,
                  const QString &result) {
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces), time = now();
    QJsonObject event{{"id", id},
                      {"timestamp", time},
                      {"workspaceId", workspace},
                      {"tool", operation.left(100)},
                      {"decision", decision.left(30)},
                      {"target", workspace},
                      {"result", result.left(1000)},
                      {"durationMs", 0}};
    QSqlQuery q(db_);
    q.prepare("INSERT INTO audit(id,timestamp,payload) VALUES(?,?,?)");
    q.addBindValue(id);
    q.addBindValue(time);
    q.addBindValue(encode(event));
    check(q);
}
QJsonArray Store::audits(const QString &workspace, int limit) const {
    if (limit < 1 || limit > 200 || workspace.isEmpty())
        fail("Invalid audit page");
    QSqlQuery q(db_);
    q.prepare("SELECT payload FROM audit WHERE (" + scope +
              ")=? ORDER BY timestamp DESC,id LIMIT ?");
    q.addBindValue(workspace);
    q.addBindValue(limit);
    check(q);
    QJsonArray out;
    while (q.next())
        out.append(decode(q.value(0).toString()));
    return out;
}
int Store::reconcileInterrupted(const QString &kind, const QString &workspace) {
    if ((kind != "agent-session" && kind != "terminal-resource") || workspace.isEmpty())
        fail("Invalid recovery scope");
    QSqlQuery q(db_);
    q.prepare("UPDATE records SET "
              "payload=json_set(payload,'$.status','STOPPED','$.updatedAt',?,'$.recoveryReason','"
              "Application session ended; process not reattached'),updatedAt=? WHERE kind=? AND (" +
              scope +
              ")=? AND json_extract(CASE WHEN json_valid(payload) THEN payload ELSE '{}' "
              "END,'$.status') IN ('STARTING','RUNNING','WAITING','STOPPING')");
    const auto time = now();
    q.addBindValue(time);
    q.addBindValue(time);
    q.addBindValue(kind);
    q.addBindValue(workspace);
    check(q);
    return int(q.numRowsAffected());
}
void Store::transaction(const std::function<void()> &operation) {
    if (!db_.transaction())
        fail(db_.lastError().text());
    try {
        operation();
        if (!db_.commit())
            fail(db_.lastError().text());
    } catch (...) {
        db_.rollback();
        throw;
    }
}
} // namespace mterm
