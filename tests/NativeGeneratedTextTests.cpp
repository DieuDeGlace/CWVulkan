#include "../tools/ZoneExtractor/CaptureTextUtf8.h"
#include "../tools/ZoneExtractor/NativeGeneratedText.h"
#include <iostream>
#include <map>
#include <vector>

namespace {
int disposed = 0;
void destroy(captureabi::WideTextStorage *) { ++disposed; }
} // namespace

int nativeGeneratedTextTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL generated text: " << label << '\n';
            ++failures;
        }
    };
    const auto generate = [](captureabi::WideTextStorage *out) { out->words = {0, 0, 0, 7}; };
    disposed = 0;
    {
        const captureabi::WideTextResult result(generate, destroy);
        check(disposed == 0, "completed result remains alive during consumption");
    }
    check(disposed == 1, "normal scope disposes native object exactly once");
    try {
        const captureabi::WideTextResult result(generate, destroy);
        throw 1;
    } catch (int) {
    }
    check(disposed == 2, "consumer exception disposes through native object callback");
    try {
        const captureabi::WideTextResult result([](captureabi::WideTextStorage *) { throw 1; }, destroy);
    } catch (int) {
    }
    check(disposed == 2, "failed native construction has no assumed valid destination to dispose");

    check(captureabi::textResourceOwner(0x2000) == 0x2050 && captureabi::textResourceOwner(0) == 0 &&
              captureabi::textResourceOwner(UINTPTR_MAX - 0x4f) == 0,
          "owner is an inline address, with null and overflow rejected");
    std::array<uint8_t, 32> record{};
    std::map<uintptr_t, std::vector<uint8_t>> heap;
    const auto put = [&](size_t offset, uint64_t value) { std::memcpy(record.data() + offset, &value, 8); };
    const auto setText = [&](const std::u16string &text, uint64_t capacity) {
        record.fill(0xa5);
        put(16, text.size());
        put(24, capacity);
        if (capacity < 8)
            std::memcpy(record.data(), text.c_str(), (text.size() + 1) * 2);
        else {
            put(0, 0x4000);
            auto &buffer = heap[0x4000];
            buffer.resize((text.size() + 1) * 2);
            std::memcpy(buffer.data(), text.c_str(), buffer.size());
        }
    };
    const auto read = [&](uintptr_t address, void *out, size_t count) {
        if (address == 0x1000 && count == record.size()) {
            std::memcpy(out, record.data(), count);
            return true;
        }
        const auto found = heap.find(address);
        if (found == heap.end() || count > found->second.size())
            return false;
        std::memcpy(out, found->second.data(), count);
        return true;
    };
    std::u16string output;
    setText(u"", 7);
    check(captureabi::readWideText(0x1000, output, read) && output.empty(), "empty inline result is successful");
    const std::u16string inlineText{u'A', 0, u'\u00e9', 0xd83d, 0xde00, u'Z', u'!'};
    setText(inlineText, 7);
    check(captureabi::readWideText(0x1000, output, read) && output == inlineText,
          "seven inline UTF16 units retain embedded zero and surrogate pair, ignoring spare bytes");
    setText(u"12345678", 8);
    check(captureabi::readWideText(0x1000, output, read) && output == u"12345678", "capacity8 uses heap storage");
    setText(u"small", 2047);
    check(captureabi::readWideText(0x1000, output, read) && output == u"small",
          "large allocation capacity does not become text length or a manual free policy");
    heap[0x4000].back() = 1;
    check(!captureabi::readWideText(0x1000, output, read) && output.empty(),
          "bad terminator publishes no partial text");
    setText(u"12345678", 8);
    heap.clear();
    check(!captureabi::readWideText(0x1000, output, read) && output.empty(),
          "unreadable heap is a failure, not valid empty");
    put(16, 9);
    check(!captureabi::readWideText(0x1000, output, read), "length exceeding capacity rejected");
    put(16, captureabi::MAX_CAPTURE_TEXT_UNITS + 1);
    put(24, captureabi::MAX_CAPTURE_TEXT_UNITS + 1);
    check(!captureabi::readWideText(0x1000, output, read), "capture budget enforced before allocation/read");
    setText(u"ok", 7);
    int calls = 0;
    const auto changing = [&](uintptr_t address, void *out, size_t count) {
        if (++calls == 2)
            record[24] ^= 1;
        return read(address, out, count);
    };
    check(!captureabi::readWideText(0x1000, output, changing) && output.empty(),
          "changed object metadata publishes no partial text");
    check(!captureabi::readWideText(UINTPTR_MAX - 15, output, read), "record address overflow rejected");
    std::string utf8;
    check(captureabi::exportUtf8(u"A\u00e9\U0001f600", utf8) && utf8 == "A\xc3\xa9\xf0\x9f\x98\x80",
          "UTF16 BMP and surrogate pair convert to exact UTF8 without normalization");
    check(!captureabi::exportUtf8(std::u16string{u'A', 0, u'B'}, utf8) && utf8.empty(),
          "line export rejects embedded zero instead of truncating text");
    check(!captureabi::exportUtf8(std::u16string{0xd800}, utf8) && utf8.empty(),
          "ill-formed UTF16 rejects without replacement or partial bytes");
    check(captureabi::exportUtf8(u"", utf8) && utf8.empty(), "valid empty converts successfully");
    std::cout << "Native generated text capture: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
