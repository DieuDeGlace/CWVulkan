#pragma once
// The owner's Steam Cube World world state (Save/world.db), read only: never written.
//
// world.db is a SQLite file with one table, blobs(key, value); the "state" blob
// starts with the creatures that have died in this world (decoded 2026-10-22,
// docs/creatures.md "Death is permanent"):
//   int32 version (44), int32 day (the world's day, as at world + 8 in the game;
//   it seeds townspeople's conversations), int32 (unknown), int32 count,
//   then `count` int64 creature ids, sorted (signed).
// The game adds every creature that dies and does not bring them back.
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

namespace gameworld {
// Decodes the dead-creature list from a "state" blob; false if it is not one.
bool parseDead(const std::string &state, std::set<int64_t> &dead, int *day = nullptr);
// Reads <gameDir>/Save/world.db; false (with a reason) if it is missing or unreadable.
bool readDead(const std::filesystem::path &gameDir, std::set<int64_t> &dead, std::string &error, int *day = nullptr);
} // namespace gameworld
