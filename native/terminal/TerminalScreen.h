// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#pragma once
#include <QByteArray>
#include <QString>
#include <vterm.h>
namespace mterm {
struct TerminalCell {
    QString text;
    quint32 foreground = 0xffeeeeee, background = 0xff101010;
    bool bold = false, underline = false;
    int width = 1;
};
/// RAII wrapper around the pinned MIT libvterm parser. Thread-confined, bounded grid.
/// OSC clipboard and filesystem side effects are not implemented or exposed.
class TerminalScreen {
  public:
    TerminalScreen(int rows = 30, int columns = 100);
    ~TerminalScreen();
    TerminalScreen(const TerminalScreen &) = delete;
    TerminalScreen &operator=(const TerminalScreen &) = delete;
    void resize(int rows, int columns);
    QByteArray feed(const QByteArray &data);
    QByteArray key(VTermKey key, VTermModifier modifiers = VTERM_MOD_NONE);
    QByteArray unicode(char32_t character, VTermModifier modifiers = VTERM_MOD_NONE);
    TerminalCell cell(int row, int column) const;
    QString text() const;
    VTermPos cursor() const;
    int rows() const {
        return rows_;
    }
    int columns() const {
        return columns_;
    }

  private:
    VTerm *term_ = nullptr;
    VTermScreen *screen_ = nullptr;
    QByteArray replies_, utf8Tail_;
    int rows_ = 0, columns_ = 0;
    static void output(const char *bytes, size_t length, void *user);
    QByteArray takeReplies();
};
} // namespace mterm
