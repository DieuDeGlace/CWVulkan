// The game's own save files, written from docs/specs/native-saves.md.
#include "NativeSaves.h"
#include "NativeInteract.h"

#include <sqlite3.h>
#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <system_error>

namespace fs = std::filesystem;

namespace nativesaves {

// --- Items and looks -------------------------------------------------------------------------

Item makeItem(int type, int subtype, int material) {
    Item it{};
    it[item::TYPE] = (uint8_t)type;
    const int32_t sub = subtype;
    std::memcpy(&it[item::SUBTYPE], &sub, 4);
    it[item::MATERIAL] = (uint8_t)material;
    return it;
}

namespace {
template <class T> T readAt(const uint8_t *bytes, size_t at) {
    T v;
    std::memcpy(&v, bytes + at, sizeof v);
    return v;
}
template <class T> void writeAt(uint8_t *bytes, size_t at, T v) { std::memcpy(bytes + at, &v, sizeof v); }

// The looks bytes (§3.5).
constexpr size_t LOOKS_PEOPLE = 0x00, LOOKS_SEX = 0x04, LOOKS_FACE = 0x08, LOOKS_HAIRCUT = 0x0c, LOOKS_COLOUR = 0x10;
} // namespace

int32_t Looks::people() const { return readAt<int32_t>(bytes.data(), LOOKS_PEOPLE); }
int Looks::sex() const { return bytes[LOOKS_SEX] & 1; }
int32_t Looks::face() const { return readAt<int32_t>(bytes.data(), LOOKS_FACE); }
int32_t Looks::haircut() const { return readAt<int32_t>(bytes.data(), LOOKS_HAIRCUT); }
std::array<uint8_t, 3> Looks::hairColour() const {
    return {bytes[LOOKS_COLOUR], bytes[LOOKS_COLOUR + 1], bytes[LOOKS_COLOUR + 2]};
}
void Looks::set(int32_t people, int sex, int32_t face, int32_t haircut, std::array<uint8_t, 3> colour) {
    writeAt(bytes.data(), LOOKS_PEOPLE, people);
    bytes[LOOKS_SEX] = (uint8_t)((bytes[LOOKS_SEX] & ~1) | (sex & 1));
    writeAt(bytes.data(), LOOKS_FACE, face);
    writeAt(bytes.data(), LOOKS_HAIRCUT, haircut);
    for (int i = 0; i < 3; i++)
        bytes[LOOKS_COLOUR + i] = colour[(size_t)i];
}

int32_t raceOf(int32_t people, int sex) {
    // human, elf, dwarf, orc, goblin, lizard, undead, frogman: the male race; the female is next.
    static constexpr int32_t MALE_RACE[8] = {2, 0, 9, 11, 4, 7, 15, 13};
    if (people < 0 || people > 7)
        return -1;
    return MALE_RACE[people] + (sex & 1);
}

Cell starGroup(int32_t x, int32_t y) { return {x / 4096, y / 4096}; } // C division: toward zero

std::array<int32_t, 3> BlockChange::block() const {
    return {readAt<int32_t>(bytes.data(), 0), readAt<int32_t>(bytes.data(), 4), readAt<int32_t>(bytes.data(), 8)};
}
std::array<uint8_t, 6> BlockChange::was() const {
    std::array<uint8_t, 6> b;
    std::memcpy(b.data(), bytes.data() + 0x0c, 6);
    return b;
}
std::array<uint8_t, 6> BlockChange::now() const {
    std::array<uint8_t, 6> b;
    std::memcpy(b.data(), bytes.data() + 0x12, 6);
    return b;
}

// --- Reading and writing fields --------------------------------------------------------------

namespace {

// The reading cursor of 0x147750: a field that does not fit sends the cursor to the end and is
// left as it was, so every later field fails too.
class Reader {
  public:
    explicit Reader(const std::vector<uint8_t> &data) : data(data) {}
    bool bytes(void *out, size_t n) {
        if (data.size() - at < n) {
            at = data.size();
            return false;
        }
        std::memcpy(out, data.data() + at, n);
        at += n;
        return true;
    }
    template <class T> bool value(T &out) {
        T v;
        if (!bytes(&v, sizeof v))
            return false;
        out = v;
        return true;
    }
    bool cell(Cell &out) {
        Cell c;
        if (!value(c[0]) || !value(c[1]))
            return false;
        out = c;
        return true;
    }
    bool text(std::string &out) {
        int32_t n;
        if (!value(n))
            return false;
        if (n < 0 || data.size() - at < (size_t)n) {
            at = data.size();
            return false;
        }
        out.assign(reinterpret_cast<const char *>(data.data() + at), (size_t)n);
        at += (size_t)n;
        return true;
    }
    // A count of entries; false (and no entries) when it does not fit or is negative. A port
    // stops at the end of the data rather than going on with a stale count (§3.1).
    bool count(int32_t &n) {
        n = 0;
        if (!value(n) || n < 0) {
            n = 0;
            return false;
        }
        return true;
    }
    void skip(size_t n) {
        at = data.size() - at < n ? data.size() : at + n;
    }

  private:
    const std::vector<uint8_t> &data;
    size_t at = 0;
};

class Writer {
  public:
    std::vector<uint8_t> out;
    void bytes(const void *p, size_t n) {
        const uint8_t *b = static_cast<const uint8_t *>(p);
        out.insert(out.end(), b, b + n);
    }
    template <class T> void value(T v) { bytes(&v, sizeof v); }
    void cell(const Cell &c) { value(c[0]), value(c[1]); }
    void text(const std::string &s) {
        value((int32_t)s.size());
        bytes(s.data(), s.size());
    }
    void count(size_t n) { value((int32_t)n); }
};

// A set of pairs: a count, then the pairs (sorted when written; a duplicate keeps the first).
void readSet(Reader &r, std::set<Cell> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        Cell c;
        if (!r.cell(c))
            return;
        out.insert(c);
    }
}
void writeSet(Writer &w, const std::set<Cell> &s) {
    w.count(s.size());
    for (const Cell &c : s)
        w.cell(c);
}
template <class V> void readMap(Reader &r, std::map<Cell, V> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        Cell c;
        V v;
        if (!r.cell(c) || !r.value(v))
            return;
        out.emplace(c, v);
    }
}
template <class V> void writeMap(Writer &w, const std::map<Cell, V> &m) {
    w.count(m.size());
    for (const auto &[c, v] : m) {
        w.cell(c);
        w.value(v);
    }
}
void readCells(Reader &r, std::vector<Cell> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        Cell c;
        if (!r.cell(c))
            return;
        out.push_back(c);
    }
}
void writeCells(Writer &w, const std::vector<Cell> &cells) {
    w.count(cells.size());
    for (const Cell &c : cells)
        w.cell(c);
}
void readItems(Reader &r, std::vector<Item> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        Item it;
        if (!r.bytes(it.data(), it.size()))
            return;
        out.push_back(it);
    }
}
void writeItems(Writer &w, const std::vector<Item> &items) {
    w.count(items.size());
    for (const Item &it : items)
        w.bytes(it.data(), it.size());
}
template <class T> void readIds(Reader &r, std::set<T> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        T v;
        if (!r.value(v))
            return;
        out.insert(v);
    }
}
template <class T> void writeIds(Writer &w, const std::set<T> &ids) {
    w.count(ids.size());
    for (T v : ids)
        w.value(v);
}
template <size_t N> void readRecords(Reader &r, std::vector<std::array<uint8_t, N>> &out) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        std::array<uint8_t, N> rec;
        if (!r.bytes(rec.data(), N))
            return;
        out.push_back(rec);
    }
}
template <size_t N> void writeRecords(Writer &w, const std::vector<std::array<uint8_t, N>> &records) {
    w.count(records.size());
    for (const auto &rec : records)
        w.bytes(rec.data(), N);
}

// Gold above this, compared as an unsigned 64-bit number, is replaced (§3.6).
constexpr uint64_t GOLD_LIMIT = 10000000000ull;
constexpr int32_t GOLD_REPLACEMENT = 1410065408; // the low 32 bits of 10^10

// The legacy list of version < 4 (§3.4): read and discarded.
void skipLegacyList(Reader &r) {
    int32_t n;
    if (!r.count(n))
        return;
    for (int32_t i = 0; i < n; i++) {
        std::string discarded;
        r.skip(4);
        if (!r.text(discarded))
            return;
        r.skip(24);
    }
}

} // namespace

void readCharacter(const std::vector<uint8_t> &blob, Character &c) {
    Reader r(blob);
    r.value(c.version);
    for (int64_t &p : c.position)
        r.value(p);
    for (float &o : c.orientation)
        r.value(o);
    r.value(c.health);
    r.value(c.level);
    r.value(c.experience);
    r.value(c.classType);
    r.value(c.specialization);
    r.value(c.field9a4);
    r.value(c.field9a8);
    for (Item &slot : c.equipment)
        r.bytes(slot.data(), slot.size());
    r.text(c.name);
    r.bytes(c.looks.bytes.data(), c.looks.bytes.size());

    // The inventory: its tabs, each a list of stacks (a count and the item).
    int32_t tabs;
    if (r.count(tabs)) {
        c.inventory.resize((size_t)tabs);
        for (int32_t t = 0; t < tabs; t++) {
            int32_t n;
            if (!r.count(n))
                break;
            bool complete = true;
            for (int32_t i = 0; i < n && complete; i++) {
                Stack s;
                complete = r.value(s.count) && r.bytes(s.item.data(), s.item.size());
                if (complete)
                    c.inventory[(size_t)t].push_back(s);
            }
            if (!complete)
                break;
        }
    }
    r.value(c.gold);
    r.value(c.fieldAb0);
    readItems(r, c.recipes);

    if (c.version > 9) { // the places, missions and their timers
        readSet(r, c.discoveredZones);
        readSet(r, c.set48);
        readSet(r, c.discoveredLands);
        readMap(r, c.map88);
        readMap(r, c.map98);
        readSet(r, c.usedCells);
        readSet(r, c.setB8);
        readSet(r, c.setC8);
        readSet(r, c.finishedMissions);
        readMap(r, c.missionTimers);
    }
    if (c.version < 4)
        skipLegacyList(r);

    r.value(c.worldSeed);
    r.text(c.worldName);
    readCells(r, c.flightPoints);
    r.cell(c.homeCell);
    { // buffs, copied whole
        int32_t n;
        if (r.count(n))
            for (int32_t i = 0; i < n; i++) {
                Buff b;
                if (!r.bytes(b.bytes.data(), b.bytes.size()))
                    break;
                c.buffs.push_back(b);
            }
    }
    readMap(r, c.regionValues);
    { // the stars: one flat list, regrouped by the cell of 4096 blocks they fall in
        int32_t n;
        if (r.count(n))
            for (int32_t i = 0; i < n; i++) {
                Star s;
                if (!r.value(s.unknown) || !r.value(s.x) || !r.value(s.y) || !r.value(s.z))
                    break;
                c.stars[starGroup(s.x, s.y)].push_back(s);
            }
    }
    readCells(r, c.regions118);
    { // what was bought, by shop
        int32_t shops;
        if (r.count(shops))
            for (int32_t i = 0; i < shops; i++) {
                int64_t shop;
                if (!r.value(shop))
                    break;
                std::vector<Item> bought;
                readItems(r, bought);
                c.purchases.emplace(shop, std::move(bought));
            }
    }

    if ((uint64_t)(int64_t)c.gold > GOLD_LIMIT) // only a negative amount can pass
        c.gold = GOLD_REPLACEMENT;
}

std::vector<uint8_t> writeCharacter(const Character &c) {
    Writer w;
    w.value(CHARACTER_VERSION);
    for (int64_t p : c.position)
        w.value(p);
    for (float o : c.orientation)
        w.value(o);
    w.value(c.health);
    w.value(c.level);
    w.value(c.experience);
    w.value(c.classType);
    w.value(c.specialization);
    w.value(c.field9a4);
    w.value(c.field9a8);
    for (const Item &slot : c.equipment)
        w.bytes(slot.data(), slot.size());
    w.text(c.name);
    w.bytes(c.looks.bytes.data(), c.looks.bytes.size());
    w.count(c.inventory.size());
    for (const auto &tab : c.inventory) {
        w.count(tab.size());
        for (const Stack &s : tab) {
            w.value(s.count);
            w.bytes(s.item.data(), s.item.size());
        }
    }
    w.value(c.gold);
    w.value(c.fieldAb0);
    writeItems(w, c.recipes);
    writeSet(w, c.discoveredZones);
    writeSet(w, c.set48);
    writeSet(w, c.discoveredLands);
    writeMap(w, c.map88);
    writeMap(w, c.map98);
    writeSet(w, c.usedCells);
    writeSet(w, c.setB8);
    writeSet(w, c.setC8);
    writeSet(w, c.finishedMissions);
    writeMap(w, c.missionTimers);
    w.value(c.worldSeed);
    w.text(c.worldName);
    writeCells(w, c.flightPoints);
    w.cell(c.homeCell);
    w.count(c.buffs.size());
    for (const Buff &b : c.buffs)
        w.bytes(b.bytes.data(), b.bytes.size());
    writeMap(w, c.regionValues);
    size_t stars = 0;
    for (const auto &[group, list] : c.stars)
        stars += list.size();
    w.count(stars);
    for (const auto &[group, list] : c.stars)
        for (const Star &s : list) {
            w.value(s.unknown);
            w.value(s.x), w.value(s.y), w.value(s.z);
        }
    writeCells(w, c.regions118);
    w.count(c.purchases.size());
    for (const auto &[shop, bought] : c.purchases) {
        w.value(shop);
        writeItems(w, bought);
    }
    return std::move(w.out);
}

void ensureLoadRecipe(Character &c) {
    const Item wanted = makeItem(1, 7);
    for (const Item &r : c.recipes)
        if (nativeinteract::sameItem(r.data(), wanted.data()))
            return;
    c.recipes.push_back(wanted);
}

// --- world.db --------------------------------------------------------------------------------

void readWorld(const std::vector<uint8_t> &blob, WorldState &w) {
    Reader r(blob);
    r.value(w.version);
    r.value(w.day);
    r.value(w.timeMs);
    if (w.version != WORLD_VERSION) // day and time are kept even so
        return;
    w.dead.clear(), w.rewards.clear(), w.zones.clear();
    readIds(r, w.dead);
    readIds(r, w.rewards);
    int32_t zones;
    if (!r.count(zones))
        return;
    for (int32_t i = 0; i < zones; i++) {
        Cell key;
        ZoneState z;
        if (!r.cell(key) || !r.value(z.flags))
            return;
        readRecords(r, z.groundItems);
        readRecords(r, z.staticObjects);
        {
            int32_t n;
            if (r.count(n))
                for (int32_t k = 0; k < n; k++) {
                    BlockChange b;
                    if (!r.bytes(b.bytes.data(), b.bytes.size()))
                        break;
                    z.blockChanges.push_back(b);
                }
        }
        readMap(r, z.placeCounters);
        const bool complete = r.value(z.deadTracked);
        w.zones.emplace(key, std::move(z));
        if (!complete)
            return;
    }
}

std::vector<uint8_t> writeWorld(const WorldState &w) {
    Writer out;
    out.value(WORLD_VERSION);
    out.value(w.day);
    out.value(w.timeMs);
    writeIds(out, w.dead);
    writeIds(out, w.rewards);
    out.count(w.zones.size());
    for (const auto &[key, z] : w.zones) {
        out.cell(key);
        out.value(z.flags);
        writeRecords(out, z.groundItems);
        writeRecords(out, z.staticObjects);
        out.count(z.blockChanges.size());
        for (const BlockChange &b : z.blockChanges)
            out.bytes(b.bytes.data(), b.bytes.size());
        writeMap(out, z.placeCounters);
        out.value(z.deadTracked);
    }
    return std::move(out.out);
}

void recordZone(WorldState &world, Cell zone, const ZoneLists &lists) {
    auto found = world.zones.find(zone);
    // The record is made the first time anything has to be kept.
    auto record = [&]() -> ZoneState & {
        if (found == world.zones.end())
            found = world.zones.emplace(zone, ZoneState{}).first;
        return found->second;
    };
    if (lists.groundItemsChanged) {
        ZoneState &z = record();
        z.flags |= ZoneState::GROUND_ITEMS_SAVED;
        z.groundItems = lists.groundItems;
    }
    if (lists.staticObjectsChanged) {
        ZoneState &z = record();
        z.flags |= ZoneState::STATIC_OBJECTS_SAVED;
        z.staticObjects = lists.staticObjects;
    }
    if (!lists.blockChanges.empty() || found != world.zones.end())
        record().blockChanges = lists.blockChanges;
    if (!lists.placeCounters.empty() || found != world.zones.end())
        record().placeCounters = lists.placeCounters;
    if (lists.deadTracked != 0 || found != world.zones.end())
        record().deadTracked = lists.deadTracked;
}

void applyStoredZone(const WorldState &world, Cell zone, ZoneLists &lists, bool serverMode,
                     const std::function<void(const BlockChange &)> &setBlock) {
    if (serverMode)
        return;
    auto found = world.zones.find(zone);
    if (found == world.zones.end())
        return;
    const ZoneState &z = found->second;
    if (z.flags & ZoneState::GROUND_ITEMS_SAVED)
        lists.groundItems = z.groundItems;
    if (z.flags & ZoneState::STATIC_OBJECTS_SAVED) {
        lists.staticObjects = z.staticObjects;
        lists.staticObjectsChanged = true;
    }
    lists.blockChanges = z.blockChanges;
    if (setBlock)
        for (const BlockChange &b : lists.blockChanges)
            setBlock(b);
    lists.deadTracked = z.deadTracked;
    lists.placeCounters = z.placeCounters;
}

void resetWorld(WorldState &w) {
    w.dead.clear();
    w.zones.clear();
}

void resetCharacterForWorld(Character &c) {
    constexpr uint8_t RESET_TYPE = 0x16;
    for (auto &tab : c.inventory)
        tab.erase(std::remove_if(tab.begin(), tab.end(), [](const Stack &s) { return s.item[item::TYPE] == RESET_TYPE; }),
                  tab.end());
    c.map88.clear();
    c.finishedMissions.clear();
    c.purchases.clear();
}

// --- zlib ------------------------------------------------------------------------------------

std::vector<uint8_t> compress(const std::vector<uint8_t> &data) {
    uLongf size = compressBound((uLong)data.size());
    std::vector<uint8_t> out(size);
    if (compress2(out.data(), &size, data.data(), (uLong)data.size(), Z_DEFAULT_COMPRESSION) != Z_OK)
        return {};
    out.resize(size);
    return out;
}

std::vector<uint8_t> inflate(const std::vector<uint8_t> &data) {
    constexpr size_t CHUNK = 0x1f400; // as 0x66dc0 inflates
    std::vector<uint8_t> out;
    z_stream s{};
    if (inflateInit(&s) != Z_OK)
        return out;
    s.next_in = const_cast<Bytef *>(data.data());
    s.avail_in = (uInt)data.size();
    std::vector<uint8_t> chunk(CHUNK);
    for (;;) {
        s.next_out = chunk.data();
        s.avail_out = (uInt)chunk.size();
        const int status = ::inflate(&s, Z_NO_FLUSH);
        out.insert(out.end(), chunk.data(), chunk.data() + (chunk.size() - s.avail_out));
        if (status != Z_OK) // the end of the stream, or damage: keep what came out
            break;
        if (s.avail_out != 0 && s.avail_in == 0)
            break; // the input ran out before the stream's end
    }
    inflateEnd(&s);
    return out;
}

// --- The databases ---------------------------------------------------------------------------

namespace {

// A prepared statement, finalised when it goes.
class Statement {
  public:
    Statement(sqlite3 *db, const char *sql) {
        if (db)
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    }
    ~Statement() { sqlite3_finalize(stmt); }
    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;
    explicit operator bool() const { return stmt != nullptr; }
    sqlite3_stmt *get() const { return stmt; }
    void bindText(int i, const std::string &s) { sqlite3_bind_text(stmt, i, s.c_str(), (int)s.size(), SQLITE_TRANSIENT); }
    // An empty value is bound as a null pointer of length 0, as the game binds it.
    void bindBlob(int i, const std::vector<uint8_t> &v) {
        sqlite3_bind_blob(stmt, i, v.empty() ? nullptr : v.data(), (int)v.size(), SQLITE_TRANSIENT);
    }
    int step() { return sqlite3_step(stmt); }

  private:
    sqlite3_stmt *stmt = nullptr;
};

std::string utf8Path(const fs::path &p) {
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

} // namespace

BlobDatabase::~BlobDatabase() { close(); }

void BlobDatabase::close() {
    if (db)
        sqlite3_close(db);
    db = nullptr;
}

bool BlobDatabase::open(const fs::path &file, std::string *why) {
    close();
    readOnly = false;
    if (sqlite3_open_v2(utf8Path(file).c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        if (why)
            *why = db ? sqlite3_errmsg(db) : "cannot open " + utf8Path(file);
        close();
        return false;
    }
    // (fails harmlessly when the table exists)
    sqlite3_exec(db, "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);", nullptr, nullptr, nullptr);
    return true;
}

bool BlobDatabase::openReadOnly(const fs::path &file, std::string *why) {
    close();
    readOnly = true;
    if (!fs::exists(file)) {
        if (why)
            *why = "no " + utf8Path(file);
        return false;
    }
    if (sqlite3_open_v2(utf8Path(file).c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (why)
            *why = db ? sqlite3_errmsg(db) : "cannot open " + utf8Path(file);
        close();
        return false;
    }
    return true;
}

bool BlobDatabase::read(const std::string &key, std::vector<uint8_t> &value) const {
    Statement s(db, "SELECT value FROM blobs WHERE key = ?");
    if (!s)
        return false;
    s.bindText(1, key);
    if (s.step() != SQLITE_ROW)
        return false;
    const auto *p = static_cast<const uint8_t *>(sqlite3_column_blob(s.get(), 0));
    const int n = sqlite3_column_bytes(s.get(), 0);
    value.assign(p, p + (p ? n : 0));
    return true;
}

bool BlobDatabase::write(const std::string &key, const std::vector<uint8_t> &value) {
    if (!db || readOnly)
        return false;
    bool exists = false;
    {
        Statement s(db, "SELECT 1 FROM blobs WHERE key = ?");
        if (!s)
            return false;
        s.bindText(1, key);
        exists = s.step() == SQLITE_ROW;
    }
    Statement s(db, exists ? "UPDATE blobs SET value=? WHERE key=?" : "INSERT INTO blobs(key, value) VALUES(?, ?)");
    if (!s)
        return false;
    if (exists) {
        s.bindBlob(1, value);
        s.bindText(2, key);
    } else {
        s.bindText(1, key);
        s.bindBlob(2, value);
    }
    return s.step() == SQLITE_DONE;
}

bool BlobDatabase::remove(const std::string &key) {
    if (!db || readOnly)
        return false;
    Statement s(db, "DELETE FROM blobs WHERE key = ?");
    if (!s)
        return false;
    s.bindText(1, key);
    return s.step() == SQLITE_DONE;
}

bool BlobDatabase::rename(const std::string &from, const std::string &to) {
    if (!db || readOnly)
        return false;
    Statement s(db, "UPDATE blobs SET key = ? WHERE key = ?");
    if (!s)
        return false;
    s.bindText(1, to);
    s.bindText(2, from);
    return s.step() == SQLITE_DONE;
}

std::vector<std::string> BlobDatabase::keys() const {
    std::vector<std::string> out;
    Statement s(db, "SELECT key FROM blobs ORDER BY key");
    if (!s)
        return out;
    while (s.step() == SQLITE_ROW)
        out.emplace_back(reinterpret_cast<const char *>(sqlite3_column_text(s.get(), 0)));
    return out;
}

bool BlobDatabase::readInt(const std::string &key, int32_t &value) const {
    std::vector<uint8_t> bytes;
    if (!read(key, bytes) || bytes.size() < 4)
        return false;
    std::memcpy(&value, bytes.data(), 4);
    return true;
}

bool BlobDatabase::writeInt(const std::string &key, int32_t value) {
    std::vector<uint8_t> bytes(4);
    std::memcpy(bytes.data(), &value, 4);
    return write(key, bytes);
}

// --- characters.db ---------------------------------------------------------------------------

namespace {
const char *const COUNT_KEY = "num";
const char *const SLOT_KEY = "slot";
std::string characterKey(int index) { return std::to_string(index); }
std::string notesKey(int index) { return "cubevulkan:" + std::to_string(index); }
} // namespace

bool CharacterStore::open(const fs::path &file, std::string *why) { return db.open(file, why); }

int CharacterStore::count() const {
    int32_t n = 0;
    return db.readInt(COUNT_KEY, n) ? std::max(0, n) : 0;
}

std::optional<int> CharacterStore::slot() const {
    int32_t s;
    if (!db.readInt(SLOT_KEY, s))
        return std::nullopt;
    return s;
}

bool CharacterStore::setSlot(int index) { return db.writeInt(SLOT_KEY, index); }

bool CharacterStore::load(int index, Character &out) const {
    out = Character{};
    std::vector<uint8_t> packed;
    if (!db.read(characterKey(index), packed))
        return false;
    readCharacter(inflate(packed), out);
    ensureLoadRecipe(out);
    return true;
}

bool CharacterStore::save(int index, const Character &c) {
    if (index < 0)
        return false; // (a negative selection saves nothing, §4.5)
    const std::vector<uint8_t> packed = compress(writeCharacter(c));
    return !packed.empty() && db.write(characterKey(index), packed);
}

int CharacterStore::create(const Character &c) {
    const int index = count();
    if (!db.writeInt(COUNT_KEY, index + 1) || !save(index, c) || !setSlot(index))
        return -1;
    return index;
}

bool CharacterStore::remove(int index) {
    const int before = count();
    if (index < 0 || index >= before)
        return false;
    const int after = before - 1;
    bool ok = db.writeInt(COUNT_KEY, after);
    ok = db.remove(characterKey(index)) && ok;
    db.remove(notesKey(index));
    for (int j = index; j <= after; j++) { // (the last rename names a key that is not there)
        ok = db.rename(characterKey(j + 1), characterKey(j)) && ok;
        db.rename(notesKey(j + 1), notesKey(j));
    }
    return ok;
}

bool CharacterStore::readNotes(int index, std::string &text) const {
    std::vector<uint8_t> bytes;
    if (!db.read(notesKey(index), bytes))
        return false;
    text.assign(bytes.begin(), bytes.end());
    return true;
}

bool CharacterStore::writeNotes(int index, const std::string &text) {
    if (index < 0)
        return false;
    return db.write(notesKey(index), std::vector<uint8_t>(text.begin(), text.end()));
}

void prepareNewCharacter(Character &c) {
    c.level = 1;
    c.usedCells.insert(c.homeCell);
}

// --- world.db --------------------------------------------------------------------------------

namespace {
const char *const STATE_KEY = "state";
}

bool readWorldFile(const fs::path &file, WorldState &out, std::string *why) {
    BlobDatabase db;
    if (!db.open(file, why))
        return false;
    std::vector<uint8_t> state;
    if (!db.read(STATE_KEY, state)) {
        if (why)
            *why = "no world state in " + utf8Path(file);
        return false;
    }
    readWorld(state, out);
    return true;
}

bool writeWorldFile(const fs::path &file, const WorldState &w, std::string *why) {
    BlobDatabase db;
    if (!db.open(file, why))
        return false;
    if (!db.write(STATE_KEY, writeWorld(w))) {
        if (why)
            *why = "cannot write the world state to " + utf8Path(file);
        return false;
    }
    return true;
}

// --- Importing the game's saves --------------------------------------------------------------

bool importGameSaves(const fs::path &gameSaveDir, const fs::path &ownSaveDir, std::string &report) {
    report.clear();
    std::error_code ec;
    if (fs::exists(ownSaveDir, ec) && fs::exists(gameSaveDir, ec) && fs::equivalent(gameSaveDir, ownSaveDir, ec)) {
        report = "not imported: the game's Save folder is CubeVulkan's own\n";
        return false;
    }
    fs::create_directories(ownSaveDir, ec);
    bool any = false;
    for (const char *name : {"characters.db", "world.db"}) {
        const fs::path from = gameSaveDir / name, to = ownSaveDir / name;
        if (fs::exists(to, ec)) {
            report += std::string(name) + ": kept CubeVulkan's own (delete it to import again)\n";
            continue;
        }
        BlobDatabase source;
        std::string why;
        if (!source.openReadOnly(from, &why)) {
            report += std::string(name) + ": not imported (" + why + ")\n";
            continue;
        }
        sqlite3 *target = nullptr;
        if (sqlite3_open_v2(utf8Path(to).c_str(), &target, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
            report += std::string(name) + ": cannot create " + utf8Path(to) + "\n";
            sqlite3_close(target);
            continue;
        }
        sqlite3_backup *copy = sqlite3_backup_init(target, "main", source.handle(), "main");
        int status = SQLITE_ERROR;
        if (copy) {
            status = sqlite3_backup_step(copy, -1);
            sqlite3_backup_finish(copy);
        }
        const bool done = status == SQLITE_DONE;
        report += std::string(name) + (done ? ": copied from " + utf8Path(from) + " (read only)\n"
                                            : ": copy failed (" + std::string(sqlite3_errmsg(target)) + ")\n");
        sqlite3_close(target);
        if (!done)
            fs::remove(to, ec);
        any = any || done;
    }
    return any;
}

} // namespace nativesaves
