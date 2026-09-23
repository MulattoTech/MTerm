// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include <QCoreApplication>
#include <QThread>
#include <iostream>
#include <string>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.contains("exec")) {
        std::string prompt;
        std::getline(std::cin, prompt);
        std::cout << "{\"type\":\"thread.started\",\"thread_id\":\"mterm-fixture-thread\"}\n"
                  << std::flush;
        if (prompt == "MALFORMED_FINAL") {
            std::cout << "not-json" << std::flush;
            return 0;
        }
        if (prompt == "FAIL_FINAL") {
            std::cout << "{\"type\":\"turn.failed\"}" << std::flush;
            return 0;
        }
        if (prompt == "NO_COMPLETION") {
            return 0;
        }
        if (prompt == "WAIT") {
            QThread::msleep(5000);
            return 0;
        }
        std::cout << "{\"type\":\"item.completed\",\"item\":{\"type\":\"agent_message\",\"text\":\""
                  << (args.contains("resume") ? "MTERM_FIXTURE_RESUMED" : "MTERM_FIXTURE_OK")
                  << "\"}}\n";
        std::cout << "{\"type\":\"turn.completed\"}" << std::flush;
        return 0;
    }
    if (args.contains("--wait")) {
        QThread::msleep(5000);
        return 0;
    }
    if (args.contains("--flood")) {
        std::cout << std::string(600000, 'x') << std::flush;
        return 0;
    }
    if (args.contains("--stdin")) {
        std::string s;
        std::getline(std::cin, s);
        std::cout << s << std::flush;
        return 0;
    }
    if (args.contains("--fail")) {
        std::cerr << "deliberate" << std::flush;
        return 7;
    }
    std::cout << "MTERM_CHILD_OK\n" << std::flush;
    return 0;
}
