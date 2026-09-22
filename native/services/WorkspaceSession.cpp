// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-resume-native (see docs/ai/changes/)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "WorkspaceSession.h"
#include "core/Canvas.h"
#include "runtime/ProcessInventory.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QUuid>
#include <stdexcept>
namespace mterm {
namespace {
[[noreturn]] void fail(const QString &s) {
    throw std::runtime_error(s.toStdString());
}
QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
QString uuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
} // namespace
void WorkspaceSession::requireScope(const QJsonObject &args) const {
    if (!store_ || workspaceId_.isEmpty() || args["workspaceId"].toString() != workspaceId_)
        fail("Workspace changed or is not open; reload before acting");
}
void WorkspaceSession::authorize(const QString &capability, const QJsonObject &args) {
    requireScope(args);
    if (!policy_.allows(capability, QDateTime::currentMSecsSinceEpoch())) {
        store_->audit(workspaceId_, capability, "DENY", "Permission required");
        fail("Permission denied: " + capability +
             ". Enable Developer and explicitly approve execution where required.");
    }
}
QJsonObject WorkspaceSession::open(const QString &root) {
    if (!store_)
        store_ = std::make_unique<Store>(databaseFile_);
    const auto selected =
        root.isEmpty() ? store_->setting("workspace-current")["root"].toString(QDir::currentPath())
                       : root;
    auto next = std::make_unique<FileService>(selected);
    files_ = std::move(next);
    workspaceId_ = QString::fromLatin1(
        QCryptographicHash::hash(files_->root().toUtf8(), QCryptographicHash::Sha256).toHex());
    policy_.setWorkspace(workspaceId_);
    QJsonObject workspace{{"id", workspaceId_},
                          {"root", files_->root()},
                          {"name", QFileInfo(files_->root()).fileName()}};
    auto catalog = store_->setting("workspace-catalog")["items"].toArray();
    for (qsizetype i = 0; i < catalog.size(); ++i)
        if (catalog[i].toObject()["id"] == workspaceId_) {
            catalog.removeAt(i);
            break;
        }
    catalog.prepend(workspace);
    while (catalog.size() > 100)
        catalog.removeLast();
    store_->transaction([&] {
        store_->setSetting("workspace-current", workspace);
        store_->setSetting("workspace-catalog", {{"items", catalog}});
        if (store_->setting("canvas:" + workspaceId_).isEmpty()) {
            store_->setSetting("canvas:" + workspaceId_, initialCanvas());
            store_->setSetting("canvas-revision:" + workspaceId_, {{"value", 0}});
        }
        // No native child survives this backend. Never restore a false RUNNING badge.
        for (const auto &v : store_->records("agent-session", workspaceId_, 200)) {
            auto session = v.toObject();
            if (QSet<QString>{"STARTING", "RUNNING", "WAITING", "STOPPING"}.contains(
                    session["status"].toString())) {
                session["status"] = "STOPPED";
                store_->putRecord("agent-session", workspaceId_, session);
            }
        }
        store_->audit(workspaceId_, "workspace.open", "ALLOW",
                      "Native workspace opened in Observe");
    });
    return snapshot();
}
QJsonObject WorkspaceSession::snapshot() const {
    auto canvas = store_->setting("canvas:" + workspaceId_);
    const auto error = validateCanvas(canvas);
    if (!error.isEmpty())
        fail("Stored canvas invalid: " + error);
    return {{"workspaceId", workspaceId_},
            {"root", files_->root()},
            {"profile", policy_.developer() ? "developer" : "observe"},
            {"canvas", canvas},
            {"revision", store_->setting("canvas-revision:" + workspaceId_)["value"]},
            {"tasks", store_->records("task", workspaceId_, 100)},
            {"sessions", store_->records("agent-session", workspaceId_, 100)},
            {"evidence", store_->records("evidence", workspaceId_, 100)},
            {"workspaces", store_->setting("workspace-catalog")["items"]}};
}
void WorkspaceSession::saveCanvas(const QJsonObject &canvas, int expectedRevision) {
    const auto error = validateCanvas(canvas);
    if (!error.isEmpty())
        fail(error);
    store_->transaction([&] {
        const int current = store_->setting("canvas-revision:" + workspaceId_)["value"].toInt();
        if (current != expectedRevision)
            fail("STALE_CANVAS: reload before saving");
        store_->setSetting("canvas:" + workspaceId_, canvas);
        store_->setSetting("canvas-revision:" + workspaceId_, {{"value", current + 1}});
        store_->audit(workspaceId_, "canvas.save", "ALLOW", "Canvas revision saved");
    });
}
QJsonObject WorkspaceSession::call(const QString &method, const QJsonObject &args) {
    requireScope(args);
    if (method == "state")
        return snapshot();
    if (method == "profile") {
        if (!args["developer"].isBool())
            fail("Invalid profile value");
        policy_.setDeveloper(args["developer"].toBool());
        store_->audit(workspaceId_, "permission.profile", "ALLOW",
                      policy_.developer() ? "Developer" : "Observe");
        return snapshot();
    }
    if (method == "grant") {
        const auto cap = args["capability"].toString();
        if (!policy_.grant(cap, QDateTime::currentMSecsSinceEpoch(), 8 * 60 * 60 * 1000))
            fail("Grant denied; enable Developer first");
        store_->audit(workspaceId_, "permission.grant", "ALLOW", cap);
        return {{"granted", true}};
    }
    if (method == "add-node") {
        authorize("canvas.write", args);
        for (const auto *key : {"kind", "title", "content"})
            if (args.contains(key) && !args[key].isString())
                fail("Node fields must be strings");
        auto canvas = store_->setting("canvas:" + workspaceId_);
        auto nodes = canvas["nodes"].toArray();
        auto node = newNode(args["kind"].toString("note"), args["title"].toString("Note"),
                            args["content"].toString());
        node["x"] = 80 + (nodes.size() % 3) * 400;
        node["y"] = 80 + (nodes.size() / 3) * 300;
        nodes.append(node);
        canvas["nodes"] = nodes;
        saveCanvas(canvas, store_->setting("canvas-revision:" + workspaceId_)["value"].toInt());
        return snapshot();
    }
    if (method == "save-canvas") {
        authorize("canvas.write", args);
        if (!args["canvas"].isObject() || !args["revision"].isDouble())
            fail("Invalid canvas update");
        saveCanvas(args["canvas"].toObject(), args["revision"].toInt(-1));
        return snapshot();
    }
    if (method == "create-task") {
        authorize("task.write", args);
        if (!args["title"].isString() || !args["objective"].isString())
            fail("Task title and objective must be strings");
        const auto title = args["title"].toString().trimmed(),
                   objective = args["objective"].toString();
        if (title.isEmpty() || title.size() > 200 || objective.size() > 20000)
            fail("Invalid task text");
        QJsonObject task{{"id", uuid()},
                         {"workspaceId", workspaceId_},
                         {"title", title},
                         {"objective", objective},
                         {"status", "READY"},
                         {"checklist", QJsonArray{}},
                         {"blockers", QJsonArray{}},
                         {"acceptanceCriteria", QJsonArray{}},
                         {"evidenceIds", QJsonArray{}},
                         {"createdAt", now()}};
        store_->transaction([&] {
            store_->putRecord("task", workspaceId_, task);
            store_->audit(workspaceId_, "task.create", "ALLOW", task["id"].toString());
        });
        return snapshot();
    }
    if (method == "task-status") {
        authorize("task.write", args);
        const auto state = args["status"].toString();
        if (!QSet<QString>{"READY", "RUNNING", "WAITING", "BLOCKED", "FAILED", "DONE"}.contains(
                state))
            fail("Invalid task state");
        for (const auto &v : store_->records("task", workspaceId_, 200))
            if (v.toObject()["id"] == args["id"]) {
                auto task = v.toObject();
                task["status"] = state;
                store_->putRecord("task", workspaceId_, task);
                store_->audit(workspaceId_, "task.status", "ALLOW", state);
                return snapshot();
            }
        fail("Task not found in this workspace");
    }
    if (method == "read-file" || method == "write-file") {
        const bool write = method == "write-file";
        authorize(write ? "filesystem.write" : "filesystem.read", args);
        if (!args["path"].isString() ||
            (write && (!args["text"].isString() || !args["version"].isString())))
            fail("File path, text and version must be strings");
        const auto relative = args["path"].toString();
        auto file =
            write ? files_->write(relative, args["text"].toString(), args["version"].toString())
                  : files_->read(relative);
        store_->audit(
            workspaceId_, write ? "filesystem.write" : "filesystem.read", "ALLOW",
            QString("%1 bytes; %2").arg(file.text.toUtf8().size()).arg(relative.left(400)));
        return {{"path", file.path}, {"text", file.text}, {"version", file.version}};
    }
    if (method == "list-files") {
        authorize("filesystem.read", args);
        const auto relative = args["path"].toString(".");
        const auto absolute = files_->resolve(relative);
        if (!QFileInfo(absolute).isDir())
            fail("Not a directory");
        QDirIterator it(absolute, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        QJsonArray items;
        while (it.hasNext() && items.size() < 500) {
            it.next();
            const auto info = it.fileInfo();
            items.append(QJsonObject{
                {"name", info.fileName()},
                {"directory", info.isDir()},
                {"path", QDir(files_->root()).relativeFilePath(info.absoluteFilePath())}});
        }
        return {{"items", items}, {"truncated", it.hasNext()}};
    }
    if (method == "processes") {
        authorize("process.read", args);
        return {{"items", processInventory()}};
    }
    if (method == "audit") {
        authorize("workspace.read", args);
        return {{"items", store_->audits(workspaceId_, 100)}};
    }
    fail("Unknown native service operation");
}
} // namespace mterm
