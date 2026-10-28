#include "Character.h"
#include "Recording.h"
#include "NativeAttackTiming.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
// Which animation state matrix the game draws each sprite with (matrix order as
// in CWSDK's Creature::animation state; verified against the snapshot's positions).
struct Binding {
    const char *sprite;
    int matrices[2];
};
const Binding BINDINGS[] = {
    {"face", {3, -1}},  {"hair", {4, -1}},  {"chest", {2, -1}},        {"hands", {9, 10}},
    {"feet", {15, 16}}, {"weapon_right", {7, -1}}, {"weapon_left", {8, -1}},
    {"wings", {13, 14}}, {"tail", {17, -1}}, {"shoulder", {11, -1}}, {"neck", {12, -1}},
};

glm::mat4 blendPart(const glm::mat4 &a, const glm::mat4 &b, float t) {
    if (t <= 0) return a;
    if (t >= 1) return b;
    if (!recording::isPart(a) || !recording::isPart(b)) return a;
    auto split = [](const glm::mat4 &m, glm::vec3 &scale, glm::quat &rotation) {
        scale = {glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2]))};
        if (glm::determinant(glm::mat3(m)) < 0) scale.x = -scale.x;
        glm::mat3 axes(glm::vec3(m[0]) / scale.x, glm::vec3(m[1]) / scale.y, glm::vec3(m[2]) / scale.z);
        rotation = glm::normalize(glm::quat_cast(axes));
    };
    glm::vec3 sa, sb; glm::quat qa, qb;
    split(a, sa, qa); split(b, sb, qb);
    if ((sa.x < 0) != (sb.x < 0)) return a;
    return glm::translate(glm::mat4(1), glm::mix(glm::vec3(a[3]), glm::vec3(b[3]), t)) *
           glm::mat4_cast(glm::slerp(qa, qb, t)) * glm::scale(glm::mat4(1), glm::mix(sa, sb, t));
}

} // namespace

bool CharacterRecording::load(const fs::path &dir, std::string &error) {
    partList.clear();
    frames.clear();
    abilityRuns.clear();
    maxHp = attack = armor = critical = haste = 0;
    name.clear(); level = specialization = 0;
    idle = walk = air = {};
    forwardAngle = 0;
    error.clear();
    std::ifstream snap(dir / "snapshot.txt");
    if (!snap) {
        error = "no snapshot.txt in " + dir.string() + " (press F5 in game with ZoneExtractor)";
        return false;
    }
    glm::u8vec3 hair(255);
    std::string line;
    while (std::getline(snap, line)) {
        int r, g, b;
        if (std::sscanf(line.c_str(), "hair_color=%d %d %d", &r, &g, &b) == 3)
            hair = glm::u8vec3(r, g, b);
        if (line.rfind("name=", 0) == 0)
            name = line.substr(5);
        if (line.rfind("level=", 0) == 0)
            level = std::atoi(line.c_str() + 6);
        if (line.rfind("specialization=", 0) == 0)
            specialization = std::atoi(line.c_str() + 15);
        if (line.rfind("race=", 0) == 0)
            race = std::atoi(line.c_str() + 5);
        if (line.rfind("flags2=", 0) == 0)
            flags2 = (uint32_t)std::strtoul(line.c_str() + 7, nullptr, 16);
        if (line.rfind("stats ", 0) == 0)
            std::sscanf(line.c_str(), "stats hp=%*f maxhp=%f attack=%f spell=%f armor=%f resistance=%f critical=%f", &maxHp,
                        &attack, &spell, &armor, &resistance, &critical);
        int category, id;
        if (std::sscanf(line.c_str(), "equip weapon_right category=%d id=%d", &category, &id) == 2)
            weaponId = category == 3 ? id : -1;
        if (line.rfind("stats ", 0) == 0 && line.find("managen=") != std::string::npos)
            manaGeneration = std::strtof(line.c_str() + line.find("managen=") + 8, nullptr);
        if (line.rfind("stats ", 0) == 0 && line.find("haste=") != std::string::npos)
            haste = std::strtof(line.c_str() + line.find("haste=") + 6, nullptr);
        if (line.rfind("scales ", 0) == 0 && line.find("hitbox=") != std::string::npos)
            hitboxScale = std::strtof(line.c_str() + line.find("hitbox=") + 7, nullptr);
        // sprite <part> slot=<n> sprites/<name>.cub
        if (line.rfind("sprite ", 0) != 0)
            continue;
        std::istringstream in(line.substr(7));
        std::string part, slot, file;
        in >> part >> slot >> file;
        if (file.empty())
            continue;
        if (file.rfind("sprites/", 0) == 0)
            file = file.substr(8);
        if (file.size() > 4 && file.compare(file.size() - 4, 4, ".cub") == 0)
            file.resize(file.size() - 4);
        for (auto &b : BINDINGS)
            if (part == b.sprite)
                for (int m : b.matrices)
                    if (m >= 0)
                        partList.push_back({file, m, part == "hair" ? hair : glm::u8vec3(255), part == "hair"});
    }
    if (partList.empty()) {
        error = "snapshot lists no body-part sprites";
        return false;
    }
    // Every recording, oldest first; runs never cross from one file to the next.
    std::vector<fs::path> files;
    std::error_code ec;
    for (auto &e : fs::directory_iterator(dir, ec))
        if (e.path().extension() == ".cwa")
            files.push_back(e.path());
    std::sort(files.begin(), files.end());
    if (files.empty()) {
        error = "no animation recording (press F4 in game with ZoneExtractor)";
        return false;
    }
    double offset = 0;
    int segment = 0;
    for (auto &file : files) {
        std::vector<recording::Frame> raw;
        bool positioned = false;
        if (!recording::read(file, raw, &positioned))
            continue;
        segment++;
        for (auto &r : raw) {
            glm::mat4 root = recording::matrix(r, 0);
            if (!recording::isPart(root))
                continue;
            Frame f;
            f.t = offset + (r.ms - raw[0].ms) / 1000.0;
            f.segment = segment;
            f.ability = r.ability;
            f.state = r.state;
            f.phase = std::isfinite(r.sinceAbility) ? r.sinceAbility * .001f : 0;
            f.yaw = std::isfinite(r.yaw) ? glm::radians(r.yaw) : 0;
            f.speed = std::hypot(r.velocity[0], r.velocity[1]);
            f.vertical = r.velocity[2];
            // Remove entity facing, not the animated root's angle: the latter
            // contains the torso wind-up and turn of the native attack itself.
            // CWA2 supplies the entity height, so root bob/lunge offsets survive.
            float angle = f.yaw;
            f.heading = std::atan2(r.velocity[1], r.velocity[0]) - angle;
            glm::mat4 toBody = glm::rotate(glm::mat4(1), -angle, glm::vec3(0, 0, 1)) *
                               glm::translate(glm::mat4(1), glm::vec3(0, 0, positioned ? -r.position[2] / 65536.f : -root[3].z));
            for (auto &p : partList) {
                glm::mat4 m = recording::matrix(r, p.matrix);
                f.local.push_back(recording::isPart(m) ? toBody * m : glm::mat4(0));
            }
            frames.push_back(std::move(f));
        }
        if (!frames.empty())
            offset = frames.back().t + 1;
    }
    if (frames.size() < 2) {
        error = "recording has no frames";
        return false;
    }
    // Feet on z = 0: the lowest foot origin of the first frame (feet sprites'
    // origins are at their soles).
    float sole = 0;
    bool any = false;
    for (size_t i = 0; i < partList.size(); i++)
        if ((partList[i].matrix == 15 || partList[i].matrix == 16) && recording::isPart(frames[0].local[i])) {
            float z = frames[0].local[i][3].z;
            sole = any ? std::min(sole, z) : z;
            any = true;
        }
    for (auto &f : frames)
        for (auto &m : f.local)
            m = glm::translate(glm::mat4(1), glm::vec3(0, 0, -sole)) * m;

    // Longest runs of standing still, walking on the ground, and being airborne.
    auto longest = [&](auto test) {
        Range best, cur;
        for (size_t i = 0; i <= frames.size(); i++) {
            if (cur.count && i < frames.size() && frames[i].segment != frames[i - 1].segment) {
                if (cur.count > best.count)
                    best = cur;
                cur = {};
            }
            if (i < frames.size() && test(frames[i])) {
                if (!cur.count)
                    cur.first = i;
                cur.count++;
            } else {
                if (cur.count > best.count)
                    best = cur;
                cur = {};
            }
        }
        if (best.count) {
            best.duration = frames[best.first + best.count - 1].t - frames[best.first].t;
            for (size_t i = best.first; i + 1 < best.first + best.count; i++)
                best.distance += frames[i].speed * (frames[i + 1].t - frames[i].t);
        }
        return best;
    };
    idle = longest([](const Frame &f) { return f.state == 0 && f.speed < .05f && std::abs(f.vertical) < .05f; });
    walk = longest([](const Frame &f) { return f.state == 0 && f.speed > 1 && std::abs(f.vertical) < .05f; });
    air = longest([](const Frame &f) { return f.state == 0 && std::abs(f.vertical) >= .05f; });
    // Facing: the mean movement direction (in the body frame) while walking.
    if (walk.count) {
        float sx = 0, sy = 0;
        for (size_t i = walk.first; i < walk.first + walk.count; i++) {
            sx += std::cos(frames[i].heading);
            sy += std::sin(frames[i].heading);
        }
        forwardAngle = std::atan2(sy, sx);
    }
    // First complete run of every ability id: frames in one clip with that id
    // while the game's animation state is active (the id stays set afterwards).
    auto active = [&](size_t k) { return frames[k].state != 0 ? frames[k].ability : 0; };
    for (size_t i = 0; i < frames.size();) {
        size_t j = i + 1;
        while (j < frames.size() && active(j) == active(i) && frames[j].segment == frames[i].segment &&
               frames[j].phase + .001f >= frames[j - 1].phase)
            j++;
        int id = active(i);
        bool known = std::any_of(abilityRuns.begin(), abilityRuns.end(), [&](auto &a) { return a.first == id; });
        // Runs touching a clip's start may be cut short; keep a later full one.
        bool basic = id == 19 || id == 20 || id == 23 || id == 31 || id == 33 || id == 34 || id == 58;
        bool clipped = i == 0 || frames[i - 1].segment != frames[i].segment ||
                       (basic && (frames[i].phase > .05f || j == frames.size() || frames[j].segment != frames[i].segment));
        if (id && !known && !clipped) {
            double end = j < frames.size() && frames[j].segment == frames[i].segment ? frames[j].t :
                         frames[j - 1].t + (j - i > 1 ? frames[j - 1].t - frames[j - 2].t : 1. / 60);
            Range r{i, j - i, end - frames[i].t, 0};
            r.phaseClock = frames[j - 1].phase > frames[i].phase + .001f;
            double native = nativeAttackDuration(id, haste);
            if (native > 0) {
                double available = r.phaseClock ? frames[j - 1].phase - frames[i].phase +
                    (r.count > 1 ? frames[j - 1].phase - frames[j - 2].phase : 1. / 60) : r.duration;
                if (available + .025 < native) { i = j; continue; }
                r.duration = native;
                // Retain a bracketing sample for interpolation at the native end.
                size_t last = i + 1;
                while (last < j && (r.phaseClock ? frames[last].phase - frames[i].phase :
                       frames[last].t - frames[i].t) < native) last++;
                r.count = std::min(j - i, last - i + 1);
            }
            abilityRuns.push_back({id, r});
        }
        i = j;
    }
    if (!idle.count)
        idle = {0, 1};
    return true;
}

void CharacterRecording::pose(Motion motion, double time, double distance, std::vector<glm::mat4> &out) const {
    if (frames.empty()) {
        out.clear();
        return;
    }
    const Range *r = motion == Motion::Walk && walk.count > 1 ? &walk
                     : motion == Motion::Air && air.count > 1 ? &air
                                                               : &idle;
    size_t index = r->first;
    if (r->count > 1) {
        if (r == &walk && r->distance > 0) {
            // Step through the walk by distance covered, as the game's cycle does.
            double target = std::fmod(distance, r->distance), covered = 0;
            for (index = r->first; index + 1 < r->first + r->count; index++) {
                covered += frames[index].speed * (frames[index + 1].t - frames[index].t);
                if (covered >= target)
                    break;
            }
        } else {
            double span = std::max(r->duration, 1e-3);
            double t = r == &air ? std::min(time, span) : std::fmod(time, span);
            double at = frames[r->first].t + t;
            while (index + 1 < r->first + r->count && frames[index + 1].t <= at)
                index++;
        }
    }
    out = frames[index].local;
}

std::string CharacterRecording::summary() const {
    auto range = [](const char *name, const Range &r) {
        char s[128];
        std::snprintf(s, sizeof s, "%s %zu frames (%.1f s, %.1f blocks)", name, r.count, r.duration, r.distance);
        return std::string(s);
    };
    char s[64];
    std::snprintf(s, sizeof s, "; facing %.0f deg", forwardAngle * 57.2958f);
    return std::to_string(partList.size()) + " parts, " + std::to_string(frames.size()) + " frames: " +
           range("idle", idle) + ", " + range("walk", walk) + ", " + range("air", air) + s;
}

bool CharacterRecording::abilityPose(int id, double time, std::vector<glm::mat4> &out) const {
    out.clear();
    if (!std::isfinite(time) || time < 0) return false;
    for (auto &[ability, r] : abilityRuns) {
        if (ability != id)
            continue;
        if (time >= r.duration)
            return false;
        size_t index = r.first;
        auto clock = [&](size_t i) { return r.phaseClock ? (double)frames[i].phase : frames[i].t; };
        double at = clock(r.first) + time;
        while (index + 1 < r.first + r.count && clock(index + 1) <= at)
            index++;
        out = frames[index].local;
        float blend = 0;
        if (index + 1 < r.first + r.count) {
            blend = (float)((at - clock(index)) / (clock(index + 1) - clock(index)));
            for (size_t i = 0; i < out.size(); i++)
                out[i] = blendPart(out[i], frames[index + 1].local[i], blend);
        }
        // Cyclone turns the entity itself as well as posing its limbs. Keep
        // that recorded turn after stripping ordinary world-space facing.
        if (id == 95) {
            float yaw = frames[index].yaw;
            if (blend > 0) yaw = glm::mix(yaw, frames[index + 1].yaw, blend);
            yaw -= frames[r.first].yaw;
            glm::mat4 turn = glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 0, 1));
            for (auto &m : out) m = turn * m;
        }
        return true;
    }
    return false;
}

double CharacterRecording::abilityDuration(int id) const {
    for (auto &[ability, r] : abilityRuns) if (ability == id) return r.duration;
    return 0;
}

std::vector<std::pair<int, double>> CharacterRecording::abilities() const {
    std::vector<std::pair<int, double>> list;
    for (auto &[id, r] : abilityRuns)
        list.push_back({id, r.duration});
    return list;
}

float CharacterRecording::abilitySpeed(int id, double time) const {
    if (!std::isfinite(time) || time < 0) return 0;
    for (auto &[ability, r] : abilityRuns) {
        if (ability != id || time >= r.duration)
            continue;
        size_t index = r.first;
        auto clock = [&](size_t i) { return r.phaseClock ? (double)frames[i].phase : frames[i].t; };
        double at = clock(r.first) + time;
        while (index + 1 < r.first + r.count && clock(index + 1) <= at)
            index++;
        if (index + 1 < r.first + r.count) {
            float blend = (float)((at - clock(index)) / (clock(index + 1) - clock(index)));
            return glm::mix(frames[index].speed, frames[index + 1].speed, blend);
        }
        return frames[index].speed;
    }
    return 0;
}

double CharacterRecording::abilityStart(int id) const {
    for (auto &[ability, r] : abilityRuns) {
        if (ability != id)
            continue;
        size_t first = r.first;
        while (first > 0 && frames[first - 1].segment == frames[r.first].segment)
            first--;
        return frames[r.first].t - frames[first].t;
    }
    return -1;
}

void CharacterRecording::restyle(const std::string &head, const std::string &hair, const std::string &hand,
                                 const std::string &foot, glm::u8vec3 hairColor) {
    for (auto &p : partList) {
        if (p.matrix == 3 && !head.empty())
            p.sprite = head;
        else if (p.matrix == 4 && !hair.empty())
            p.sprite = hair, p.tint = hairColor, p.hair = true;
        else if ((p.matrix == 9 || p.matrix == 10) && !hand.empty())
            p.sprite = hand;
        else if ((p.matrix == 15 || p.matrix == 16) && !foot.empty())
            p.sprite = foot;
    }
}
