#include "WorldModelControls.h"
#include "CubModel.h"
#include "AlphaLook.h"
#include "NativePose.h"
#include "Renderer.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <limits>
#include <cmath>

int worldModelControlsTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL WORLD model controls: %s\n", message);
        }
    };
    using worldmodel::Queue;
    const std::array<worldmodel::ResourceOrder, 6> resources{
        {{UINT64_MAX, false}, {7, true}, {7, false}, {UINT64_MAX, false}, {3, false}, {1, true}}};
    check(worldmodel::opaqueOrder(resources) == std::vector<size_t>{4, 2, 0, 3, 5, 1},
          "normal resource groups precede opposite groups; unsigned address order retains per-resource FIFO");
    check(worldmodel::lightBlock({65535, -1, 6553}) == std::array<int32_t, 3>{0, -1, 0} && worldmodel::lightBlock({0, 0, 6552})[2] == -1 &&
              worldmodel::lightBlock({0, 0, std::numeric_limits<int64_t>::min()})[2] == -1,
          "physical light uses the truncated6553 offset and low64/narrowed negative conversion");
    const glm::vec4 frameMaterial{.5f, .5f, 1, .8f};
    auto tinted = worldmodel::baseMaterial(frameMaterial, 128, 0);
    check(tinted == glm::vec4(.5f, .5f, 1, (128.f / 255.f) * .8f) && worldmodel::baseMaterial(frameMaterial, 0, 0x114) == frameMaterial,
          "physical light affects material alpha while race114 overrides the returned light factor");
    tinted *= worldmodel::buffFactor(1);
    tinted *= worldmodel::buffFactor(1);
    tinted *= worldmodel::buffFactor(4);
    check(glm::vec3(tinted) == glm::vec3(.25f, .1875f, .125f) && tinted.a == (128.f / 255.f) * .8f,
          "repeated buff factors stack without reducing the list to a set");
    worldmodel::Frame frame;
    frame.actorRange = 100;
    check(std::abs(worldmodel::opacity(frame, {95 * 65536, 0, 0}, .25f, true, 0x11d) - .15f) < 1e-5f &&
              worldmodel::opacity(frame, {}, 2, false, 0) < 0,
          "raw-drawn distance fade and visibility/halving/race factors stay separate from material without a clamp");
    int markerEntries = 0;
    for (int id = 0; id < 0x1000; ++id)
        markerEntries += worldmodel::hidesAttachmentMarkers(id);
    check(markerEntries == 105 && worldmodel::hidesAttachmentMarkers(0x5b3) && !worldmodel::hidesAttachmentMarkers(0x5c2) &&
              !worldmodel::hidesAttachmentMarkers(0xca7),
          "verified playable hair has its own load policy without classifying mammoth hair by name");
    check(worldmodel::queue(0) == Queue::Hidden && worldmodel::queue(-1) == Queue::Hidden && worldmodel::queue(.5f) == Queue::Faded &&
              worldmodel::queue(1) == Queue::Opaque && worldmodel::queue(2) == Queue::Opaque,
          "finite opacity selects its own descriptor queue");
    check(worldmodel::queue(std::numeric_limits<float>::quiet_NaN()) == Queue::Hidden &&
              worldmodel::queue(std::numeric_limits<float>::infinity()) == Queue::Hidden,
          "invalid engine opacity is rejected outside the finite native contract");
    nativepose::Draw pose{};
    pose.tint[0] = .1234567f;
    pose.tint[1] = 1.25f;
    pose.tint[2] = .37f;
    pose.tint[3] = .17f;
    pose.opacity = 1;
    pose.glow = .2f;
    pose.shininess = .3f;
    pose.white = .4f;
    pose.depthOffset = .005f;
    DrawItem item;
    check(worldmodel::applyPose(item, pose) && item.material == glm::vec4(.1234567f, 1.25f, .37f, .17f) && item.opacity == 1,
          "material RGBA remains unquantized and independent of opaque pixel opacity");
    check(item.nativeControls && item.nativeControls->glow == .2f && item.nativeControls->shininess == .3f &&
              item.nativeControls->white == .4f && item.nativeControls->depthOffset == .005f && !item.nativeControls->opposite,
          "all four effect controls retain their submitted values");
    pose.kind = nativepose::MIRRORED_DRAW;
    pose.opacity = .5f;
    check(worldmodel::applyPose(item, pose) && item.nativeControls->opposite && item.opacity == .5f,
          "the explicit opposite helper survives with an identity model and faded opacity");
    pose.kind = 0;
    item.model[0][0] = -1;
    check(worldmodel::applyPose(item, pose) && !item.nativeControls->opposite,
          "matrix reflection does not substitute for the producer's winding choice");
    pose.kind = nativepose::WEAPON_EFFECT_DRAW;
    check(!worldmodel::applyPose(item, pose), "weapon-effect placeholders require their separate spirit expansion");
    {
        const RenderStyle previous = renderStyle();
        setRenderStyle(RenderStyle::Steam);
        CubModel cub;
        cub.sx = 4;
        cub.sy = cub.sz = 1;
        cub.rgb = {255, 0, 0, 254, 0, 0, 123, 121, 119, 0, 0, 255};
        const auto source = cub.rgb;
        const auto ordinary = cub.mesh();
        const auto filtered = cub.mesh(0, {}, steamvoxel::CubEncoding::Stock, true);
        check(cub.attachmentMarker(0, 0, 0) && !cub.attachmentMarker(1, 0, 0) && cub.attachmentMarker(3, 0, 0),
              "only exact RGB markers are suppressed, not a near-red voxel");
        check(ordinary.indices.size() == 108 && filtered.indices.size() == 48 && cub.rgb == source,
              "hidden markers own no faces but still block adjacent faces without changing raw RGB");
        static_assert(sizeof(Vertex) == 20);
        check(std::all_of(filtered.vertices.begin(), filtered.vertices.end(),
                          [&](const Vertex &v) {
                              return std::any_of(ordinary.vertices.begin(), ordinary.vertices.end(),
                                                 [&](const Vertex &raw) { return std::memcmp(&v, &raw, sizeof v) == 0; });
                          }),
              "surviving packed vertex colors retain raw-marker corner occlusion");
        setRenderStyle(previous);
    }
    std::printf("Native WORLD model controls: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
