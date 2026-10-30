#include "SteamSceneRange.h"
#include "Assets.h"
#include "Extracted.h"
#include "AlphaLook.h"
#include "GameMath.h"
#include <cmath>
#include <iostream>
#include <map>

int steamMeshCostTests() {
    Assets assets;
    if (!assets.locate("")) {
        std::cout << "SKIP native mesh costs: owner installation unavailable\n";
        return 0;
    }
    const auto folder = ExtractedWorld::find(assets.gameDir());
    ExtractedWorld world;
    std::string error;
    if (folder.empty() || !world.open(folder,error)) {
        std::cout << "SKIP native mesh costs: no owner zone capture\n";
        return 0;
    }
    const auto previous = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    int compared = 0, incomplete = 0, failures = 0;
    // A long-lived export can retain its old origin while new captures are elsewhere.
    // Sample recent cache evidence by its absolute filename, not an assumed home square.
    std::vector<std::filesystem::directory_entry> records;
    for (const auto &entry : std::filesystem::directory_iterator(folder))
        if (entry.is_regular_file() && entry.path().extension() == ".rec") records.push_back(entry);
    std::sort(records.begin(),records.end(),[](const auto &a, const auto &b) {
        if (a.last_write_time() != b.last_write_time()) return a.last_write_time() > b.last_write_time();
        return a.path() < b.path();
    });
    size_t examined = 0;
    for (const auto &entry : records) {
        if (compared >= 12 || examined++ >= 256) break;
        int absoluteX, absoluteY;
        auto zoneFile = entry.path();
        zoneFile.replace_extension(".cwz"); // shared parser validates the matching block-file name
        if (!ExtractedWorld::zoneCoordinates(zoneFile,absoluteX,absoluteY)) continue;
        const int64_t localX = (int64_t)absoluteX - world.originZoneX(), localY = (int64_t)absoluteY - world.originZoneY();
        if (localX < INT32_MIN || localX > INT32_MAX || localY < INT32_MIN || localY > INT32_MAX) continue;
        const int x = (int)localX, y = (int)localY;
        const auto reference = world.meshCapture(x,y);
        if (!reference || reference->dirty || reference->pendingGroups || !reference->activeGroups) { ++incomplete; continue; }
        Mesh opaque, water, lava;
        std::optional<scenevisibility::Bounds> bounds;
        if (!world.mesh(x,y,opaque,water,&bounds,nullptr,&lava) || !bounds || !bounds->triangleCost) { ++incomplete; continue; }
        ++compared;
        if (bounds->minimum != reference->minimum || bounds->maximum != reference->maximum ||
            bounds->center != reference->center || bounds->radius != reference->radius) {
            ++failures;
            std::cout << "FAIL native cached bounds " << x << ',' << y << ": min/max/center/radius differ\n";
        }
        if (*bounds->triangleCost != reference->triangleCost) {
            ++failures;
            std::cout << "FAIL native mesh cost " << x << ',' << y << ": " << *bounds->triangleCost << " vs captured " << reference->triangleCost << '\n';
        }
    }
    setRenderStyle(previous);
    if (!compared) std::cout << "SKIP native mesh-cost comparisons: no completed supported snapshots in the bounded sample\n";
    std::cout << "Native mesh-cache owner oracle: " << compared << " compared, " << incomplete << " incomplete, " << failures << " mismatches\n";
    return failures;
}

int steamSceneRangeTests() {
    using namespace scenerange;
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) { std::cout << "FAIL Steam scene range: " << what << '\n'; ++failures; }
    };
    Frame frame;
    frame.zoneRadius = 3;
    auto available = [](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
        scenevisibility::Bounds bounds;
        bounds.center = {(x*64+32)*65536,(y*64+32)*65536,0};
        bounds.meshed = true;
        bounds.triangleCost = 0;
        return bounds;
    };
    frame.zone = available;
    check(candidate(frame) == 147.2f, "an unspent budget retains the window bound, not the final accepted distance");
    check(triangleCost(12,6,6) == 10 && triangleCost(6,6,0) == 6 && triangleCost(0,6,0) == 4,
          "ordinary, wet-film and optimized-water quads keep distinct native weights");
    frame.camera = {32*65536,32*65536,0};
    frame.zone = [&](int64_t x, int64_t y) { return x == 1 && y == 0 ? std::nullopt : available(x,y); };
    const auto missing = candidate(frame);
    check(missing && std::abs(*missing - .454833984375f) < .000001f,
          "missing readiness uses the lower XY corner, not nearest box edge or center");
    frame.camera = {-1,-1,0};
    int64_t firstX = 0, firstY = 0;
    bool first = true;
    frame.zone = [&](int64_t x, int64_t y) {
        if (first) { firstX = x; firstY = y; first = false; }
        return available(x,y);
    };
    candidate(frame);
    check(firstX == -4 && firstY == -4, "negative fixed coordinates floor into their camera zone");

    frame.camera = {};
    frame.zone = [](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
        scenevisibility::Bounds bounds;
        bounds.center = {500*65536,500*65536,0};
        bounds.meshed = true;
        bounds.triangleCost = 0;
        if (y == 0 && (x == 0 || x == 1)) {
            bounds.center = {(x == 0 ? 60 : 100)*65536,0,0};
            bounds.triangleCost = 60000;
        }
        return bounds;
    };
    const auto crossed = candidate(frame);
    check(crossed && *crossed == 15.200000762939453f, "budget crossing chooses the previous smaller distance");
    frame.visible = [](const auto &bounds, float radius) { return radius == 100000 && bounds.center[0] != 60*65536; };
    check(candidate(frame) == 147.2f, "out-of-view mesh costs do not spend the accepted budget");
    frame.visible = {};
    const auto weighted = frame.zone;
    frame.zone = [&](int64_t x, int64_t y) {
        auto bounds = weighted(x,y);
        if (x == 0 && y == 0) bounds->triangleCost = 120000;
        return bounds;
    };
    check(candidate(frame) == .001f, "first-entry budget exhaustion has previous distance zero");
    frame.zone = [&](int64_t x, int64_t y) {
        auto bounds = weighted(x,y);
        if (x == 0 && y == 0) bounds->triangleCost.reset();
        return bounds;
    };
    check(!candidate(frame), "active meshes without native face metadata stay explicitly unsupported");

    frame.zoneRadius = 4;
    frame.zone = [](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
        scenevisibility::Bounds bounds;
        bounds.center = {10000LL*65536,10000LL*65536,0};
        bounds.meshed = true; bounds.triangleCost = 0;
        if (x == 0 && y == 0) bounds.center = {};
        else if (std::max(std::abs(x),std::abs(y)) == 4) {
            bounds.center = {100*65536,0,0};
            if (x == -4 && y == -4) bounds.triangleCost = 16*12288;
        }
        return bounds;
    };
    check(candidate(frame) == 55.2f,
          "33 equal-distance zone entries retain the native permutation before spending the mesh budget");
    frame.zoneRadius = 3;
    CachePublication publication;
    bool oldGroupsActive = true;
    auto oldCache = *available(0,0);
    oldCache.center = {}; // make this the first accepted distance, independently of neighboring ties
    oldCache.triangleCost = 120000;
    frame.zone = [&](int64_t x, int64_t y) {
        if (x != 0 || y != 0) return available(x,y);
        auto bounds = publication.read().value_or(oldCache);
        bounds.meshed = oldGroupsActive;
        return std::optional<scenevisibility::Bounds>(bounds);
    };
    check(candidate(frame) == .001f, "old completed geometry initially spends its existing cache cost");
    auto pendingCache = oldCache;
    pendingCache.center[0] = 100*65536;
    pendingCache.triangleCost = 0;
    publication.publish(pendingCache);
    check(candidate(frame) == 147.2f, "completed pending metadata supplies new cost while old groups remain active");
    oldGroupsActive = false;
    check(candidate(frame) == .001f, "pending metadata alone does not admit an unactivated geometry group");
    frame.zone = available;
    frame.zoneRadius = 15;
    frame.elapsedMs = 1000;
    State warm;
    check(warm.waterOpacity() == 1, "stock water opacity starts at one");
    check(warm.update(frame) && warm.value().scene > 256 && warm.value().models == 192 && warm.value().abilities == 256,
          "initial growth and the distinct default model/ability caps");
    State capped;
    frame.elapsedMs = 100000;
    capped.update(frame);
    check(capped.value().scene == warm.value().scene && capped.waterOpacity() == warm.waterOpacity(), "scene coefficients share the capped, float-rounded wall interval");
    State fractional;
    frame.elapsedMs = 9.9999999; // rounds to float10 before the hundredths-ms count
    fractional.update(frame);
    const float expected = *candidate(frame) * gamemath::blendFactor(10.0, .0005f);
    check(fractional.value().scene == expected && fractional.waterOpacity() == 1,
          "scene time rounds to float before integration, while stock opacity stays one");
    State flagged = warm;
    const float before = flagged.value().scene;
    frame.elapsedMs = 16;
    frame.player.effect13 = true;
    flagged.update(frame);
    check(flagged.value().scene < before && flagged.value().scene > 256, "effect13 eases downward to256 instead of snapping");
    frame.player.effect13 = false;
    frame.player.blockFlags = 0x22;
    State ground = warm;
    ground.update(frame);
    check(ground.value().scene == flagged.value().scene && ground.flaggedColour() == groundColour({}) / 255.f,
          "both block flag bits and nonzero native colour select the same exceptional range branch");
    check(groundColour({-65535,-65535,0}) == groundColour({}), "flag colour truncates negative physical XY toward zero");
    frame.zoneRadius = 3;
    frame.elapsedMs = 0;
    flagged.update(frame);
    check(flagged.value().scene == 147.2f, "candidate at most256 snaps downward even with exceptional ground flags");
    frame.player = {};
    frame.zone = [](int64_t, int64_t) -> std::optional<scenevisibility::Bounds> { return {}; };
    frame.player.ability = 0x9c;
    frame.player.abilityMs = 1001;
    State flight = warm;
    flight.update(frame);
    check(flight.value().scene == before, "late flight uses easing even for a smaller target and zero elapsed time");
    frame.player.abilityMs = 1000;
    flight.update(frame);
    check(flight.value().scene == .001f, "the special flight gate is strictly after1000ms");
    const auto derived = warm.value();
    const float ready = warm.waterOpacity();
    warm.reset();
    check(warm.value().scene == 0 && warm.value().models == derived.models && warm.value().abilities == derived.abilities &&
          warm.waterOpacity() == ready, "world reset clears only the current range until the next scene update");
    std::cout << "Steam adaptive scene range: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
