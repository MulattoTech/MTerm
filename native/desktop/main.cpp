// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Modified: 2026-09-22-native-ux; internal startup phase diagnostics.
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "MainWindow.h"
#include "runtime/ProcessInventory.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QPalette>
#include <QStandardPaths>
#include <QTimer>
int main(int argc, char **argv) {
    QElapsedTimer elapsed;
    elapsed.start();
    QApplication app(argc, argv);
    app.setApplicationName("MTerm");
    app.setOrganizationName("MulattoTech");
    app.setApplicationVersion("0.2.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("MTerm native workspace preview");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"workspace", "Workspace root; defaults to last saved workspace", "path"});
    parser.addOption({"data-dir", "Separate native data directory", "path"});
    parser.addOption({"smoke-dir", "Save screenshot/metrics and exit; never run a model", "path"});
    parser.process(app);
    const auto dataDir =
        parser.isSet("data-dir")
            ? parser.value("data-dir")
            : QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/native";
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#111d27"));
    palette.setColor(QPalette::WindowText, QColor("#edf4f8"));
    palette.setColor(QPalette::Base, QColor("#0c161f"));
    palette.setColor(QPalette::AlternateBase, QColor("#192a37"));
    palette.setColor(QPalette::Text, QColor("#edf4f8"));
    palette.setColor(QPalette::Button, QColor("#223b4b"));
    palette.setColor(QPalette::ButtonText, QColor("#edf4f8"));
    palette.setColor(QPalette::Highlight, QColor("#31728c"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);
    mterm::MainWindow window(QDir(dataDir).filePath("mterm-native.sqlite"));
    qint64 firstPaint = -1, ready = -1;
    QObject::connect(&window, &mterm::MainWindow::firstPaint, &app, [&] {
        if (firstPaint < 0)
            firstPaint = elapsed.elapsed();
    });
    QObject::connect(&window, &mterm::MainWindow::workspaceReady, &app, [&] {
        if (ready < 0)
            ready = elapsed.elapsed();
        if (parser.isSet("smoke-dir"))
            QTimer::singleShot(600, &app, [&] {
                const auto dir = parser.value("smoke-dir");
                QDir().mkpath(dir);
                window.grab().save(QDir(dir).filePath("native-window.png"));
                QJsonObject record{
                    {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                    {"firstPaintMs", firstPaint},
                    {"uiPhasesMs",
                     QJsonObject{
                         {"theme", window.property("themeSetupMs").toLongLong()},
                         {"construction", window.property("uiConstructionMs").toLongLong()},
                         {"shell", window.property("shellConstructionMs").toLongLong()},
                         {"inspectors", window.property("inspectorsConstructionMs").toLongLong()},
                         {"canvas", window.property("canvasConstructionMs").toLongLong()}}},
                    {"widgetCount", window.findChildren<QWidget *>().size()},
                    {"workspaceReadyMs", ready},
                    {"pid", QCoreApplication::applicationPid()},
                    {"qtVersion", qVersion()},
                    {"processFootprint", mterm::currentProcessFootprint()},
                    {"version", app.applicationVersion()},
                    {"note", "Internal process-start timer; not comparable to CLI --version or "
                             "external cold-start timing"}};
                QFile file(QDir(dir).filePath("native-smoke.json"));
                if (!file.open(QIODevice::WriteOnly)) {
                    app.exit(2);
                    return;
                }
                file.write(QJsonDocument(record).toJson());
                file.close();
                app.quit();
            });
    });
    if (parser.isSet("smoke-dir"))
        QTimer::singleShot(15000, &app, [&] { app.exit(3); });
    window.show();
    QTimer::singleShot(0, &window, [&] { window.openWorkspace(parser.value("workspace")); });
    return app.exec();
}
