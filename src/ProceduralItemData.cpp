#include "ProceduralItemData.h"
#include <algorithm>
#include <cstdio>
#include <utility>
#include <cstring>
#include <charconv>
#include <sstream>
#include <set>

namespace proceduralitem {
namespace {
bool fail(std::string *error, const char *reason) {
    if (error)
        *error = reason;
    return false;
}
uint32_t word(const std::vector<uint8_t> &bytes, size_t at) {
    return uint32_t(bytes[at]) | uint32_t(bytes[at + 1]) << 8 | uint32_t(bytes[at + 2]) << 16 | uint32_t(bytes[at + 3]) << 24;
}
int32_t signedWord(uint32_t bits) {
    int32_t value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
void word(std::vector<uint8_t> &bytes, size_t at, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes[at + i] = uint8_t(value >> (8 * i));
}
size_t volumeBytes(const Resource &resource) {
    size_t count = 3;
    for (int32_t dimension : resource.dimensions) {
        if (dimension < 1 || dimension > 256)
            return 0;
        count *= size_t(dimension);
    }
    return count;
}
bool metadata(const Resource &resource, std::string *error) {
    if (!volumeBytes(resource))
        return fail(error, "Procedural item dimensions must be within 1..256.");
    if (uint8_t(resource.origin) > uint8_t(Origin::NativeGeneration))
        return fail(error, "Unknown procedural item capture origin.");
    return true;
}
} // namespace
bool Key::operator<(const Key &other) const {
    return std::make_tuple(id, signedWord(modifier)) < std::make_tuple(other.id, signedWord(other.modifier));
}
bool decode(const std::vector<uint8_t> &bytes, Resource &destination, std::string *error) {
    if (bytes.size() < HEADER_SIZE || bytes[0] != 'C' || bytes[1] != 'W' || bytes[2] != 'P' || bytes[3] != '1')
        return fail(error, "Missing procedural item CWP1 header.");
    if (bytes[30] || bytes[31])
        return fail(error, "Unsupported procedural item header flags.");
    Resource resource;
    resource.key = {signedWord(word(bytes, 4)), word(bytes, 8)};
    for (size_t axis = 0; axis < 3; ++axis)
        resource.dimensions[axis] = signedWord(word(bytes, 12 + 4 * axis));
    resource.classification = bytes[24];
    resource.mode = bytes[25];
    std::copy_n(bytes.begin() + 26, 3, resource.shade.begin());
    resource.origin = Origin(bytes[29]);
    resource.radius = signedWord(word(bytes, 32));
    resource.sourceTimestamp = word(bytes, 36);
    if (!metadata(resource, error))
        return false;
    const size_t count = volumeBytes(resource);
    if (word(bytes, 40) != count || bytes.size() != HEADER_SIZE + count)
        return fail(error, "Procedural item RGB allocation size does not match its dimensions.");
    resource.rgb.assign(bytes.begin() + HEADER_SIZE, bytes.end());
    destination = std::move(resource);
    return true;
}
bool encode(const Resource &resource, std::vector<uint8_t> &destination, std::string *error) {
    if (!metadata(resource, error))
        return false;
    const size_t count = volumeBytes(resource);
    if (resource.rgb.size() != count)
        return fail(error, "Procedural item RGB allocation size does not match its dimensions.");
    std::vector<uint8_t> bytes(HEADER_SIZE + count);
    bytes[0] = 'C';
    bytes[1] = 'W';
    bytes[2] = 'P';
    bytes[3] = '1';
    word(bytes, 4, uint32_t(resource.key.id));
    word(bytes, 8, resource.key.modifier);
    for (size_t axis = 0; axis < 3; ++axis)
        word(bytes, 12 + 4 * axis, uint32_t(resource.dimensions[axis]));
    bytes[24] = resource.classification;
    bytes[25] = resource.mode;
    std::copy(resource.shade.begin(), resource.shade.end(), bytes.begin() + 26);
    bytes[29] = uint8_t(resource.origin);
    word(bytes, 32, uint32_t(resource.radius));
    word(bytes, 36, resource.sourceTimestamp);
    word(bytes, 40, uint32_t(count));
    std::copy(resource.rgb.begin(), resource.rgb.end(), bytes.begin() + HEADER_SIZE);
    destination = std::move(bytes);
    return true;
}
std::string fileName(Key key) {
    char name[32];
    std::snprintf(name, sizeof name, "p_%08x_%08x.cwp", uint32_t(key.id), key.modifier);
    return name;
}
bool decodeRequest(std::string_view text, std::vector<Key> &destination, std::string *error) {
    if (text.size() > MAX_REQUEST_BYTES)
        return fail(error, "Procedural item request exceeds64 KiB.");
    if (text.substr(0, 3) == "\xef\xbb\xbf")
        text.remove_prefix(3);
    std::istringstream input{std::string(text)};
    std::string line;
    auto next = [&] {
        if (!std::getline(input, line))
            return false;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        return true;
    };
    if (!next() || line != "CWP1")
        return fail(error, "Missing procedural item request CWP1 header.");
    std::set<Key> keys;
    while (next()) {
        std::istringstream row(line);
        std::string id, modifier, extra;
        if (!(row >> id) || id[0] == '#')
            continue;
        if (!(row >> modifier) || row >> extra || modifier.empty() || modifier.size() > 8)
            return fail(error, "Expected a signed decimal ID and full hexadecimal modifier.");
        Key key;
        const auto parsedId = std::from_chars(id.data(), id.data() + id.size(), key.id);
        const auto parsedModifier = std::from_chars(modifier.data(), modifier.data() + modifier.size(), key.modifier, 16);
        if (parsedId.ec != std::errc{} || parsedId.ptr != id.data() + id.size() || parsedModifier.ec != std::errc{} ||
            parsedModifier.ptr != modifier.data() + modifier.size())
            return fail(error, "Procedural item request contains an invalid full key.");
        keys.insert(key);
        if (keys.size() > MAX_REQUEST_KEYS)
            return fail(error, "Procedural item request exceeds512 unique keys.");
    }
    if (keys.empty())
        return fail(error, "Procedural item request contains no keys.");
    destination.assign(keys.begin(), keys.end());
    return true;
}
} // namespace proceduralitem
