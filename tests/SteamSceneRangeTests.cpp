#include "SteamSceneRange.h"
#include "SteamZoneDemand.h"
#include "Assets.h"
#include "Extracted.h"
#include "AlphaLook.h"
#include "GameMath.h"
#include <cmath>
#include <iostream>
#include <map>
#include <set>

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
    const auto source = zonedemand::clientWindow({0, 0, 0}, 50);
    int fresh = 0, withNeighbors = 0;
    for (int x = -15; x <= 15; ++x)
        for (int y = -15; y <= 15; ++y) {
            fresh += source.contains(x, y);
            withNeighbors += zonedemand::neighborhood(x, y, [&](int64_t xx, int64_t yy) { return source.contains(xx, yy); });
        }
    check(fresh == 793 && withNeighbors == 673, "default native source footprint and conditional nine-neighbor mesh count");
    check(source.contains(15, 5) && !source.contains(15, 6) && !source.contains(16, 0),
          "source admission combines the inclusive circle with its narrower tile box");
    check(!zonedemand::neighborhood(15, 0, [&](int64_t x, int64_t y) { return source.contains(x, y); }) &&
              zonedemand::neighborhood(15, 0, [&](int64_t x, int64_t y) {
                  return source.contains(x, y) || (x == 16 && y >= -1 && y <= 1);
              }),
          "previously loaded source neighbors remain eligible outside fresh demand");
    check(zonedemand::withinRetention(16, 0, 15) && !zonedemand::withinRetention(16, 1, 15),
          "native source retention keeps the exact radius and excludes positions beyond it");
    int retainedMeshCenters = 0;
    for (int x = -15; x <= 15; ++x)
        for (int y = -15; y <= 15; ++y)
            retainedMeshCenters += zonedemand::neighborhood(x, y, [&](int64_t xx, int64_t yy) {
                return source.contains(xx, yy) || zonedemand::withinRetention(xx, yy, 15);
            });
    check(retainedMeshCenters == 673, "an entirely cached world cannot expand the steady default mesh footprint");
    const auto shifted = zonedemand::clientWindow({6400LL * 65536, 6400LL * 65536, 0}, 50);
    check(shifted.center == std::array<int32_t, 2>{100, 100} && shifted.minimum == std::array<int32_t, 2>{85, 85} &&
              shifted.maximum == std::array<int32_t, 2>{115, 115},
          "source window retains absolute block-to-zone bounds");
    const auto negative = zonedemand::clientWindow({-65536 / 2, -64LL * 65536 - 65536 / 2, 0}, 50);
    check(negative.center == std::array<int32_t, 2>{0, -1} && gamemath::floorFixedZone(-65536 / 2) == -1 &&
              gamemath::floorFixedZone(-64LL * 65536 - 65536 / 2) == -2,
          "source truncation and corrected mesh centers differ at negative fractional boundaries");
    const auto wrapped = zonedemand::clientWindow({(1LL << 48) + 64LL * 65536, 0, 0}, 50);
    check(wrapped.center[0] == 1, "source narrows fixed blocks before selecting a zone");
    auto countSource = [](const zonedemand::Window &window) {
        int count = 0;
        for (int x = window.minimum[0]; x <= window.maximum[0]; ++x)
            for (int y = window.minimum[1]; y <= window.maximum[1]; ++y)
                count += window.contains(x, y);
        return count;
    };
    check(countSource(zonedemand::clientWindow({0, 0, 0}, 5)) == 11 &&
              countSource(zonedemand::clientWindow({32LL * 65536, 32LL * 65536, 0}, 5)) == 11 &&
              countSource(zonedemand::clientWindow({0, 0, 0}, 0)) == 9,
          "source distance minimum remains distinct from the scene's minimum radius");
    const auto hostEdge = zonedemand::hostWindow({0, 0, 0});
    const auto hostMiddle = zonedemand::hostWindow({14LL * 65536, 14LL * 65536, 0});
    const auto hostMixed = zonedemand::hostWindow({0, 14LL * 65536, 0});
    check(countSource(hostEdge) == 79 && countSource(hostMiddle) == 77 && countSource(hostMixed) == 78 &&
              countSource(zonedemand::hostWindow({50LL * 65536, 50LL * 65536, 0})) == 79,
          "host source demand retains its native phase-dependent 77/78/79 footprint");
    check(hostEdge.minimum == zonedemand::BlockPoint{-5, -5} && hostEdge.maximum == zonedemand::BlockPoint{4, 4} &&
              hostMiddle.minimum == zonedemand::BlockPoint{-4, -4} && hostMiddle.maximum == zonedemand::BlockPoint{4, 4},
          "host demand floors the fixed 270-block box before its radius-5 gate");
    check(zonedemand::hostWindow({-65536 / 2, -64LL * 65536 - 65536 / 2, 0}).center ==
              zonedemand::BlockPoint{0, -1} &&
              zonedemand::hostWindow({(1LL << 48) + 64LL * 65536, 0, 0}).center[0] == 1,
          "host demand truncates negative fractions and narrows blocks before its tile conversion");
    std::set<zonedemand::BlockPoint> hostUnion;
    auto addHost = [&](const zonedemand::Window &window) {
        for (int x = window.minimum[0]; x <= window.maximum[0]; ++x)
            for (int y = window.minimum[1]; y <= window.maximum[1]; ++y)
                if (window.contains(x, y))
                    hostUnion.insert({x, y});
    };
    addHost(hostEdge);
    addHost(hostEdge);
    check(hostUnion.size() == 79, "multiple colocated players do not duplicate host demand");
    addHost(zonedemand::hostWindow({64LL * 65536, 0, 0}));
    check(hostUnion.size() == 89, "adjacent player host windows form the native overlapping union");
    const zonedemand::BlockPoint destination{6400, -64};
    const auto sourceFlight = zonedemand::eagleAnchor(0x9c, destination);
    const auto inFlight = zonedemand::clientWindow({0, 0, 0}, 50, sourceFlight);
    check(zonedemand::hostWindow({0, 0, 0}, sourceFlight).center == zonedemand::BlockPoint{100, -1},
          "host source demand selects the same active eagle block destination");
    check(sourceFlight && inFlight.center == zonedemand::BlockPoint{100, -1} &&
              zonedemand::preparationCenter({0, 0, 0}, sourceFlight) == zonedemand::BlockPoint{100, -1},
          "eagle source and mesh preparation use the raw signed block destination");
    check(zonedemand::preparationCenter({0, 0, 0}) == zonedemand::BlockPoint{0, 0},
          "physical retention center remains independent of the eagle destination");
    check(!zonedemand::eagleAnchor(0x9d, destination) && !zonedemand::eagleAnchor(0x78, destination),
          "title eagle and arrival glider ignore retained destination fields");
    check(zonedemand::eagleAnchor(0x9c, {INT32_MAX, INT32_MAX}).has_value(),
          "native eagle substitution does not invent an age or sentinel repair gate");
    const auto afterArrival = zonedemand::clientWindow({0, 0, 0}, 50, zonedemand::eagleAnchor(0x78, destination));
    check(afterArrival.center == zonedemand::BlockPoint{0, 0}, "arrival returns source demand to the physical position");
    PreparationBudget preparation(15);
    check(preparation.consider(14155776) && preparation.consider(0),
          "preparation retains a zero-cost candidate at the exact native budget");
    check(preparation.consider(1) && !preparation.consider(0),
          "preparation rejects a new zero-cost candidate only strictly above budget");
    check(preparation.consider(100) && !preparation.consider(0),
          "known mesh cost remains admitted and counted above the preparation budget");
    check(preparation.consider(-101) && preparation.consider(0),
          "preparation accumulates the signed cached cost before later candidates");
    PreparationBudget minimumPreparation(3);
    check(minimumPreparation.consider(566231) && minimumPreparation.consider(0) &&
              minimumPreparation.consider(1) && !minimumPreparation.consider(0),
          "minimum-window preparation keeps the separately rounded float budget");
    {
        Frame narrowed;
        narrowed.zoneRadius = 3;
        std::vector<std::pair<int64_t, int64_t>> visited;
        narrowed.zone = [&](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
            visited.emplace_back(x, y);
            scenevisibility::Bounds bounds;
            bounds.meshed = true;
            bounds.triangleCost = 0;
            return bounds;
        };
        narrowed.camera = {(1LL << 48) + 64LL * 65536, 0, 0};
        candidate(narrowed);
        check(!visited.empty() && visited.front() == std::pair<int64_t, int64_t>{-2, -3},
              "camera window narrows positive blocks before selecting its native zone");
        visited.clear();
        narrowed.camera[0] = -(1LL << 48);
        candidate(narrowed);
        check(!visited.empty() && visited.front() == std::pair<int64_t, int64_t>{-4, -3},
              "camera negative reconstruction follows narrowing even at a wrapped exact block");
    }
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
    check(groundColour({-65535, -65535, 0}) == groundColour({}), "flag colour truncates negative physical XY toward zero");
    Frame metadataFrame = frame;
    metadataFrame.zone = {};
    State fallback = ground;
    const float retained = fallback.value().scene;
    check(!fallback.update(metadataFrame) && fallback.flaggedColour() == groundColour({}) / 255.f && fallback.value().scene == retained,
          "physical-block sky target remains available when mesh-cost metadata is unavailable");
    metadataFrame.player.blockFlags = 0;
    check(!fallback.update(metadataFrame) && fallback.flaggedColour() == glm::vec3(0) && fallback.value().scene == retained,
          "unavailable range does not retain the previous frame's interior sky target");
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
