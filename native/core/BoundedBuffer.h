// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QByteArray>
#include <algorithm>
namespace mterm {
/// Byte-bounded tail capture. droppedBytes is cumulative; decode UTF-8 separately.
/// Not thread-safe: each stream has one owning event loop/worker.
class BoundedBuffer {
  public:
    explicit BoundedBuffer(qsizetype capacity) : capacity_(std::max<qsizetype>(0, capacity)) {}
    void append(const QByteArray &data) {
        if (data.size() >= capacity_) {
            dropped_ += bytes_.size() + data.size() - capacity_;
            bytes_ = capacity_ ? data.right(capacity_) : QByteArray{};
        } else {
            const auto excess = std::max<qsizetype>(0, bytes_.size() + data.size() - capacity_);
            if (excess) {
                bytes_.remove(0, excess);
                dropped_ += excess;
            }
            bytes_.append(data);
        }
    }
    QByteArray bytes() const {
        return bytes_;
    }
    qint64 droppedBytes() const {
        return dropped_;
    }
    void clear() {
        bytes_.clear();
        dropped_ = 0;
    }

  private:
    qsizetype capacity_;
    QByteArray bytes_;
    qint64 dropped_ = 0;
};
} // namespace mterm
