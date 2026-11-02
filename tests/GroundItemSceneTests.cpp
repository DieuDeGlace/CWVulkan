#include "GroundItemScene.h"
#include "GuiItem.h"
#include <cmath>
#include <cstdio>
#include <cstring>

int groundItemSceneTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL ground-item scene: %s\n", message);
        }
    };
    nativesaves::Item item{};
    item[0] = 3;
    groundscene::Input input;
    input.position = {11, -22, 33};
    input.dimensions = {14, 100, 6};
    input.originalCategory = 3;
    input.delayMs = 250;
    const auto hop = groundscene::prepare(input, item);
    check(hop.position[2] == 33 + 5 * 65536 && hop.sphere.centre == input.position && hop.sphere.radius == 1,
          "raw sphere ignores hop and voxel Y while the fixed anchor rises five blocks");
    input.delayMs = 750;
    check(groundscene::prepare(input, item).position[2] == 33 - 15 * 65536, "positive long delays can hop below the raw ground point");
    input.delayMs = 0;
    const auto still = groundscene::prepare(input, item);
    check(std::abs(still.model[3].x + 3.f / 14.f) < 1e-6f && std::abs(still.model[3].y + 50.f / 14.f) < 1e-6f &&
              std::abs(still.model[3].z - 1.f) < 1e-6f,
          "sideways equipment uses scaled local centering after its ninety-degree rotation");
    input.delayMs = -100;
    const auto negative = groundscene::prepare(input, item);
    check(negative.position == input.position && std::memcmp(&still.model, &negative.model, sizeof still.model) != 0,
          "negative delay has no hop but retains the two-axis tumble");
    input.delayMs = 0;
    input.light = 128;
    input.originalCategory = item[0] = 23;
    auto pulse = groundscene::prepare(input, item);
    check(pulse.material == glm::vec4(2, 2, 2, (128.f / 255.f) * 2), "clock-zero pulse multiplies all RGBA, including light alpha");
    input.worldMs = 450;
    const auto low = groundscene::prepare(input, item);
    check(std::abs(low.material.r - 1) < 1e-6f && low.model[3].z == pulse.model[3].z,
          "cosine reaches its material minimum without changing vertical placement");
    input.selected = true;
    const auto selected = groundscene::prepare(input, item);
    check(selected.material == low.material + glm::vec4(.25f) && selected.opacityScale == 1,
          "selection adds to alpha as well as RGB without changing pixel opacity");
    input.selected = false;
    input.light = 0;
    input.originalCategory = item[0] = 11;
    int32_t kind = 14;
    std::memcpy(item.data() + 4, &kind, 4);
    check(groundscene::prepare(input, item).material.a == 1, "special category11 subtype14 forces light alpha1");
    kind = 36;
    std::memcpy(item.data() + 4, &kind, 4);
    check(groundscene::itemScale(item) == .05f, "subtype36 scale overrides the ordinary item scale");
    kind = 37;
    std::memcpy(item.data() + 4, &kind, 4);
    check(groundscene::prepare(input, item).opacityScale == .6000000238418579f,
          "translucent subtype retains its separate opacity multiplier");
    item = {};
    item[0] = 3;
    item[0x9d] = 1;
    item[0x20] = 0x80;
    const auto spirit = guiitem::spiritCubes(item, glm::mat4(1), {2, 3, 4, 1.75f});
    check(spirit.size() == 1 && spirit[0].material == glm::vec4(1, .5f, .1f, 1.75f),
          "spectral spirit RGB is independent of the pulsed tint while its alpha retains the supplied material");
    std::printf("Native ground-item scene: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
