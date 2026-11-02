// Checks of the pose state the world's draw sets (creature-pose.md §12), the weapon spirits
// (creature-pose-items.md §9.1) and the HUD's turning coin (hud-money.md), and the
// `posestateoracle` command that holds the state rule against the game's pose recordings.
#include "HudCoin.h"
#include "HudCompass.h"
#include "NativePose.h"
#include "worldgen/Residents.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {
int failures = 0;
#define POSE_CHECK(...)                                                                            \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

constexpr size_t ABILITY = 0x68, ABILITY_CLOCK = 0x6c, LOCK_TIMER = 0x134, HEALTH = 0x180;
constexpr size_t LEFT_ITEM = 0x638, RIGHT_ITEM = 0x6d8, ITEM_IN_USE = 0x1d8;
constexpr size_t STATE_CLOCK = nativepose::ANIM + 0x08, STATE = nativepose::ANIM + 0x0c;
// The eased pose fields (A+0x20 .. A+0xd3).
constexpr size_t POSE_FIELDS = nativepose::ANIM + 0x20, POSE_FIELDS_END = nativepose::ANIM + 0xd4;

template <class T> void put(std::vector<uint8_t> &r, size_t at, T v) { std::memcpy(&r[at], &v, sizeof v); }
template <class T> T get(const std::vector<uint8_t> &r, size_t at) {
    T v;
    std::memcpy(&v, &r[at], sizeof v);
    return v;
}
void setItem(std::vector<uint8_t> &r, size_t slot, uint8_t category, uint32_t kind) {
    r[slot] = category;
    put<uint32_t>(r, slot + 4, kind);
}

// §12.2: the fixed table, the three item-dependent abilities and the lock override.
void stateRule() {
    std::vector<uint8_t> c(nativepose::CREATURE_SIZE, 0);
    auto stateOf = [&](int ability) {
        c[ABILITY] = (uint8_t)ability;
        return nativepose::poseStateFor(c.data());
    };
    POSE_CHECK(stateOf(0) == 0 && stateOf(1) == 0 && stateOf(0x1e) == 0 && stateOf(0xa4) == 0 && stateOf(0xff) == 0);
    POSE_CHECK(stateOf(0x02) == 0x10 && stateOf(0x03) == 0x0f && stateOf(0x13) == 0x11 && stateOf(0x3a) == 0x02);
    POSE_CHECK(stateOf(0x98) == 0x61 && stateOf(0xa3) == 0x66 && stateOf(0x6e) == 0x08 && stateOf(0x9e) == 0x1d);
    // 0x32: the right weapon of kind 6 or 7, else the left weapon, else none.
    POSE_CHECK(stateOf(0x32) == 0x23);
    setItem(c, LEFT_ITEM, 3, 0xd);
    POSE_CHECK(stateOf(0x32) == 0x09);
    setItem(c, LEFT_ITEM, 3, 0);
    POSE_CHECK(stateOf(0x32) == 0x24);
    setItem(c, RIGHT_ITEM, 3, 6);
    POSE_CHECK(stateOf(0x32) == 0x16);
    setItem(c, RIGHT_ITEM, 3, 5);
    POSE_CHECK(stateOf(0x32) == 0x24);
    // 0x67: anything in the left hand.
    POSE_CHECK(stateOf(0x67) == 0x24);
    setItem(c, LEFT_ITEM, 0, 0);
    POSE_CHECK(stateOf(0x67) == 0x32);
    // 0x89: the vehicle kinds of the item in use.
    POSE_CHECK(stateOf(0x89) == 0x4e);
    const int kinds[] = {4, 5, 6, 7, 8, 0xa, 0xb};
    for (int k = 0; k < 7; k++) {
        setItem(c, ITEM_IN_USE, 0x18, kinds[k]);
        POSE_CHECK(stateOf(0x89) == 0x5a + k);
    }
    setItem(c, ITEM_IN_USE, 0x18, 9);
    POSE_CHECK(stateOf(0x89) == 0x4e);
    // The lock timer wins over every ability; a NaN counts as running.
    put<float>(c, LOCK_TIMER, 250.0f);
    POSE_CHECK(stateOf(0x02) == 0x3d && stateOf(0) == 0x3d);
    put<float>(c, LOCK_TIMER, std::nanf(""));
    POSE_CHECK(stateOf(0x02) == 0x3d);
    put<float>(c, LOCK_TIMER, 0.0f);
    // The clock is copied bit for bit, the state written whole.
    c[ABILITY] = 0x02;
    put<uint32_t>(c, ABILITY_CLOCK, 0x7fc00001u);
    put<int32_t>(c, STATE, -1);
    nativepose::setPoseState(c.data());
    POSE_CHECK(get<uint32_t>(c, STATE_CLOCK) == 0x7fc00001u && get<int32_t>(c, STATE) == 0x10);
}

// The bug this fixes: with nothing setting the state, every creature posed as state 0 however
// far into an attack it was. A creature swinging (ability 2) through a stretch of frames must
// change its pose from frame to frame, and two moments of the same attack must differ.
void swingMoves() {
    std::vector<uint8_t> c = worldgen::newCreature(7);
    put<float>(c, HEALTH, 100.0f);
    setItem(c, RIGHT_ITEM, 3, 0); // a sword in the right hand
    c[ABILITY] = 0x02;
    nativepose::Models none;
    nativepose::Call call;
    call.dt = 16.0f;
    std::vector<std::vector<uint8_t>> frames;
    for (float t = 0; t <= 800.0f; t += 16.0f) {
        put<float>(c, ABILITY_CLOCK, t);
        nativepose::setPoseState(c.data());
        POSE_CHECK(get<int32_t>(c, STATE) == 0x10 && get<float>(c, STATE_CLOCK) == t);
        nativepose::pose(c.data(), call, none, nullptr);
        frames.emplace_back(c.begin() + POSE_FIELDS, c.begin() + POSE_FIELDS_END);
    }
    size_t changed = 0;
    for (size_t i = 1; i < frames.size(); i++)
        changed += frames[i] != frames[i - 1];
    std::set<std::vector<uint8_t>> distinct(frames.begin(), frames.end());
    POSE_CHECK(changed + 1 >= frames.size() / 2);
    POSE_CHECK(distinct.size() >= 10);
    // The same frames without the state set (the old behaviour): the pose settles and stops
    // following the attack.
    std::vector<uint8_t> frozen = worldgen::newCreature(7);
    put<float>(frozen, HEALTH, 100.0f);
    setItem(frozen, RIGHT_ITEM, 3, 0);
    frozen[ABILITY] = 0x02;
    std::vector<uint8_t> last, settled;
    for (int i = 0; i < 200; i++) {
        put<float>(frozen, ABILITY_CLOCK, (float)(i * 16 % 800));
        nativepose::pose(frozen.data(), call, none, nullptr);
        last.assign(frozen.begin() + POSE_FIELDS, frozen.begin() + POSE_FIELDS_END);
        if (i == 190)
            settled = last;
    }
    POSE_CHECK(last == settled); // nothing moves any more
    POSE_CHECK(frames.back() != last);
}

// §9.1: one cube per spirit at the weapon matrix moved to the spirit's voxel, coloured by its
// material; none without spirits.
void weaponSpirits() {
    uint8_t item[0xa0] = {};
    nativepose::Model cube;
    cube.size[0] = cube.size[1] = cube.size[2] = 1;
    nativepose::Draw effect;
    effect.kind = nativepose::WEAPON_EFFECT_DRAW;
    effect.model = &cube;
    effect.item = item;
    // A weapon matrix: turned a quarter about z, scaled by 2, at (10, 20, 30).
    const float M[16] = {0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 2, 0, 10, 20, 30, 1};
    std::memcpy(effect.matrix, M, sizeof M);
    const float tint[4] = {0.5f, 1.0f, 1.0f, 0.75f};
    std::memcpy(effect.tint, tint, sizeof tint);
    effect.opacity = 1, effect.glow = 0, effect.shininess = 0.25f, effect.white = 0, effect.depthOffset = 3;
    std::vector<nativepose::Draw> out;
    nativepose::spiritDraws(effect, out);
    POSE_CHECK(out.empty());
    item[0x9d] = 2;
    const uint8_t spirits[8] = {1, 2, 3, 0x80, 0xff, 0, 0, 2};
    std::memcpy(item + 0x1d, spirits, sizeof spirits);
    nativepose::spiritDraws(effect, out);
    POSE_CHECK(out.size() == 2);
    if (out.size() == 2) {
        const nativepose::Draw &a = out[0], &b = out[1];
        POSE_CHECK(a.kind == nativepose::PLAIN_DRAW && a.model == &cube && a.shininess == 0.25f && a.depthOffset == 3);
        // (1, 2, 3) through the turned, scaled frame: x 10 - 4, y 20 + 2, z 30 + 6.
        POSE_CHECK(a.matrix[12] == 6 && a.matrix[13] == 22 && a.matrix[14] == 36 && a.matrix[15] == 1);
        POSE_CHECK(std::memcmp(a.matrix, M, 12 * sizeof(float)) == 0);
        // Material 0x80: its own orange, only the tint's alpha.
        POSE_CHECK(a.tint[0] == 1.0f && a.tint[1] == 0.5f && a.tint[2] == 0.1f && a.tint[3] == 0.75f);
        // (-1, 0, 0), material 2: (0.4, 0.3, 0.2, 1) times the tint.
        POSE_CHECK(b.matrix[12] == 10 && b.matrix[13] == 18 && b.matrix[14] == 30);
        POSE_CHECK(b.tint[0] == 0.4f * 0.5f && b.tint[1] == 0.3f && b.tint[2] == 0.2f && b.tint[3] == 0.75f);
    }
    // Unlisted materials are a tinted grey; the spirit materials brighten with the strength.
    const auto grey = nativepose::materialColour(0x42, tint);
    POSE_CHECK(grey[0] == 0.25f && grey[1] == 0.5f && grey[3] == 0.75f);
    const auto strong = nativepose::materialColour(0x82, tint, 1.0f);
    POSE_CHECK(strong[2] == 1.5f && strong[3] == (1.0f + 0.75f) * 1.5f);
    // A draw that is not a weapon effect stands for nothing.
    out.clear();
    effect.kind = nativepose::PLAIN_DRAW;
    nativepose::spiritDraws(effect, out);
    POSE_CHECK(out.empty());
}

// hud-money.md §2 and §3.
void hudCoin() {
    POSE_CHECK(hudcoin::turnDegrees(0) == 0.f);
    POSE_CHECK(std::fabs(hudcoin::turnDegrees(2500) - 90.f) < 1e-4f);
    POSE_CHECK(hudcoin::turnDegrees(10000) == 0.f);
    POSE_CHECK(std::fabs(hudcoin::turnDegrees(12500.9f) - 90.f) < 1e-4f);
    POSE_CHECK(std::fabs(hudcoin::turnDegrees(-2500.9f) + 90.f) < 1e-4f);
    POSE_CHECK(std::fabs(hudcoin::turnDegrees(4294967296.f) + 131.328f) < 1e-4f);
    POSE_CHECK(std::fabs(hudcoin::turnDegrees(float(16777215.75)) - 259.776f) < 1e-4f); // native float clock boundary
    const float W = 1280, H = 1024, g = 1.25f;
    const glm::ivec3 allocation(8, 8, 2);
    const auto requests = hudcoin::coinRequests(17, allocation, 960, g, H, 0);
    POSE_CHECK(requests.size() == 2);
    if (requests.size() != 2)
        return;
    POSE_CHECK(hudcoin::coinRequests(0, allocation, 960, g, H, 0).empty());
    POSE_CHECK(hudcoin::coinRequests(17, allocation, 960, g, 0, 0).empty());
    POSE_CHECK(hudcoin::coinRequests(17, {}, 960, g, H, 0).empty());
    const auto &shadow = requests[0], &coin = requests[1];
    POSE_CHECK(shadow.material == glm::vec4(0, 0, 0, 1) && shadow.depthOffset == -.5f);
    POSE_CHECK(shadow.at.screen == glm::vec2(62.5f, 71.25f));
    POSE_CHECK(coin.material == glm::vec4(1) && coin.depthOffset == 0 && coin.at.screen == glm::vec2(56.25f, 65));
    POSE_CHECK(coin.kind == guiitem::Request::Kind::QueuedModel && coin.mesh == shadow.mesh && coin.mesh == 17);
    POSE_CHECK(coin.beforeVertex == 0 && shadow.beforeVertex == 0 && coin.farPlane == 960);
    POSE_CHECK(coin.at.euler == glm::vec3(-30, 0, 0) && std::fabs(coin.at.size - g * .004f) < 1e-7f);
    auto retained = guiitem::characterState(); // earlier character setup must not leak into the coin
    std::vector<guiitem::Draw> draws;
    for (const auto &request : requests)
        guiitem::appendDraws(request, {int(W), int(H)}, retained, draws);
    POSE_CHECK(draws.size() == 2);
    if (draws.size() != 2)
        return;
    for (const auto &draw : draws) {
        POSE_CHECK(draw.state.glow == 0 && draw.state.white == 0 && draw.state.shininess == 1 && !draw.state.whiteSky);
        POSE_CHECK(draw.state.front == glm::vec4(.7f, .7f, .2f, 1) && draw.opacity == 1);
    }
    auto pixel = [&](const guiitem::Draw &draw, glm::vec3 voxel) {
        auto clip = draw.projection * draw.model * glm::vec4(voxel, 1);
        clip.z -= draw.depthOffset;
        return glm::vec3((clip.x / clip.w + 1) * .5f * W, (1 - clip.y / clip.w) * .5f * H, clip.z / clip.w);
    };
    const glm::vec3 middle = pixel(draws[1], glm::vec3(allocation) * .5f);
    const glm::vec3 shadowMiddle = pixel(draws[0], glm::vec3(allocation) * .5f);
    POSE_CHECK(std::fabs(middle.x - 56.25f) < 1e-3f && std::fabs(middle.y - 65.f) < 1e-3f);
    POSE_CHECK(middle.z > 0 && middle.z < shadowMiddle.z && shadowMiddle.z < 1);
    const auto left = pixel(draws[1], {0, 4, 1}), right = pixel(draws[1], {8, 4, 1});
    POSE_CHECK(std::fabs((left.x - right.x) - 35.13f * g) < .05f * g);
    const auto top = pixel(draws[1], {4, 8, 1}), bottom = pixel(draws[1], {4, 0, 1});
    POSE_CHECK(std::fabs(top.y - bottom.y) < 35.13f * g && std::fabs(top.y - bottom.y) > 25.f * g);
    const auto smallerWindow = hudcoin::coinRequests(17, allocation, 1280, g, H * .5f, 2500);
    POSE_CHECK(smallerWindow[1].at.size == coin.at.size * 2 && smallerWindow[1].farPlane == 1280);
    POSE_CHECK(smallerWindow[1].at.euler.y == 90);
    const auto compass = hudcompass::request(19, {29, 29, 15}, {128, 128}, 1280, 1, 725.25f);
    POSE_CHECK(compass.has_value());
    if (compass) {
        POSE_CHECK(compass->at.screen == glm::vec2(68, 20) && compass->at.euler == glm::vec3(-120, 0, -725.25f));
        POSE_CHECK(std::fabs(compass->at.size - .024f) < 1e-7f && compass->farPlane == 1280);
        POSE_CHECK(compass->state && compass->state->glow == 1 && compass->state->shininess == 0);
        POSE_CHECK(compass->material == glm::vec4(1) && compass->opacity == 1 && compass->depthOffset == 0);
        POSE_CHECK(compass->allocation == glm::ivec3(29, 29, 15) && compass->beforeVertex == 0);
    }
    const auto positiveZero = hudcompass::request(19, {1, 1, 1}, {128, 128}, 960, 1, 0.f);
    const auto negativeZero = hudcompass::request(19, {1, 1, 1}, {128, 128}, 960, 1, -0.f);
    POSE_CHECK(positiveZero && std::signbit(positiveZero->at.euler.z));
    POSE_CHECK(negativeZero && !std::signbit(negativeZero->at.euler.z));
    POSE_CHECK(!hudcompass::request(0, {1, 1, 1}, {128, 128}, 960, 1, 0));
    POSE_CHECK(!hudcompass::request(19, {}, {128, 128}, 960, 1, 0));
}

// The creature bytes of each record of a pose recording (creature-pose.md §11): a record is
// its length, the creature's address (8 bytes), then the creature as the pose call found it.
std::vector<std::vector<uint8_t>> recordedCreatures(const std::string &path) {
    std::vector<std::vector<uint8_t>> out;
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || (std::memcmp(b.data(), "CWA2", 4) && std::memcmp(b.data(), "CWA3", 4)))
        return out;
    size_t at = 4;
    while (at + 4 <= b.size()) {
        uint32_t n;
        std::memcpy(&n, &b[at], 4);
        at += 4;
        if (at + n > b.size() || n < 8 + nativepose::CREATURE_SIZE)
            break;
        out.emplace_back(b.begin() + at + 8, b.begin() + at + 8 + nativepose::CREATURE_SIZE);
        at += n;
    }
    return out;
}

} // namespace

int poseHudTests() {
    failures = 0;
    stateRule();
    swingMoves();
    weaponSpirits();
    hudCoin();
    return failures;
}

// posestateoracle <files...>: every recorded pose call found the state and clock the world's
// draw had just set; the rule must give the same from the rest of the creature.
int poseStateOracle(const std::vector<std::string> &files) {
    size_t records = 0, sameState = 0, both = 0;
    int shown = 0;
    for (const std::string &file : files)
        for (std::vector<uint8_t> &c : recordedCreatures(file)) {
            records++;
            const std::vector<uint8_t> game = c;
            put<uint32_t>(c, STATE_CLOCK, 0xdeadbeefu); // so nothing is left over
            put<int32_t>(c, STATE, -12345);
            nativepose::setPoseState(c.data());
            const bool state = get<int32_t>(c, STATE) == get<int32_t>(game, STATE);
            const bool clock = get<uint32_t>(c, STATE_CLOCK) == get<uint32_t>(game, STATE_CLOCK);
            sameState += state, both += state && clock;
            if (!(state && clock) && shown++ < 10)
                std::printf("%s: ability 0x%x clock %.9g lock %.9g: ours state 0x%x clock %.9g, game state 0x%x clock %.9g\n",
                            std::filesystem::path(file).filename().string().c_str(), game[ABILITY], get<float>(game, ABILITY_CLOCK),
                            get<float>(game, LOCK_TIMER), get<int32_t>(c, STATE), get<float>(c, STATE_CLOCK),
                            get<int32_t>(game, STATE), get<float>(game, STATE_CLOCK));
        }
    std::printf("pose state oracle: %zu records, state the same in %zu, state and clock bit for bit in %zu\n", records, sameState,
                both);
    return records && both == records ? 0 : 1;
}
