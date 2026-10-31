#include "ProceduralItemData.h"
#include "ProceduralItems.h"
#include "Assets.h"
#include "CubModel.h"
#include "AlphaLook.h"
#include "World.h"
#include <cstdio>
#include <limits>
#include <set>
#include <fstream>
#include <cstdlib>
#include <algorithm>

int proceduralItemTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL procedural item: %s\n", what);
            ++failures;
        }
    };
    using namespace proceduralitem;
    Resource original;
    original.key = {-7, 0xff112233};
    original.origin = Origin::NativeLookup;
    original.sourceTimestamp = 0x5d93ab0d;
    original.shade = {17, 129, 255};
    original.radius = -12345; // Transport retains metadata; renderer admission is separate.
    original.rgb.resize(3 * 24 * 24 * 24);
    for (int z = 0; z < 24; ++z)
        for (int y = 0; y < 24; ++y)
            for (int x = 0; x < 24; ++x) {
                const size_t at = size_t((z * 24 + y) * 24 + x) * 3;
                original.rgb[at] = uint8_t(x);
                original.rgb[at + 1] = uint8_t(y);
                original.rgb[at + 2] = uint8_t(z);
            }
    std::vector<uint8_t> bytes;
    std::string error;
    check(encode(original, bytes, &error) && bytes.size() == 41516, "full24-cube volume and44-byte header");
    check(bytes[4] == 0xf9 && bytes[7] == 0xff && bytes[8] == 0x33 && bytes[11] == 0xff && bytes[36] == 0x0d && bytes[37] == 0xab &&
              bytes[38] == 0x93 && bytes[39] == 0x5d,
          "signed ID, full modifier and provenance use explicit little-endian words");
    const size_t sample = HEADER_SIZE + size_t((5 * 24 + 4) * 24 + 3) * 3;
    check(bytes[sample] == 3 && bytes[sample + 1] == 4 && bytes[sample + 2] == 5,
          "RGB order is preserved without cropping or color encoding");
    Resource restored;
    check(decode(bytes, restored, &error) && restored.key == original.key && restored.dimensions == original.dimensions &&
              restored.rgb == original.rgb && restored.classification == original.classification && restored.mode == original.mode &&
              restored.shade == original.shade && restored.radius == original.radius && restored.origin == original.origin &&
              restored.sourceTimestamp == original.sourceTimestamp,
          "all resource semantics survive transport");
    check(fileName(original.key) == "p_fffffff9_ff112233.cwp", "asset filenames retain every key bit");
    const std::set<Key> ordered{{0, 1}, {0, 0x80000000}, {-1, 0}, {0, 0xffffffff}};
    auto at = ordered.begin();
    check(*at++ == (Key{-1, 0}) && *at++ == (Key{0, 0x80000000}) && *at++ == (Key{0, 0xffffffff}) && *at == (Key{0, 1}),
          "native cache ordering compares both key words as signed");
    auto reject = [&](std::vector<uint8_t> malformed, const char *what) {
        Resource destination = original;
        check(!decode(malformed, destination, &error) && destination.rgb == original.rgb && destination.key == original.key, what);
    };
    reject({}, "empty input leaves destination intact");
    auto malformed = bytes;
    malformed[3] = '2';
    reject(malformed, "unknown format version");
    malformed = bytes;
    malformed[30] = 1;
    reject(malformed, "unknown reserved flags");
    malformed = bytes;
    malformed[29] = 255;
    reject(malformed, "unknown capture origin");
    malformed = bytes;
    malformed[12] = 0;
    reject(malformed, "zero dimension");
    malformed = bytes;
    malformed[15] = 0x80;
    reject(malformed, "negative dimension");
    malformed = bytes;
    malformed[13] = 2;
    reject(malformed, "oversized dimension before volume allocation");
    malformed = bytes;
    malformed[40] ^= 1;
    reject(malformed, "RGB length contradicts dimensions");
    malformed = bytes;
    malformed.pop_back();
    reject(malformed, "truncated RGB allocation");
    malformed = bytes;
    malformed.push_back(0);
    reject(malformed, "trailing bytes");
    auto invalid = original;
    invalid.rgb.pop_back();
    const auto retained = bytes;
    check(!encode(invalid, bytes, &error) && bytes == retained, "failed encoding leaves existing output intact");
    std::vector<Key> requested;
    check(decodeRequest("\xef\xbb\xbf"
                        "CWP1\r\n# owner keys\r\n0 ffffffff\r\n-1 0\r\n0 FFFFFFFF\r\n0 1\r\n",
                        requested, &error) &&
              requested == std::vector<Key>{{-1, 0}, {0, 0xffffffff}, {0, 1}},
          "request parser handles BOM/CRLF, full bits, deduplication and native key order");
    const auto priorRequest = requested;
    for (const std::string text :
         {"", "CWP2\n0 1\n", "CWP1\n", "CWP1\n2147483648 0\n", "CWP1\n0 100000000\n", "CWP1\n0 0x1\n", "CWP1\n0 1 extra\n", "CWP1\n0\n"})
        check(!decodeRequest(text, requested, &error) && requested == priorRequest, "invalid requests never replace pending keys");
    std::string tooMany = "CWP1\n";
    for (size_t i = 0; i <= MAX_REQUEST_KEYS; ++i)
        tooMany += std::to_string(i) + " 0\n";
    check(!decodeRequest(tooMany, requested, &error) && requested == priorRequest, "request key budget");
    check(!decodeRequest(std::string(MAX_REQUEST_BYTES + 1, ' '), requested, &error), "request input-size budget");
    check(parseSpriteKey(spriteKey(original.key)) == std::optional<Key>(original.key) &&
              !parseSpriteKey("procedural/p_00000000_00000001.cwp/extra") && !parseSpriteKey("procedural/p_0000000z_00000001.cwp"),
          "procedural render keys preserve full identity without accepting paths");
    for (int id : {-1, 0, 1, 2, 3, 4, 5, 6, 7}) {
        const auto volume = generate({id, 0xff112233});
        check(volume.key == (Key{id, 0xff112233}) && volume.rgb.size() == 41472 &&
                  volume.dimensions == std::array<int32_t, 3>{24, 24, 24} && volume.mode == 1 && volume.classification == 1 &&
                  volume.radius == 3 && std::any_of(volume.rgb.begin(), volume.rgb.end(), [](uint8_t byte) { return byte != 0; }),
              "all procedural families retain full allocations and native resource metadata");
    }
    const auto cacheDirectory = std::filesystem::current_path() / "build" / "procedural-cache-test";
    std::filesystem::create_directories(cacheDirectory);
    auto syntheticOwner = generate({3, 1});
    syntheticOwner.origin = Origin::NativeLookup;
    syntheticOwner.rgb.assign(41472, 17);
    std::vector<uint8_t> serializedOwner;
    check(encode(syntheticOwner, serializedOwner), "synthetic owner-cache fixture encoding");
    {
        std::ofstream output(cacheDirectory / fileName(syntheticOwner.key), std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char *>(serializedOwner.data()), std::streamsize(serializedOwner.size()));
    }
    OwnerCache cache(cacheDirectory);
    const auto *cached = &cache.get(syntheticOwner.key, &error);
    check(cached->rgb == syntheticOwner.rgb && cached->origin == Origin::NativeLookup,
          "owner volume takes priority over generated geometry");
    const auto *highModifier = &cache.get({3, 0x80000001}, &error);
    check(highModifier != cached && highModifier->key.modifier == 0x80000001 && &cache.get(syntheticOwner.key) == cached,
          "full-key cache entries retain stable addresses");
    {
        std::ofstream output(cacheDirectory / fileName({4, 1}), std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char *>(serializedOwner.data()), std::streamsize(serializedOwner.size()));
    }
    check(cache.get({4, 1}, &error).key == (Key{4, 1}) && !error.empty(), "wrong-key owner data cannot poison another resource");
    const auto priorStyle = renderStyle();
    CubModel voxel;
    voxel.sx = voxel.sy = voxel.sz = 1;
    voxel.rgb = {100, 120, 130};
    setRenderStyle(RenderStyle::Steam);
    const auto stockMesh = voxel.mesh();
    const auto unhalvedMesh = voxel.mesh(0, {}, steamvoxel::CubEncoding::Unhalved);
    check(!stockMesh.empty() && stockMesh.vertices.size() == unhalvedMesh.vertices.size() &&
              std::all_of(stockMesh.vertices.begin(), stockMesh.vertices.end(), [](const auto &vertex) { return vertex.ao == 255; }) &&
              std::all_of(unhalvedMesh.vertices.begin(), unhalvedMesh.vertices.end(), [](const auto &vertex) { return vertex.ao == 254; }),
          "procedural Steam meshes preserve native unhalved input/light byte254");
    setRenderStyle(RenderStyle::Alpha);
    const auto alphaStock = voxel.mesh();
    const auto alphaProcedural = voxel.mesh(0, {}, steamvoxel::CubEncoding::Unhalved);
    check(!alphaStock.empty() && alphaStock.vertices[0].r < alphaProcedural.vertices[0].r,
          "Alpha styling receives the procedural model's logical full color");
    setRenderStyle(priorStyle);

    size_t compared = 0, mismatched = 0;
    Assets assets;
    if (assets.locate("")) {
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        {
            Terrain terrain;
            World world(terrain, assets, renderer, 0);
            const auto name = spriteKey({0, 0xff112233});
            const int model = world.loadModel(name);
            check(model >= 0 && world.modelMesh(model) && world.modelSize(model) == glm::vec3(24) && !world.voxels(model).empty() &&
                      world.loadModel(name) == model,
                  "world drops resolve procedural resources with full dimensions, cached geometry and voxel access");
        }
        renderer.shutdown();
        const auto directory = assets.gameDir() / "Extract" / "procedural-items";
        std::error_code ec;
        if (std::filesystem::is_directory(directory, ec))
            for (const auto &entry : std::filesystem::directory_iterator(directory)) {
                if (entry.path().extension() != ".cwp")
                    continue;
                std::ifstream input(entry.path(), std::ios::binary | std::ios::ate);
                const auto size = input.tellg();
                if (size < std::streamoff(HEADER_SIZE) || size > std::streamoff(HEADER_SIZE + MAX_RGB_BYTES)) {
                    check(false, "owner oracle file size");
                    continue;
                }
                std::vector<uint8_t> data(size_t(size), uint8_t{});
                input.seekg(0);
                input.read(reinterpret_cast<char *>(data.data()), std::streamsize(data.size()));
                Resource native;
                if (!input || !decode(data, native, &error)) {
                    check(false, "owner oracle decoding");
                    continue;
                }
                if (native.origin == Origin::Generated)
                    continue; // Never use the implementation's own output as its native oracle.
                const auto independent = generate(native.key);
                ++compared;
                const bool same = native.dimensions == independent.dimensions && native.classification == independent.classification &&
                                  native.mode == independent.mode && native.radius == independent.radius &&
                                  native.shade == independent.shade && native.rgb == independent.rgb;
                if (!same) {
                    ++mismatched;
                    size_t different = 0, first = 0;
                    for (size_t i = 0; i < std::min(native.rgb.size(), independent.rgb.size()); ++i)
                        if (native.rgb[i] != independent.rgb[i]) {
                            if (!different)
                                first = i;
                            ++different;
                        }
                    std::printf("Procedural owner mismatch id%d modifier%08x: %zu RGB bytes; first byte%zu\n", native.key.id,
                                native.key.modifier, different, first);
                }
                check(same, "independent generation equals completed native RGB and metadata");
            }
    }
    if (compared)
        std::printf("Procedural owner volumes: %zu compared, %zu mismatched\n", compared, mismatched);
    else {
        std::puts("SKIP procedural native volumes: completed owner captures unavailable");
        if (std::getenv("CW_REQUIRE_PROCEDURAL_ORACLES"))
            check(false, "required owner procedural volumes are absent");
    }
    std::printf("Procedural item data: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
