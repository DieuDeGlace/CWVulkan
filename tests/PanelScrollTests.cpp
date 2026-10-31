#include "PanelScroll.h"
#include <cstdio>
#include <limits>

int panelScrollTests() {
    using namespace panelscroll;
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            std::printf("FAIL panel scroll: %s\n", message);
            ++failures;
        }
    };
    const auto down = controls(8, -120, 600);
    const auto boundary = controls(9, -1, 600);
    check(down.first == 9 && down.refreshThumb && boundary.first == 9 && boundary.refreshThumb,
          "Controls moves one row and refreshes even at its native upper boundary");
    check(controls(0, 1, 600).first == 0 && controls(0, 1, 600).refreshThumb && !controls(4, 0, 600).refreshThumb,
          "Controls up clamps only below zero; a zero wheel calls neither handler");
    check(controls(0, -1, 1000).first == -5, "Controls down does not invent a lower clamp for a fully fitting list");
    check(controls(std::numeric_limits<int32_t>::max(), -1, 600).first == std::numeric_limits<int32_t>::min(),
          "Controls offset increment uses signed32 wrapping");
    const auto controlsBar = controlsThumb(9, 500, 600, 7);
    check(controlsBar && controlsBar->x == 472 && controlsBar->y == 216 && controlsBar->height == 234 && controlsBar->width == 7,
          "Controls thumb matches the independent H600 arithmetic reference");
    const auto friendsBar = friendsThumb(16, 420, 600, 8, 30);
    check(friendsBar && friendsBar->x == 392 && friendsBar->y == 281 && friendsBar->height == 169 && friendsBar->width == 8,
          "Multiplayer thumb matches the independent H600/N30 arithmetic reference");
    const auto fitting = friends(9, -1, 600, 13);
    check(fitting.first == 0 && !fitting.refreshThumb, "fitting Multiplayer down resets first row without refreshing its thumb");
    const auto last = friends(16, -1, 600, 30);
    check(last.first == 16 && last.refreshThumb && friends(0, 1, 600, 0).refreshThumb,
          "non-fitting down and every Multiplayer up request their native refresh");
    check(friends(4, -1, 0, 30).first == 0 && !friends(4, -1, 0, 30).refreshThumb,
          "negative visible-row quotient keeps the native unsigned list-fit comparison");
    const auto fullTrack = friendsThumb(5, 420, 600, 8, 14);
    check(fullTrack && fullTrack->y == 60 && fullTrack->height == 390,
          "Multiplayer's thumb-fit branch is separate from row-offset normalization");
    const auto stationary = controlsDrag(1, 600, 234, 55, 0);
    check(stationary && stationary->first == 0 && stationary->y == 55,
          "stationary held drag keeps continuous Y but recomputes the row by truncation");
    const auto dragLow = controlsDrag(5, 600, 234, 55, -1000);
    const auto dragHigh = controlsDrag(5, 600, 234, 55, 1000);
    check(dragLow && dragLow->first == 0 && dragLow->y == 35 && dragHigh && dragHigh->first == 9 && dragHigh->y == 216,
          "finite drag clamps to independently specified H600 endpoints");
    const auto noTravel = controlsDrag(9, 600, 415, 55, 0);
    check(noTravel && noTravel->first == 9 && noTravel->y == 35, "zero travel skips the row ratio and retains a valid stored row");
    const auto negativeTravel = controlsDrag(1, 1000, 900, 55, 0);
    check(negativeTravel && negativeTravel->first == -5 && negativeTravel->y == -50,
          "lower-then-upper drag clamps preserve the native negative-travel asymmetry");
    std::printf("Panel scroll: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
