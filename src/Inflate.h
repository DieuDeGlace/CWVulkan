#pragma once
// A small zlib-stream decoder (RFC 1950 around RFC 1951 DEFLATE), written from the public
// specifications, for the compressed parts of the game's .plx files (Texture.compressedPixels).
#include <cstdint>
#include <vector>

namespace inflate {

// Decodes a zlib stream (2-byte header, DEFLATE blocks, Adler-32). False on malformed input.
bool zlib(const uint8_t *data, size_t size, std::vector<uint8_t> &out);

} // namespace inflate
