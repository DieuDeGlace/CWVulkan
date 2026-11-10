#pragma once
// Plain term resolution for Help. Authority: the complete held
// steam-help-action-description-dictionaries-and-term-resolution-20261110.md.
// XML decoding, stream locale and cycle protection are owned provider policies.
// Existing NPC/dialogue APIs keep their independent paths.
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

namespace nativeterms {
using Text = std::u16string;
using View = std::u16string_view;
struct Context {
    std::set<Text> categories;
    std::map<Text, Text> names;
};

View actionToken(int32_t action);
class Dictionary {
  public:
    Dictionary();
    ~Dictionary();
    Dictionary(const Dictionary &) = delete;
    Dictionary &operator=(const Dictionary &) = delete;
    // Complete successful loads replace the owned resource maps. Failed loads
    // retain them; application startup attempts the fixed file once per Game owner.
    bool load(const std::filesystem::path &, std::string &error);
    bool loadXml(std::string_view utf8, std::string &error);
    size_t size() const;
    Text format(View token, const Context &, uint32_t seed = 0) const;
    Text describe(int32_t action) const; // #default, @ -> empty, seed zero

  private:
    struct Data;
    std::unique_ptr<Data> data;
};
} // namespace nativeterms
