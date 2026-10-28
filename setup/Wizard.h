#pragma once
// The first-run wizard window of CubeVulkanSetup: game, data, one-button extraction, play.
// The work itself is src/OwnerSetup.*; this is only its window.
#include <windows.h>
#include <filesystem>

namespace wizard {
// Opens the wizard over `owner` (or brings it forward). `root` is the folder of
// CubeVulkan.ini, CubeVulkan.exe and tools\ZoneExtractor.dll; `game` the game folder.
void open(HWND owner, const std::filesystem::path &root, const std::filesystem::path &game, HFONT font);
// For the message loop: true when the message was the wizard's dialog navigation.
bool dialogMessage(MSG &msg);
} // namespace wizard
