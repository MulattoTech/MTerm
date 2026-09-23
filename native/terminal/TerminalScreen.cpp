// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "TerminalScreen.h"
#include <stdexcept>
namespace mterm {
namespace {
void dimensions(int rows, int columns) {
    if (rows < 1 || rows > 200 || columns < 1 || columns > 400)
        throw std::runtime_error("Terminal dimensions exceed bounds");
}
quint32 rgb(VTermColor color, VTermScreen *screen) {
    vterm_screen_convert_color_to_rgb(screen, &color);
    return 0xff000000U | (quint32(color.rgb.red) << 16) | (quint32(color.rgb.green) << 8) |
           color.rgb.blue;
}
} // namespace
TerminalScreen::TerminalScreen(int rows, int columns) {
    dimensions(rows, columns);
    rows_ = rows;
    columns_ = columns;
    term_ = vterm_new(rows, columns);
    if (!term_)
        throw std::bad_alloc();
    vterm_set_utf8(term_, 1);
    vterm_output_set_callback(term_, &TerminalScreen::output, this);
    screen_ = vterm_obtain_screen(term_);
    vterm_screen_enable_altscreen(screen_, 1);
    VTermColor fg{}, bg{};
    vterm_color_rgb(&fg, 225, 235, 242);
    vterm_color_rgb(&bg, 12, 22, 31);
    vterm_state_set_default_colors(vterm_obtain_state(term_), &fg, &bg);
    vterm_screen_reset(screen_, 1);
}
TerminalScreen::~TerminalScreen() {
    if (term_)
        vterm_free(term_);
}
void TerminalScreen::resize(int rows, int columns) {
    dimensions(rows, columns);
    rows_ = rows;
    columns_ = columns;
    vterm_set_size(term_, rows, columns);
}
QByteArray TerminalScreen::feed(const QByteArray &data) {
    if (data.size() > 1048576)
        throw std::runtime_error("Terminal input batch exceeds 1 MiB");
    QByteArray complete = std::move(utf8Tail_);
    utf8Tail_.clear();
    complete.append(data);
    // libvterm has separate G0/high-bit decoders. Keep an incomplete UTF-8 scalar
    // together across pipe reads so switching decoders cannot replace its tail.
    if (!complete.isEmpty()) {
        qsizetype lead = complete.size() - 1;
        while (lead > 0 && (static_cast<unsigned char>(complete[lead]) & 0xc0) == 0x80 &&
               complete.size() - lead < 4)
            --lead;
        const auto byte = static_cast<unsigned char>(complete[lead]);
        const int required = byte >= 0xc2 && byte <= 0xdf   ? 2
                             : byte >= 0xe0 && byte <= 0xef ? 3
                             : byte >= 0xf0 && byte <= 0xf4 ? 4
                                                            : 1;
        if (required > complete.size() - lead) {
            utf8Tail_ = complete.mid(lead);
            complete.truncate(lead);
        }
    }
    vterm_input_write(term_, complete.constData(), size_t(complete.size()));
    vterm_screen_flush_damage(screen_);
    return takeReplies();
}
void TerminalScreen::output(const char *bytes, size_t length, void *user) {
    auto *self = static_cast<TerminalScreen *>(user);
    // Terminal response packets are bounded. Never grant OSC52 clipboard or host file access.
    if (length <= 65536 && self->replies_.size() + qsizetype(length) <= 65536)
        self->replies_.append(bytes, qsizetype(length));
}
QByteArray TerminalScreen::takeReplies() {
    auto out = std::move(replies_);
    replies_.clear();
    return out;
}
QByteArray TerminalScreen::key(VTermKey key, VTermModifier modifiers) {
    vterm_keyboard_key(term_, key, modifiers);
    return takeReplies();
}
QByteArray TerminalScreen::unicode(char32_t character, VTermModifier modifiers) {
    vterm_keyboard_unichar(term_, uint32_t(character), modifiers);
    return takeReplies();
}
TerminalCell TerminalScreen::cell(int row, int column) const {
    if (row < 0 || row >= rows_ || column < 0 || column >= columns_)
        throw std::runtime_error("Terminal cell outside bounds");
    VTermScreenCell value{};
    if (!vterm_screen_get_cell(screen_, {row, column}, &value))
        return {};
    char32_t characters[VTERM_MAX_CHARS_PER_CELL]{};
    int count = 0;
    while (count < VTERM_MAX_CHARS_PER_CELL && value.chars[count]) {
        characters[count] = char32_t(value.chars[count]);
        ++count;
    }
    auto fg = rgb(value.fg, screen_), bg = rgb(value.bg, screen_);
    if (value.attrs.reverse)
        std::swap(fg, bg);
    return {QString::fromUcs4(characters, count), fg,         bg, bool(value.attrs.bold),
            bool(value.attrs.underline),          value.width};
}
QString TerminalScreen::text() const {
    QByteArray bytes(rows_ * columns_ * 24, '\0');
    const auto size =
        vterm_screen_get_text(screen_, bytes.data(), size_t(bytes.size()), {0, rows_, 0, columns_});
    return QString::fromUtf8(bytes.constData(), qsizetype(size));
}
VTermPos TerminalScreen::cursor() const {
    VTermPos p{};
    vterm_state_get_cursorpos(vterm_obtain_state(term_), &p);
    return p;
}
} // namespace mterm
