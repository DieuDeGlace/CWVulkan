#include "Config.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>

static std::string lower(std::string s) {
    for (auto &c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}
static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

bool Config::load(const std::filesystem::path &file) {
    std::ifstream in(file);
    if (!in)
        return false;
    std::string line, section;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = lower(trim(line.substr(1, line.size() - 2)));
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        values[section + "." + lower(trim(line.substr(0, eq)))] = trim(line.substr(eq + 1));
    }
    return true;
}

const std::string *Config::find(const std::string &section, const std::string &key) const {
    auto it = values.find(lower(section) + "." + lower(key));
    return it == values.end() ? nullptr : &it->second;
}

std::string Config::text(const std::string &section, const std::string &key,
                         const std::string &fallback) const {
    auto v = find(section, key);
    return v ? *v : fallback;
}

double Config::number(const std::string &section, const std::string &key, double fallback) const {
    auto v = find(section, key);
    if (!v || v->empty())
        return fallback;
    char *end = nullptr;
    double d = std::strtod(v->c_str(), &end);
    return end == v->c_str() ? fallback : d;
}

double Config::number(const std::string &section, const std::string &key, double fallback,
                      double low, double high) const {
    return std::clamp(number(section, key, fallback), low, high);
}

bool Config::flag(const std::string &section, const std::string &key, bool fallback) const {
    return number(section, key, fallback ? 1 : 0) != 0;
}

void Config::set(const std::string &section, const std::string &key, const std::string &value) {
    std::string name = section + "." + key;
    for (auto &c : name)
        c = (char)std::tolower((unsigned char)c);
    values[name] = value;
}
