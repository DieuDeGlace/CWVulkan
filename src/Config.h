#pragma once
// Minimal INI reader: `[Section]`, `Key=Value`, `;` or `#` comments.
// Section and key names are case-insensitive.
#include <filesystem>
#include <map>
#include <string>

class Config {
  public:
    bool load(const std::filesystem::path &file);
    const std::string *find(const std::string &section, const std::string &key) const;
    std::string text(const std::string &section, const std::string &key,
                     const std::string &fallback = {}) const;
    double number(const std::string &section, const std::string &key, double fallback) const;
    // Number clamped to [low, high].
    double number(const std::string &section, const std::string &key, double fallback, double low,
                  double high) const;
    bool flag(const std::string &section, const std::string &key, bool fallback) const;
    // Sets a value in memory (settings changed in the menu while playing).
    void set(const std::string &section, const std::string &key, const std::string &value);

  private:
    std::map<std::string, std::string> values; // "section.key" (lower case) -> value
};
