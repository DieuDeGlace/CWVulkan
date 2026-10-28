#include "Inflate.h"

#include <cstring>

namespace inflate {
namespace {

struct Bits {
    const uint8_t *p, *end;
    uint32_t buffer = 0;
    int count = 0;
    bool bad = false;
    int need(int n) {
        while (count < n) {
            if (p >= end) {
                bad = true;
                return 0;
            }
            buffer |= (uint32_t)*p++ << count;
            count += 8;
        }
        const int v = (int)(buffer & ((1u << n) - 1));
        buffer >>= n;
        count -= n;
        return v;
    }
    void byteAlign() { buffer = 0, count = 0; }
};

// A canonical Huffman code: counts per length and the symbols in order (RFC 1951 3.2.2).
struct Huffman {
    uint16_t counts[16] = {};
    uint16_t symbols[320] = {};
    bool build(const uint8_t *lengths, int n) {
        std::memset(counts, 0, sizeof counts);
        for (int i = 0; i < n; i++)
            counts[lengths[i]]++;
        counts[0] = 0;
        int left = 1;
        for (int len = 1; len < 16; len++) {
            left <<= 1;
            left -= counts[len];
            if (left < 0)
                return false; // over-subscribed
        }
        uint16_t offsets[16];
        offsets[1] = 0;
        for (int len = 1; len < 15; len++)
            offsets[len + 1] = offsets[len] + counts[len];
        for (int i = 0; i < n; i++)
            if (lengths[i])
                symbols[offsets[lengths[i]]++] = (uint16_t)i;
        return true;
    }
    int decode(Bits &b) const {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len < 16; len++) {
            code |= b.need(1);
            if (b.bad)
                return -1;
            const int count = counts[len];
            if (code - count < first)
                return symbols[index + (code - first)];
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }
};

const uint16_t lengthBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t lengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t distanceBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                                   1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint8_t distanceExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool codes(Bits &b, std::vector<uint8_t> &out, const Huffman &lit, const Huffman &dist) {
    for (;;) {
        const int sym = lit.decode(b);
        if (sym < 0)
            return false;
        if (sym < 256) {
            out.push_back((uint8_t)sym);
            continue;
        }
        if (sym == 256)
            return true;
        const int li = sym - 257;
        if (li >= 29)
            return false;
        const int length = lengthBase[li] + b.need(lengthExtra[li]);
        const int ds = dist.decode(b);
        if (ds < 0 || ds >= 30)
            return false;
        const size_t distance = distanceBase[ds] + (size_t)b.need(distanceExtra[ds]);
        if (b.bad || distance > out.size())
            return false;
        const size_t from = out.size() - distance;
        for (int i = 0; i < length; i++)
            out.push_back(out[from + (size_t)i]);
    }
}

bool fixedBlock(Bits &b, std::vector<uint8_t> &out) {
    static Huffman lit, dist;
    static bool made = false;
    if (!made) {
        uint8_t lengths[288];
        for (int i = 0; i < 144; i++) lengths[i] = 8;
        for (int i = 144; i < 256; i++) lengths[i] = 9;
        for (int i = 256; i < 280; i++) lengths[i] = 7;
        for (int i = 280; i < 288; i++) lengths[i] = 8;
        lit.build(lengths, 288);
        for (int i = 0; i < 30; i++) lengths[i] = 5;
        dist.build(lengths, 30);
        made = true;
    }
    return codes(b, out, lit, dist);
}

bool dynamicBlock(Bits &b, std::vector<uint8_t> &out) {
    static const uint8_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    const int nlen = b.need(5) + 257, ndist = b.need(5) + 1, ncode = b.need(4) + 4;
    if (b.bad || nlen > 286 || ndist > 30)
        return false;
    uint8_t lengths[320] = {};
    for (int i = 0; i < ncode; i++)
        lengths[order[i]] = (uint8_t)b.need(3);
    Huffman codeCode;
    if (!codeCode.build(lengths, 19))
        return false;
    int index = 0;
    while (index < nlen + ndist) {
        int sym = codeCode.decode(b);
        if (sym < 0)
            return false;
        if (sym < 16) {
            lengths[index++] = (uint8_t)sym;
            continue;
        }
        uint8_t value = 0;
        int repeat;
        if (sym == 16) {
            if (index == 0)
                return false;
            value = lengths[index - 1];
            repeat = 3 + b.need(2);
        } else if (sym == 17)
            repeat = 3 + b.need(3);
        else
            repeat = 11 + b.need(7);
        if (index + repeat > nlen + ndist)
            return false;
        while (repeat--)
            lengths[index++] = value;
    }
    Huffman lit, dist;
    if (!lit.build(lengths, nlen) || !dist.build(lengths + nlen, ndist))
        return false;
    return codes(b, out, lit, dist);
}

} // namespace

bool zlib(const uint8_t *data, size_t size, std::vector<uint8_t> &out) {
    if (size < 2 || (data[0] & 0x0f) != 8 || ((data[0] << 8) | data[1]) % 31 != 0 || (data[1] & 0x20))
        return false;
    Bits b{data + 2, data + size};
    for (;;) {
        const int last = b.need(1), type = b.need(2);
        if (b.bad)
            return false;
        if (type == 0) { // stored
            b.byteAlign();
            if (b.end - b.p < 4)
                return false;
            const uint16_t len = (uint16_t)(b.p[0] | b.p[1] << 8), nlen = (uint16_t)(b.p[2] | b.p[3] << 8);
            b.p += 4;
            if ((uint16_t)~nlen != len || b.end - b.p < len)
                return false;
            out.insert(out.end(), b.p, b.p + len);
            b.p += len;
        } else if (type == 1) {
            if (!fixedBlock(b, out))
                return false;
        } else if (type == 2) {
            if (!dynamicBlock(b, out))
                return false;
        } else
            return false;
        if (last)
            return true;
    }
}

} // namespace inflate
