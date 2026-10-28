#include "GameWorld.h"
#include <sqlite3.h>
#include <cstring>

namespace gameworld {

bool parseDead(const std::string &state, std::set<int64_t> &dead, int *day) {
    if (state.size() < 16)
        return false;
    int32_t head[4];
    std::memcpy(head, state.data(), sizeof head);
    if (head[0] != 44 || head[3] < 0 || (size_t)head[3] > (state.size() - 16) / 8)
        return false;
    dead.clear();
    int64_t previous = INT64_MIN;
    for (int32_t i = 0; i < head[3]; i++) {
        int64_t id;
        std::memcpy(&id, state.data() + 16 + 8 * (size_t)i, 8);
        if (i > 0 && id <= previous)
            return false; // the game keeps them sorted; anything else is not this list
        dead.insert(id);
        previous = id;
    }
    if (day)
        *day = head[1];
    return true;
}

bool readDead(const std::filesystem::path &gameDir, std::set<int64_t> &dead, std::string &error, int *day) {
    std::filesystem::path file = gameDir / "Save" / "world.db";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(file, ec)) {
        error = "no Save/world.db";
        return false;
    }
    auto path = file.generic_u8string(); // SQLite takes UTF-8
    std::string uri = "file:" + std::string(path.begin(), path.end()) + "?mode=ro";
    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr) != SQLITE_OK) {
        error = db ? sqlite3_errmsg(db) : "cannot open Save/world.db";
        sqlite3_close(db);
        return false;
    }
    sqlite3_stmt *query = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, "SELECT value FROM blobs WHERE key = 'state'", -1, &query, nullptr) == SQLITE_OK &&
        sqlite3_step(query) == SQLITE_ROW) {
        const void *data = sqlite3_column_blob(query, 0);
        int size = sqlite3_column_bytes(query, 0);
        ok = data && parseDead(std::string((const char *)data, (size_t)size), dead, day);
        if (!ok)
            error = "Save/world.db: unrecognised world state";
    } else
        error = std::string("Save/world.db: ") + sqlite3_errmsg(db);
    sqlite3_finalize(query);
    sqlite3_close(db);
    return ok;
}
} // namespace gameworld
