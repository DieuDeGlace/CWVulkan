// Mathematical checks derived from steam-gui-item-icons.md; no game asset is distributed.
#include "GuiItem.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define GUI_CHECK(...)                                                                                                                     \
    do {                                                                                                                                   \
        if (!(__VA_ARGS__)) {                                                                                                              \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                                                             \
            failures++;                                                                                                                    \
        }                                                                                                                                  \
    } while (0)
bool near(float a, float b) {
    return std::abs(a - b) < 1e-6f;
}
} // namespace
int guiItemTests() {
    using namespace guiitem;
    failures = 0;
    GUI_CHECK(near(phase(-1), -0.036f) && phase(10000) == 0 && near(phase(2500), 90));
    Placement at;
    at.screen = {800, 512};
    at.size = 0.1f;
    const auto one = geometry(3, {1, 1, 1}, {1600, 1024}, at);
    GUI_CHECK(one && near(one->model[0][0], 0.02f)); // small-Z increases divisor1 to5
    GUI_CHECK(one && one->projection[0][0] < 0 && one->projection[2][3] == 1);
    GUI_CHECK(one && std::bit_cast<uint32_t>(one->projection[2][2]) == 0x3f800347u);
    GUI_CHECK(one && std::bit_cast<uint32_t>(one->projection[3][2]) == 0xbdccd20bu);
    if (one) {
        const glm::vec4 centre = one->model * glm::vec4(0.5f, 0.5f, 0.5f, 1);
        GUI_CHECK(near(centre.x, 0) && near(centre.y, 0) && near(centre.z, 1));
        const auto projected = one->projection * centre;
        GUI_CHECK(near(projected.x / projected.w, 0) && near(projected.y / projected.w, 0));
    }
    const auto armour = geometry(4, {1, 1, 1}, {1600, 1024}, at);
    GUI_CHECK(armour && near(armour->model[0][0], 0.1f / 7.0f)); // trunc(5*1.5), not7.5
    const auto flat = geometry(3, {10, 20, 3}, {1600, 1024}, at);
    const auto solid = geometry(3, {10, 20, 4}, {1600, 1024}, at);
    GUI_CHECK(flat && solid && near(flat->model[0][0], 0.1f / 24) && near(solid->model[0][0], 0.1f / 20));
    at.screen = {400, 256};
    at.screenDepth = 0.2f;
    at.darkPass = true;
    const auto shifted = geometry(3, {1, 1, 1}, {1600, 1024}, at);
    if (shifted) {
        const auto p = shifted->projection * shifted->model * glm::vec4(0.5f, 0.5f, 0.5f, 1);
        GUI_CHECK(near(p.x / p.w, -0.5f) && near(p.y / p.w, 0.5f));
        GUI_CHECK(near(p.z / p.w, 1.10009f));
        GUI_CHECK(near(shifted->darkView[3][0], -0.01f) && near(shifted->darkView[3][1], -0.01f));
        GUI_CHECK(shifted->depthOffset == 0.1f && one && shifted->model == one->model);
    }
    at.darkPass = false;
    const auto ring = geometry(9, {2, 2, 2}, {1600, 1024}, at);
    GUI_CHECK(ring && near(ring->model[0][0], 0) && near(ring->model[0][1], 0) && near(ring->model[0][2], -0.1f / 6));
    GUI_CHECK(!geometry(3, {0, 1, 1}, {1600, 1024}, at) && !geometry(3, {1, 1, 1}, {1600, 0}, at));
    auto item = nativesaves::makeItem(0, 0, 1);
    GUI_CHECK(shininess(item) == 0);
    for (int material : {1, 11, 12, 13, 23})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 1);
    for (int material : {4, 29})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 0.5f);
    for (int material : {33, 35})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 0.25f);
    GUI_CHECK(shininess(nativesaves::makeItem(15, 0)) == 0 && shininess(nativesaves::makeItem(23, 0)) == 1);
    GUI_CHECK(shininess(nativesaves::makeItem(24, 18)) == 1 && shininess(nativesaves::makeItem(24, 19)) == 0);
    glm::mat4 basis;
    basis[0] = {2, 3, 4, 5};
    basis[1] = {6, 7, 8, 9};
    basis[2] = {10, 11, 12, 13};
    basis[3] = {14, 15, 16, 17};
    auto weapon = nativesaves::makeItem(3, 0);
    weapon[nativesaves::item::ATTRIBUTE_COUNT] = 2;
    weapon[0x1d] = uint8_t(-2);
    weapon[0x1e] = 3;
    weapon[0x1f] = uint8_t(-4);
    weapon[0x20] = 1;
    weapon[0x21] = 1;
    const auto cubes = spiritCubes(weapon, basis);
    GUI_CHECK(cubes.size() == 2 && cubes[0].model[3] == glm::vec4(-12, -14, -16, -18));
    GUI_CHECK(cubes.size() == 2 && cubes[1].model[3] == glm::vec4(16, 18, 20, 22));
    GUI_CHECK(cubes.size() == 2 && cubes[0].model[0] == basis[0] && cubes[0].model[2] == basis[2]);
    weapon[nativesaves::item::TYPE] = 15;
    GUI_CHECK(spiritCubes(weapon, basis).empty());
    weapon[nativesaves::item::TYPE] = 3;
    weapon[nativesaves::item::ATTRIBUTE_COUNT] = 1;
    weapon[0x1d] = weapon[0x1e] = weapon[0x1f] = 1;
    basis[0].x = 1e10f;
    basis[1].x = -1e10f;
    basis[2].x = 1;
    basis[3].x = 1;
    const auto grouped = spiritCubes(weapon, basis);
    GUI_CHECK(grouped.size() == 1 && grouped[0].model[3].x == 2); // XYZ sum before original translation
    Request request;
    request.mesh = 7;
    request.cubeMesh = 8;
    request.allocation = {1, 1, 1};
    request.item = weapon;
    request.at = at;
    request.at.darkPass = true;
    auto state = overlayState();
    state.white = 0.4f;
    state.glow = 0.25f;
    state.shininess = 0.5f;
    std::vector<Draw> draws;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == 3 && draws[0].state.white == 0.4f && draws[0].state.glow == 0.25f && draws[0].state.shininess == 0.5f);
    GUI_CHECK(draws.size() == 3 && draws[0].material == glm::vec4(0, 0, 0, 1) && draws[0].depthOffset == 0);
    GUI_CHECK(draws.size() == 3 && draws[1].state.white == 0 && draws[1].state.glow == 0 && draws[1].depthOffset == 0.1f);
    GUI_CHECK(draws.size() == 3 && draws[2].mesh == 8 && draws[2].depthOffset == draws[1].depthOffset);
    const auto count = draws.size();
    request.mesh = 0;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == count);
    const auto light = lightDirection();
    GUI_CHECK(near(glm::length(light), 1) && near(light.y / light.x, 0.4f) && near(light.z, -light.x));
    std::printf("GUI item transforms: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
