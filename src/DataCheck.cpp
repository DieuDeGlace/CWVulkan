#include "DataCheck.h"
#include "Assets.h"
#include "Character.h"
#include "ClassMoves.h"
#include "Creatures.h"
#include "CubModel.h"
#include "Extracted.h"
#include "Gear.h"
#include "Saves.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace dataCheck {
namespace {
bool nonempty(const fs::path &file) {
    std::error_code ec;
    return fs::is_regular_file(file, ec) && fs::file_size(file, ec) > 0 && !ec;
}
bool safeSprite(const std::string &name) {
    return !name.empty() && name.find_first_of("/\\:") == std::string::npos && name != "." && name != "..";
}
std::string examples(const std::vector<std::string> &items) {
    std::string out;
    for (size_t i = 0; i < std::min<size_t>(items.size(), 4); i++) out += (i ? "; " : "") + items[i];
    if (items.size() > 4) out += "; ...";
    return out;
}
bool placementFile(const fs::path &file, std::string &error) {
    std::ifstream in(file, std::ios::binary | std::ios::ate);
    auto length = in ? in.tellg() : std::streampos(-1);
    if (length < 4 || length > 128 * 1024 * 1024) { error = "invalid placement file size"; return false; }
    in.seekg(0);
    char magic[4]; in.read(magic, 4);
    if (std::memcmp(magic, "CWR1", 4)) { error = "missing CWR1 header"; return false; }
    std::streamoff at = 4;
    while (at < length) {
        uint32_t header[3];
        if (length - at < 12 || !in.read((char *)header, 12)) { error = "truncated placement header"; return false; }
        at += 12;
        uint64_t bytes = (uint64_t)header[1] * header[2];
        if (!header[1] || bytes > (uint64_t)(length - at)) { error = "truncated placement records"; return false; }
        at += (std::streamoff)bytes;
        in.seekg(at);
    }
    return true;
}
}

bool gameBuild(const fs::path &exe, uint32_t &timestamp, std::string &error) {
    timestamp = 0; error.clear();
    std::ifstream in(exe, std::ios::binary | std::ios::ate);
    auto length = in ? in.tellg() : std::streampos(-1);
    auto fail = [&](const char *why) { error = why; return false; };
    if (length < 64) return fail("cubeworld.exe is missing or truncated");
    uint8_t dos[64]; in.seekg(0);
    if (!in.read((char *)dos, sizeof dos) || dos[0] != 'M' || dos[1] != 'Z') return fail("invalid executable DOS header");
    uint32_t offset; std::memcpy(&offset, dos + 0x3c, 4);
    if (offset < 64 || offset > (uint64_t)length - 26) return fail("invalid executable PE offset");
    uint8_t pe[26]; in.seekg(offset);
    if (!in.read((char *)pe, sizeof pe) || std::memcmp(pe, "PE\0\0", 4)) return fail("invalid executable PE signature");
    uint16_t machine, optionalSize, magic;
    std::memcpy(&machine, pe + 4, 2); std::memcpy(&optionalSize, pe + 20, 2); std::memcpy(&magic, pe + 24, 2);
    if (machine != 0x8664 || magic != 0x20b || optionalSize < 112 ||
        (uint64_t)offset + 24 + optionalSize > (uint64_t)length) return fail("executable is not a complete x64 PE image");
    std::memcpy(&timestamp, pe + 8, 4);
    return true;
}

bool Report::errors() const {
    return std::any_of(entries.begin(), entries.end(), [](const Entry &e) { return e.status == Status::Error; });
}
std::string Report::text() const {
    std::ostringstream out;
    out << "CubeVulkan owner data check (" << (deep ? "deep" : "quick") << ")\nGame: " << utf8(game)
        << "\nWorld: " << (world.empty() ? "not found" : utf8(world))
        << "\nGameplay data: " << (gameplayData ? "ready to test" : "incomplete")
        << "\nTitle-menu data: " << (frontendData ? "ready to test" : "incomplete") << "\n\n";
    for (const auto &e : entries) {
        out << (e.status == Status::Pass ? "[OK] " : e.status == Status::Warning ? "[WARN] " : "[ERROR] ")
            << e.name << ": " << e.detail << '\n';
        if (!e.action.empty()) out << "  Next: " << e.action << '\n';
    }
    if (!estimatedObjectTypes.empty()) {
        out << "\nObject types still using estimated sizes:\n";
        for (const auto &name : estimatedObjectTypes) out << "  " << name << '\n';
    }
    if (!deep) out << "\nQuick mode indexes non-home terrain, placements, sprites and creature files. Use --deep to validate their payloads and inspect object sizes.\n";
    out << "\nThis checks available data, not complete gameplay or visual parity. No game files or saves were changed.\n";
    return out.str();
}

Report inspect(const fs::path &game, const fs::path &wantedWorld, bool deep, const Progress &progress) {
    Report r; r.game = game; r.deep = deep;
    auto add = [&](Status status, const std::string &name, const std::string &detail, const std::string &action = "") {
        r.entries.push_back({status, name, detail, action});
    };
    uint32_t stamp = 0; std::string why;
    bool exe = gameBuild(game / "cubeworld.exe", stamp, why);
    r.supportedBuild = exe && stamp == SUPPORTED_BUILD;
    std::ostringstream stampText; stampText << std::hex << std::setw(8) << std::setfill('0') << stamp;
    add(!exe ? Status::Error : r.supportedBuild ? Status::Pass : Status::Warning, "Game build",
        exe ? stampText.str() + (r.supportedBuild ? " (supported extractor build)" : " (unsupported extractor build)") : why,
        r.supportedBuild ? "" : "Select the installed supported Steam build before installing the extractor.");

    size_t sprites = 0; std::vector<std::string> badSprites;
    if (progress) progress("Checking original sprites...");
    std::error_code ec;
    for (const auto &e : fs::directory_iterator(game / "sprites", ec)) if (e.is_regular_file(ec) && e.path().extension() == ".cub") {
        sprites++;
        if (deep) {
            CubModel cub;
            if (e.file_size(ec) > 256 * 1024 * 1024) badSprites.push_back(utf8(e.path().filename()) + " (excessive file size)");
            else if (!cub.load(e.path(), &why)) badSprites.push_back(utf8(e.path().filename()) + " (" + why + ")");
        }
    }
    bool models = sprites > 0 && badSprites.empty() && !ec;
    add(models ? Status::Pass : Status::Error, "Original sprites", std::to_string(sprites) + " files" +
        (deep ? ", " + std::to_string(badSprites.size()) + " invalid" : " present (payloads not checked)") +
        (badSprites.empty() ? "" : "; " + examples(badSprites)), models ? "" : "Repair the owner's Steam installation; do not substitute generated assets.");
    bool fonts = nonempty(game / "resource1.dat") && nonempty(game / "resource2.dat");
    add(fonts ? Status::Pass : Status::Error, "Original fonts", fonts ? "both font resources present" : "font resource missing or empty",
        fonts ? "" : "Repair the owner's Steam installation.");

    r.world = wantedWorld.empty() ? ExtractedWorld::find(game) : wantedWorld;
    ExtractedWorld world; bool terrain = !r.world.empty() && world.open(r.world, why);
    if (terrain) {
        r.zones = world.zoneCount();
        if (progress) progress("Checking terrain and placement records: " + std::to_string(r.zones) + " zones...");
        std::vector<std::string> badZones, badPlacements;
        std::set<std::string> estimates, missingModels;
        size_t propsChecked = 0;
        size_t checked = 0;
        ec.clear();
        for (const auto &e : fs::directory_iterator(r.world, ec)) {
            int x, y;
            if (!e.is_regular_file(ec) || !ExtractedWorld::zoneCoordinates(e.path(), x, y)) continue;
            if (progress && deep && ++checked % 500 == 0) progress("Terrain: " + std::to_string(checked) + "/" + std::to_string(r.zones));
            if (deep && !ExtractedWorld::validateZoneFile(e.path(), why)) badZones.push_back(utf8(e.path().filename()) + " (" + why + ")");
            auto rec = e.path(); rec.replace_extension(".rec");
            if (nonempty(rec)) {
                r.placements++;
                if (deep) {
                    if (!placementFile(rec, why)) badPlacements.push_back(utf8(rec.filename()) + " (" + why + ")");
                    else {
                        for (const auto &p : world.propsAbsolute(x, y)) {
                            propsChecked++;
                            if (p.beam) continue; // generated primitive, not a missing owner voxel asset
                            if (p.estimatedScale) estimates.insert(p.model);
                            if (!safeSprite(p.model) || !nonempty(game / "sprites" / (p.model + ".cub"))) missingModels.insert(p.model);
                        }
                    }
                }
            }
        }
        terrain = badZones.empty() && !ec;
        add(terrain ? Status::Pass : Status::Error, "Terrain", std::to_string(r.zones) + " zones; home zone decoded" +
            (deep ? "; " + std::to_string(badZones.size()) + " invalid zones" : "; other zones indexed") +
            (badZones.empty() ? "" : "; " + examples(badZones)), terrain ? "" : "Re-extract the damaged zones from the owner's game.");
        bool props = r.placements == r.zones && badPlacements.empty();
        add(props ? Status::Pass : Status::Warning, "Placement records", std::to_string(r.placements) + "/" + std::to_string(r.zones) +
            " zones have records" + (badPlacements.empty() ? "" : "; " + examples(badPlacements)), props ? "" : "Revisit those zones with ZoneExtractor to record their objects.");
        if (deep) {
            r.estimatedObjectTypes.assign(estimates.begin(), estimates.end());
            add(missingModels.empty() ? Status::Pass : Status::Warning, "Placed sprites", std::to_string(propsChecked) +
                " decoded placements; " + std::to_string(missingModels.size()) + " missing model files" +
                (missingModels.empty() ? "" : ": " + examples(std::vector<std::string>(missingModels.begin(), missingModels.end()))),
                missingModels.empty() ? "" : "Capture the missing native geometry or mapping; replacement models are not generated.");
            add(estimates.empty() ? Status::Pass : Status::Warning, "Object sizes", std::to_string(estimates.size()) + " types use provisional size estimates",
                estimates.empty() ? "" : "These types need native transform measurements before size parity can be claimed; the full list follows.");
        }
    } else add(Status::Error, "Terrain", r.world.empty() ? "no extracted seed folder" : why,
               "Install ZoneExtractor, load the owner's character and extract terrain (F11). Keep manifest.txt with the zones.");
    for (const char *name : {"sprite-groups.txt", "sprite-scales.txt", "class-rules.txt", "item-functions.txt"}) {
        bool exists = nonempty(game / "Extract" / name);
        add(exists ? Status::Pass : Status::Warning, name, exists ? "present" : "missing or empty",
            exists ? "" : std::string(name).find("sprite") == 0 ? "Run the extractor with objects visible to record sprite mappings and sizes." : "Run the extractor's F8 gear survey.");
    }

    auto characters = listCharacters(game);
    if (progress) progress("Checking recorded characters...");
    for (const auto &entry : characters) {
        CharacterRecording recording;
        if (!recording.load(entry.dir, why)) {
            add(Status::Warning, "Character " + entry.id, why, "F5 records appearance; F4 records idle, movement and attacks.");
            continue;
        }
        std::vector<std::string> missingParts;
        for (const auto &p : recording.parts())
            if (!safeSprite(p.sprite) || !nonempty(game / "sprites" / (p.sprite + ".cub"))) missingParts.push_back(p.sprite);
        if (!missingParts.empty()) {
            add(Status::Warning, "Character " + entry.id, "missing sprites: " + examples(missingParts), "Re-capture the appearance from the owner's installed game.");
            continue;
        }
        r.characters++;
        ClassMoves moves = classMoves(entry.classType, entry.specialization);
        std::set<int> expected{moves.attack[0], moves.attack[1], moves.special, moves.releaseSpecial, moves.skill, moves.tap, moves.ultimate, moves.dodge};
        expected.erase(0);
        std::vector<std::string> missing;
        for (int ability : expected) if (recording.abilityDuration(ability) <= 0) missing.push_back(std::to_string(ability));
        bool basic = moves.attack[0] && recording.abilityDuration(moves.attack[0]) > 0 &&
                     moves.attack[1] && recording.abilityDuration(moves.attack[1]) > 0;
        r.combatCharacters += basic;
        add(missing.empty() && basic ? Status::Pass : Status::Warning, "Character " + entry.id, std::to_string(recording.frameCount()) +
            " usable frames; " + (basic ? "basic attacks available" : "basic attack missing") +
            (missing.empty() ? "" : "; missing mapped abilities " + examples(missing)), missing.empty() ? "" : "Record the missing moves with F4; no replacement animation is generated.");
        Gear gear;
        if (!gear.load(entry.dir, why)) add(Status::Warning, "Gear " + entry.id, why, "Run the extractor's F8 gear survey for this character.");
    }
    if (!r.characters) add(Status::Error, "Characters", "no usable recorded character", "Record a snapshot (F5) and animation clip (F4) in the original game.");

    auto creatureDir = r.world / "creatures";
    if (terrain && deep) {
        if (progress) progress("Checking creature recordings...");
        CreatureRecordings creatures;
        creatures.load(creatureDir, [&](const int64_t *at) { return world.toLocal(at); });
        const auto &s = creatures.stats();
        std::set<std::string> missingParts;
        size_t missingLootFlags = 0, missingLootModifiers = 0;
        for (const auto &c : creatures.all()) for (const auto &p : c.parts)
            if (!safeSprite(p.sprite) || !nonempty(game / "sprites" / (p.sprite + ".cub"))) missingParts.insert(p.sprite);
        for (const auto &c : creatures.all()) {
            missingLootFlags += !c.flags2.has_value();
            for (const auto &w : c.worn) missingLootModifiers += w.category && !w.modifier.has_value();
        }
        add(s.loaded == s.listed && s.loaded && missingParts.empty() ? Status::Pass : Status::Warning, "Creatures",
            std::to_string(s.loaded) + "/" + std::to_string(s.listed) + " captures usable; missing appearance " + std::to_string(s.missingAppearance) +
            ", clip " + std::to_string(s.missingFrames) + ", pose " + std::to_string(s.missingPose) + ", sprite files " + std::to_string(missingParts.size()),
            s.loaded == s.listed && s.loaded && missingParts.empty() ? "" : "Use F3 to record initialized, visible creatures and NPCs.");
        add(missingLootFlags || missingLootModifiers ? Status::Warning : Status::Pass, "Creature loot metadata",
            std::to_string(missingLootFlags) + " captures lack appearance flags; " + std::to_string(missingLootModifiers) + " worn items lack modifiers",
            missingLootFlags || missingLootModifiers ? "Use the updated extractor's F3 capture; old exports remain readable but cannot establish complete drop parity." : "");
    } else add(Status::Warning, "Creatures", nonempty(creatureDir / "creatures.txt") ? "manifest present; payloads not checked" : "no creature manifest",
               "Use --deep to validate captures, or F3 in the extractor to record them.");
    // The title is the ordinary world with the game's logo and interface (docs/menu.md).
    const bool menuData = nonempty(game / "sprites/cubeworld.cub") && nonempty(game / "gui.plx");
    add(menuData ? Status::Pass : Status::Warning, "Title screen", menuData ? "logo model and gui.plx present" : "sprites/cubeworld.cub or gui.plx missing",
        menuData ? "" : "Check the Cube World installation. --play can test gameplay without the title.");
    bool hud = nonempty(game / "Extract/ui/hud/hud.txt") && nonempty(game / "Extract/ui/hud/hp.png") &&
               nonempty(game / "Extract/ui/hud/mp.png") && nonempty(game / "Extract/ui/hud/xp.png");
    add(hud ? Status::Pass : Status::Warning, "HUD exports", hud ? "bar metadata and HP/MP/XP images present" : "bar metadata or images missing",
        hud ? "" : "Capture a native frame with F10, then run tools/HudExtract as described in README.md.");
    r.gameplayData = exe && models && fonts && terrain && r.characters > 0;
    r.frontendData = r.gameplayData && menuData;
    return r;
}
} // namespace dataCheck
