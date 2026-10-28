#pragma once
// The chat's input line (frontend-options.md §3: Enter opens the line, or sends it; the line is
// the controller's +0x118, and while it takes the keys the player's controls are skipped,
// player-control.md §4.9). What is sent goes out as packet 14 (netcode-packets.md §2.11): any
// length, nothing filtered, only a line that is not empty.
//
// The spec does not give the line's place or look: CubeVulkan draws it with the game's "edit"
// template (the field the character creation's name uses) at the bottom left. Escape closing the
// line without sending is CubeVulkan's.
#include "Hud.h"

#include <optional>
#include <string>

class ChatLine {
  public:
    // Whether the line is open and takes the keys ("typing", 0x1014f0).
    bool typing() const { return open; }
    // Enter: opens the line when closed; when open, closes it and gives the text to send (none
    // when it is empty).
    std::optional<std::u16string> enter();
    // Escape: closes the line, its text dropped.
    void cancel();
    // Characters typed while the line is open (the window's text input, printable ones).
    void type(const std::string &characters);
    void backspace();
    const std::u16string &text() const { return line; }

    // The line on screen for a window of `window` interface units (nothing while closed).
    static constexpr glm::vec2 FIELD_SIZE{420, 20};
    static constexpr float FIELD_LEFT = 20, FIELD_FROM_BOTTOM = 110;
    static constexpr size_t SHOWN_CHARACTERS = 30; // (the field's text wraps at 200 units)
    Hud::Screen screen(glm::vec2 window) const;

  private:
    bool open = false;
    std::u16string line;
};
