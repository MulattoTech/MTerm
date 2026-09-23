// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "ProcessInventory.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <algorithm>
#include <stdexcept>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Windows SDK extension headers require windows.h first.
// clang-format off
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
// clang-format on
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif
namespace mterm {
QJsonArray processInventory(int limit) {
    if (limit < 1 || limit > 4096)
        throw std::runtime_error("Process limit must be 1..4096");
    QList<QJsonObject> rows;
#ifdef Q_OS_WIN
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Cannot enumerate processes");
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry))
        do {
            QJsonObject row{{"pid", qint64(entry.th32ProcessID)},
                            {"parentPid", qint64(entry.th32ParentProcessID)},
                            {"name", QString::fromWCharArray(entry.szExeFile)},
                            {"workingSetBytes", QJsonValue::Null},
                            {"privateBytes", QJsonValue::Null},
                            {"cpuSeconds", QJsonValue::Null},
                            {"startedAt", QJsonValue::Null}};
            HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE,
                                        entry.th32ProcessID);
            if (!handle)
                handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
            if (handle) {
                PROCESS_MEMORY_COUNTERS_EX memory{};
                memory.cb = sizeof(memory);
                if (GetProcessMemoryInfo(handle,
                                         reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),
                                         sizeof(memory))) {
                    row["workingSetBytes"] = qint64(memory.WorkingSetSize);
                    row["privateBytes"] = qint64(memory.PrivateUsage);
                }
                FILETIME created{}, exited{}, kernel{}, user{};
                if (GetProcessTimes(handle, &created, &exited, &kernel, &user)) {
                    const auto ticks = [](FILETIME x) {
                        return (quint64(x.dwHighDateTime) << 32) | x.dwLowDateTime;
                    };
                    row["cpuSeconds"] = double(ticks(kernel) + ticks(user)) / 10000000.0;
                    row["startedAt"] =
                        QDateTime::fromMSecsSinceEpoch(
                            qint64(ticks(created) / 10000) - 11644473600000LL, Qt::UTC)
                            .toString(Qt::ISODate);
                }
                CloseHandle(handle);
            }
            rows.append(row);
        } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
#elif defined(Q_OS_LINUX)
    for (const auto &name : QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        const auto pid = name.toLongLong(&ok);
        if (!ok)
            continue;
        QFile file("/proc/" + name + "/comm");
        QString command;
        if (file.open(QIODevice::ReadOnly))
            command = QString::fromUtf8(file.read(4096)).trimmed();
        QJsonObject row{{"pid", pid},
                        {"name", command},
                        {"workingSetBytes", QJsonValue::Null},
                        {"privateBytes", QJsonValue::Null},
                        {"cpuSeconds", QJsonValue::Null}};
        QFile statm("/proc/" + name + "/statm");
        if (statm.open(QIODevice::ReadOnly)) {
            auto fields = statm.read(4096).simplified().split(' ');
            if (fields.size() > 1)
                row["workingSetBytes"] = fields[1].toLongLong() * ::sysconf(_SC_PAGESIZE);
        }
        rows.append(row);
    }
#else
    rows.append(QJsonObject{{"pid", QCoreApplication::applicationPid()},
                            {"name", QCoreApplication::applicationName()},
                            {"limitation", "Full inventory is not implemented on this platform"}});
#endif
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
        return a["pid"].toInteger() < b["pid"].toInteger();
    });
    QJsonArray result;
    for (qsizetype i = 0; i < std::min<qsizetype>(limit, rows.size()); ++i)
        result.append(rows[i]);
    return result;
}
QJsonObject currentProcessFootprint() {
    QJsonObject result{{"pid", QCoreApplication::applicationPid()}};
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),
                             sizeof(memory))) {
        result["workingSetBytes"] = qint64(memory.WorkingSetSize);
        result["privateBytes"] = qint64(memory.PrivateUsage);
    }
#else
    for (const auto &value : processInventory(4096)) {
        const auto item = value.toObject();
        if (item["pid"].toInteger() == QCoreApplication::applicationPid())
            return item;
    }
#endif
    return result;
}
} // namespace mterm
