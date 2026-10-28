#pragma once
// The byte level of Steam Cube World's network protocol (docs/specs/netcode-packets.md §1):
// little-endian values written one after another, a reader that never fails, and the zlib
// streams packets 3 and 7 carry.
//
// The reader follows the game's rule: a read of n bytes first checks that n bytes remain. If
// they don't, the read position jumps to the end and the value keeps whatever the caller had
// in it before (its default). A short packet therefore never fails; its missing fields simply
// keep their defaults. Extra trailing bytes are ignored. The reader remembers that it ran
// short, so a tool can tell a complete packet from a cut one.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace cwnet {

using Bytes = std::vector<uint8_t>;

class ByteWriter {
  public:
    explicit ByteWriter(Bytes &out) : out(out) {}

    void bytes(const void *data, size_t size) {
        const auto *p = static_cast<const uint8_t *>(data);
        out.insert(out.end(), p, p + size);
    }
    // Any plain value, as its raw little-endian bytes (the machines are all little-endian).
    template <class T> void value(const T &v) {
        static_assert(std::is_trivially_copyable_v<T>);
        bytes(&v, sizeof v);
    }
    void u32(uint32_t v) { value(v); }
    void i32(int32_t v) { value(v); }
    void i64(int64_t v) { value(v); }
    size_t size() const { return out.size(); }

  private:
    Bytes &out;
};

class ByteReader {
  public:
    ByteReader(const uint8_t *data, size_t size) : data(data), size(size) {}
    explicit ByteReader(const Bytes &b) : data(b.data()), size(b.size()) {}

    // Reads `count` bytes into `to`; when fewer remain, `to` is left alone and the reader
    // moves to the end.
    bool bytes(void *to, size_t count) {
        if (count > size - position) {
            position = size;
            ranShort = true;
            return false;
        }
        std::memcpy(to, data + position, count);
        position += count;
        return true;
    }
    template <class T> bool value(T &v) {
        static_assert(std::is_trivially_copyable_v<T>);
        return bytes(&v, sizeof v);
    }
    // Convenience reads that start from a default.
    template <class T> T valueOr(T fallback) {
        value(fallback);
        return fallback;
    }

    size_t remaining() const { return size - position; }
    size_t offset() const { return position; }
    bool atEnd() const { return position == size; }
    // Whether some read asked for more than was left.
    bool wasShort() const { return ranShort; }
    const uint8_t *here() const { return data + position; }
    void skip(size_t count) {
        if (count > size - position) {
            position = size;
            ranShort = true;
        } else
            position += count;
    }

  private:
    const uint8_t *data;
    size_t size;
    size_t position = 0;
    bool ranShort = false;
};

// zlib (RFC 1950) at zlib's default level, as the game's one-call compress does (0x322280).
// Any valid stream is accepted by the receiver, so the bytes need not equal the game's.
Bytes compress(const uint8_t *data, size_t size);
inline Bytes compress(const Bytes &b) { return compress(b.data(), b.size()); }

// Inflates a zlib stream as the game's reader does (0x66dc0): as far as the stream goes. A
// stream that is damaged or cut yields what was inflated before the fault; `complete` says
// whether the stream ended properly.
Bytes inflateStream(const uint8_t *data, size_t size, bool *complete = nullptr);

} // namespace cwnet
