#pragma once
// ChatWidget history/editor, independently specified in steam-chat-feat-presentation.md.
// Font measurements come from the owner's font under the live widget matrix.
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class ChatLine {
  public:
    using Colour = std::array<uint8_t, 3>;
    using Measure = std::function<float(const std::wstring &)>;
    using NameResolver = std::function<std::optional<std::u16string>(int64_t)>;
    struct Word {
        std::wstring text;
        Colour colour;
    };
    using Row = std::vector<Word>;
    enum class Key { Left, Right, Home, End, Delete, Backspace };
    bool typing() const { return open; }
    std::optional<std::u16string> enter();
    // Mouse-down retains text and selection; Escape belongs to the outer menu.
    void stopEditing() { open = false; }
    void type(const std::string &lowBytes);
    void key(Key key, bool shift = false);
    void backspace() { key(Key::Backspace); }
    const std::u16string &text() const { return line; }
    int caretIndex() const { return caret; }
    int selectedUnits() const { return selection; }
    static bool caretVisible(int32_t guiClock) { return ((guiClock / 500) & 1) != 0; }
    void append(const std::u16string &, Colour, float frameWidth, const Measure &);
    void trim(float frameHeight);
    const std::deque<Row> &rows() const { return history; }
    void receive(std::optional<std::u16string> sender, std::u16string body);
    void receive(int64_t sender, std::u16string body);
    void consumeIncoming(float frameWidth, const Measure &, const NameResolver & = {});
    size_t pending() const { return incoming.size(); }
    static constexpr float WIDTH = 400, HEIGHT = 250;

  private:
    void clampSelection();
    void eraseSelection();
    bool open = false;
    std::u16string line;
    int caret = 0, selection = 0;
    std::deque<Row> history;
    struct Received {
        std::optional<std::u16string> sender;
        std::u16string body;
        int64_t id = -1;
        bool resolve = false;
    };
    std::deque<Received> incoming;
};
