#include "net/Wire.h"

#include <zlib.h>

namespace cwnet {

Bytes compress(const uint8_t *data, size_t size) {
    uLongf bound = compressBound((uLong)size);
    Bytes out(bound);
    if (compress2(out.data(), &bound, data, (uLong)size, Z_DEFAULT_COMPRESSION) != Z_OK)
        return {};
    out.resize(bound);
    return out;
}

Bytes inflateStream(const uint8_t *data, size_t size, bool *complete) {
    // The game inflates in chunks of 128,000 bytes until the stream ends.
    constexpr size_t CHUNK = 128000;
    Bytes out;
    if (complete)
        *complete = false;
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK)
        return out;
    stream.next_in = const_cast<Bytef *>(data);
    stream.avail_in = (uInt)size;
    int result = Z_OK;
    while (result == Z_OK) {
        const size_t had = out.size();
        out.resize(had + CHUNK);
        stream.next_out = out.data() + had;
        stream.avail_out = (uInt)CHUNK;
        result = inflate(&stream, Z_NO_FLUSH);
        out.resize(had + (CHUNK - stream.avail_out));
        if (result == Z_OK && stream.avail_in == 0 && stream.avail_out != 0)
            break; // the input ran out before the stream's end
    }
    if (complete)
        *complete = result == Z_STREAM_END;
    inflateEnd(&stream);
    return out;
}

} // namespace cwnet
