// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QByteArray>
#include <QString>
namespace mterm {
struct FileSnapshot {
    QString path;
    QString text;
    QString version;
};
/// Bounded UTF-8 file service; methods throw std::runtime_error on failure.
/// All symlinks/reparse points and hard-linked files are conservatively refused.
/// Policy authorization is the caller's responsibility. Not a hostile-OS sandbox.
class FileService {
  public:
    explicit FileService(QString root);
    QString root() const {
        return root_;
    }
    QString resolve(const QString &relative, bool writing = false) const;
    FileSnapshot read(const QString &relative) const;
    FileSnapshot write(const QString &relative, const QString &text,
                       const QString &expectedVersion) const;
    static constexpr qint64 MaxBytes = 1000000;

  private:
    QString root_;
};
} // namespace mterm
