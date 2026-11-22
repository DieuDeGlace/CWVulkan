#pragma once
// The extraction panel of CubeVulkanSetup's main window: one line saying whether the game data
// is ready, one "Extract game data" button that runs the whole safe extraction (src/OwnerSetup.*:
// the extractor goes into the game folder only for the run, and everything is put back after),
// and "Stop and put back" while a run is open. The work itself is OwnerSetup; this is its panel.
#include <windows.h>
#include <filesystem>

namespace wizard {
// Creates the panel's controls inside `parent` at (x, y), `width` wide and `height` tall.
// `root` is the folder of CubeVulkan.ini, CubeVulkan.exe and tools\ZoneExtractor.dll.
void embed(HWND parent, const std::filesystem::path &root, const std::filesystem::path &game, HFONT font, int x, int y,
           int width, int height);
// Looks again at a (new) game folder: an interrupted run first, then the game, Steam and the data.
void setGame(const std::filesystem::path &game);
// The parent's WM_COMMAND and WM_TIMER: true when the message was the panel's.
bool command(WPARAM wp);
bool timer(WPARAM wp);
// Whether everything CubeVulkan needs is present, so Play can start it.
bool readyToPlay();
// Starts an extraction run (the same as the button), after asking the owner.
void extract();
// Whether the window may close now (a run must be put back first).
bool mayClose();
} // namespace wizard
