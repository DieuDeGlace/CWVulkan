#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "StateStore.h"
#include "TownRules.h"
#include <Windows.h>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <tuple>
#include <vector>

namespace encounters {
namespace {
constexpr uint64_t MaxTimestamp = 253402300799ull; // Last second of year 9999 UTC.
constexpr uintmax_t MaxFileBytes = 64 * 1024 * 1024;
constexpr unsigned MaxRace = 4095;

bool fail(std::string *error, const char *message) {
    if (error)
        *error = message;
    return false;
}
template <class T> bool number(std::istream &stream, T &value) {
    std::string token;
    if (!(stream >> token) || token.size() > 32)
        return false;
    auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size();
}
bool flag(std::istream &stream, bool &value) {
    unsigned parsed = 0;
    if (!number(stream, parsed) || parsed > 1)
        return false;
    value = parsed != 0;
    return true;
}
bool end(std::istream &stream) {
    stream >> std::ws;
    return stream.eof();
}
bool line(std::istream &stream, std::string &value) {
    if (!std::getline(stream, value) || value.size() > 4096)
        return false;
    if (!value.empty() && value.back() == '\r')
        value.pop_back();
    return true;
}
bool readRecord(const std::string &line, EventRecord &record, unsigned version) {
    std::istringstream stream(line);
    std::string marker;
    unsigned kind = 0;
    if (!(stream >> marker) || marker != "E" || !number(stream, record.world) ||
        !number(stream, record.region.x) || !number(stream, record.region.y) ||
        !number(stream, record.slot) || !number(stream, record.cycle) || !number(stream, kind) ||
        !number(stream, record.site.x) || !number(stream, record.site.y) ||
        !number(stream, record.previousSite.x) || !number(stream, record.previousSite.y) ||
        !number(stream, record.attempt) || !number(stream, record.enemyCount) ||
        !number(stream, record.defeatedMask) || !flag(stream, record.keyOwned) ||
        !flag(stream, record.keySearchFound) || !flag(stream, record.objectiveDone) ||
        !flag(stream, record.rewardClaimed) || !number(stream, record.deadlineUTCsecs) ||
        !number(stream, record.emptySince) || !flag(stream, record.planned))
        return false;
    record.kind = static_cast<Kind>(kind);
    for (size_t i = 0; i < record.enemyRaces.size(); ++i)
        if (!number(stream, record.enemyRaces[i]) || !number(stream, record.enemyStars[i]) ||
            !number(stream, record.enemyHPFraction[i]))
            return false;
    if (!number(stream, record.npcHPFraction) || !flag(stream, record.failed))
        return false;
    if (version >= 2 && (!flag(stream, record.townQuest) || !flag(stream, record.awaitingSite) ||
                         !number(stream, record.town.x) || !number(stream, record.town.y) ||
                         !number(stream, record.difficultyLevel)))
        return false;
    return end(stream) && StateStore::valid(record);
}
void writeRecord(std::ostream &stream, const EventRecord &record) {
    stream << "E " << record.world << ' ' << record.region.x << ' ' << record.region.y << ' '
           << record.slot << ' ' << record.cycle << ' ' << unsigned(record.kind) << ' '
           << record.site.x << ' ' << record.site.y << ' ' << record.previousSite.x << ' '
           << record.previousSite.y << ' ' << record.attempt << ' ' << record.enemyCount << ' '
           << record.defeatedMask << ' ' << record.keyOwned << ' ' << record.keySearchFound << ' '
           << record.objectiveDone << ' ' << record.rewardClaimed << ' ' << record.deadlineUTCsecs
           << ' ' << record.emptySince << ' ' << record.planned;
    for (size_t i = 0; i < record.enemyRaces.size(); ++i)
        stream << ' ' << record.enemyRaces[i] << ' ' << record.enemyStars[i] << ' '
               << record.enemyHPFraction[i];
    stream << ' ' << record.npcHPFraction << ' ' << record.failed << ' ' << record.townQuest
           << ' ' << record.awaitingSite << ' ' << record.town.x << ' ' << record.town.y
           << ' ' << record.difficultyLevel << '\n';
}
bool atomicWrite(const std::filesystem::path &path, const std::string &text, std::string *error) {
    auto temporary = path;
    temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                 std::to_wstring(GetTickCount64());
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return fail(error, "Cannot create temporary event state file.");
    DWORD written = 0;
    bool succeeded =
        WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
        written == text.size() && FlushFileBuffers(file);
    succeeded = CloseHandle(file) && succeeded;
    if (succeeded)
        succeeded = MoveFileExW(temporary.c_str(), path.c_str(),
                                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!succeeded) {
        DeleteFileW(temporary.c_str());
        return fail(error, "Cannot atomically replace event state file.");
    }
    return true;
}
} // namespace

EventRecord &StateStore::get(uint64_t world, Region region, unsigned slot) {
    EventKey key{world, region, slot};
    auto result = records_.try_emplace(key);
    if (result.second) {
        result.first->second.world = world;
        result.first->second.region = region;
        result.first->second.slot = slot;
    }
    return result.first->second;
}
const EventRecord *StateStore::find(uint64_t world, Region region, unsigned slot) const {
    auto found = records_.find({world, region, slot});
    return found == records_.end() ? nullptr : &found->second;
}
size_t StateStore::restartUnfinished(uint64_t world) {
    size_t restarted = 0;
    for (auto &entry : records_) {
        auto &old = entry.second;
        if (old.world != world || !old.planned || old.rewardClaimed || old.failed || old.townQuest)
            continue;
        EventRecord fresh;
        fresh.world = old.world;
        fresh.region = old.region;
        fresh.slot = old.slot;
        fresh.cycle = old.cycle + 1;
        fresh.previousSite = old.site;
        old = fresh;
        ++restarted;
    }
    return restarted;
}
bool StateStore::valid(const EventRecord &record) {
    auto coordinate = [](int64_t value) { return value >= INT32_MIN && value <= INT32_MAX; };
    if (record.slot >= MaxSlots || unsigned(record.kind) >= unsigned(Kind::Count) ||
        record.enemyCount > 6 || record.attempt > 65535 ||
        (record.defeatedMask & ~allEnemies(record.enemyCount)) != 0 ||
        record.deadlineUTCsecs > MaxTimestamp || record.emptySince > MaxTimestamp ||
        !coordinate(record.site.x) || !coordinate(record.site.y) ||
        !coordinate(record.previousSite.x) || !coordinate(record.previousSite.y) ||
        !coordinate(record.town.x) || !coordinate(record.town.y) ||
        record.difficultyLevel < 1 || record.difficultyLevel > 10000000 ||
        (record.townQuest && (!record.planned || !town::referralKind(record.kind))) ||
        (record.awaitingSite && (!record.townQuest || record.objectiveDone)))
        return false;
    if ((record.kind == Kind::ChestSack && record.enemyCount != 0) ||
        (groupEvent(record.kind) && record.enemyCount != 0 && record.enemyCount < 4) ||
        (landmarkEvent(record.kind) && record.enemyCount != 0 && record.enemyCount < 3) ||
        (!groupEvent(record.kind) && !landmarkEvent(record.kind) && record.enemyCount > 1) ||
        (record.planned && record.kind != Kind::ChestSack && record.enemyCount == 0))
        return false;
    auto health = [](float value) { return std::isfinite(value) && value >= 0.f && value <= 1.f; };
    if (!health(record.npcHPFraction))
        return false;
    for (size_t i = 0; i < record.enemyRaces.size(); ++i)
        if (record.enemyRaces[i] > MaxRace || record.enemyStars[i] > 4 ||
            !health(record.enemyHPFraction[i]) ||
            (i >= record.enemyCount && (record.enemyRaces[i] || record.enemyStars[i])))
            return false;
    return true;
}
bool StateStore::load(const std::filesystem::path &path, std::string *error) {
    if (error)
        error->clear();
    std::error_code status;
    auto bytes = std::filesystem::file_size(path, status);
    if (status || bytes > MaxFileBytes)
        return fail(error, "Event state file is missing, unreadable, or too large.");
    std::ifstream file(path, std::ios::binary);
    std::string textLine;
    if (!file || !line(file, textLine))
        return fail(error, "Cannot read event state header.");
    std::istringstream header(textLine);
    std::string name;
    unsigned version = 0;
    if (!(header >> name) || name != "OverworldEventsState" || !number(header, version) ||
        (version != 1 && version != 2) || !end(header))
        return fail(error, "Unsupported event state format.");
    if (!line(file, textLine))
        return fail(error, "Event state record count is missing.");
    std::istringstream countLine(textLine);
    size_t count = 0;
    if (!(countLine >> name) || name != "Records" || !number(countLine, count) ||
        count > MaxRecords || !end(countLine))
        return fail(error, "Invalid event state record count.");
    Records parsed;
    parsed.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        EventRecord record;
        if (!line(file, textLine) || !readRecord(textLine, record, version))
            return fail(error, "Invalid or truncated event state record.");
        if (!parsed.emplace(EventKey{record.world, record.region, record.slot}, record).second)
            return fail(error, "Duplicate event state key.");
    }
    if (!line(file, textLine) || textLine != "End")
        return fail(error, "Event state completion marker is missing.");
    file >> std::ws;
    if (!file.eof() || file.bad())
        return fail(error, "Unexpected trailing event state data.");
    records_.swap(parsed);
    return true;
}
bool StateStore::save(const std::filesystem::path &path, std::string *error) const {
    if (error)
        error->clear();
    if (records_.size() > MaxRecords)
        return fail(error, "Too many event state records.");
    std::vector<const EventRecord *> ordered;
    ordered.reserve(records_.size());
    for (const auto &entry : records_) {
        const auto &record = entry.second;
        if (!valid(record) || !(entry.first == EventKey{record.world, record.region, record.slot}))
            return fail(error, "Invalid in-memory event state.");
        ordered.push_back(&record);
    }
    std::sort(ordered.begin(), ordered.end(), [](const auto *a, const auto *b) {
        return std::tie(a->world, a->region.x, a->region.y, a->slot) <
               std::tie(b->world, b->region.x, b->region.y, b->slot);
    });
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(std::numeric_limits<float>::max_digits10);
    text << "OverworldEventsState 2\nRecords " << ordered.size() << '\n';
    for (const auto *record : ordered)
        writeRecord(text, *record);
    text << "End\n";
    return atomicWrite(path, text.str(), error);
}
} // namespace encounters
