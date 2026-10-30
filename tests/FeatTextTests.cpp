#include "FeatText.h"
#include <cmath>
#include <cstdio>
namespace {
int failures = 0;
#define FEAT_CHECK(...)                                                                                                                    \
    do {                                                                                                                                   \
        if (!(__VA_ARGS__)) {                                                                                                              \
            std::printf("FAIL feat text line%d: %s\n", __LINE__, #__VA_ARGS__);                                                            \
            ++failures;                                                                                                                    \
        }                                                                                                                                  \
    } while (0)
bool near(float a, float b) {
    return std::abs(a - b) < .00001f;
}
} // namespace
int featTextTests() {
    feattext::Store store;
    scenevisibility::Projection projection;
    projection.offset = {65536, 0, 99 * 65536}; // Z is not origin-rebased.
    feattext::Point physical{0, 0, 65536};
    store.received(physical);
    physical[0] = 100 * 65536; // A receipt snapshots its point rather than following the player.
    auto labels = store.visit(projection, {128, 128}, 1500);
    FEAT_CHECK(labels.size() == 1 && near(labels[0].at.x, 133) && near(labels[0].at.y, -36));
    FEAT_CHECK(near(labels[0].fill.a, 1) && near(labels[0].fill.r, .7f) && near(labels[0].outline.r, .07f));
    labels = store.visit(projection, {128, 128}, 1500);
    FEAT_CHECK(labels.size() == 1 && near(labels[0].at.y, -86) && near(labels[0].fill.a, .5f));
    FEAT_CHECK(store.size() == 1); // equality remains until a strictly later age.
    labels = store.visit(projection, {128, 128}, 0);
    FEAT_CHECK(labels.size() == 1 && near(labels[0].at.y, -136) && labels[0].fill.a == 0);
    labels = store.visit(projection, {128, 128}, 1);
    FEAT_CHECK(labels.size() == 1 && store.size() == 0); // draw old age, then remove.
    store.received({0, 0, -65536});
    FEAT_CHECK(store.visit(projection, {128, 128}, 3001).empty() && store.size() == 0);
    store.received({0, 0, 65536});
    FEAT_CHECK(store.visit({}, {128, 128}, 3001).empty() && store.size() == 0);
    store.received({100 * 65536, 0, 65536});
    labels = store.visit(projection, {128, 128}, 0);
    FEAT_CHECK(labels.size() == 1 && labels[0].at.x > 128); // no screen-edge or distance gate.
    feattext::Store odd;
    odd.received({0, 0, 65536});
    labels = odd.visit(scenevisibility::Projection{}, {801, 601}, 0);
    FEAT_CHECK(labels.size() == 1 && labels[0].at.x == 405.5f && labels[0].at.y == 200.5f);
    std::printf("Feat text projection/lifetime: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
