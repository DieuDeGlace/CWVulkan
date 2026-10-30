#include "NpcSpeech.h"
#include "GameMath.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <set>

namespace npcspeech {
namespace {
template <class T> T read(const uint8_t *record, size_t field) {
    T out;
    std::memcpy(&out, record + field, sizeof out);
    return out;
}
int64_t add(int64_t a, int64_t b) { return std::bit_cast<int64_t>((uint64_t)a + (uint64_t)b); }
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>((uint64_t)a - (uint64_t)b); }
int modelFor(const TalkPlay::Page &page) {
    if (page.words.empty()) return 0;
    const auto &word = page.words.front().text;
    if (word == L"!alert") return 0xded;
    if (word == L"!question") return 0xdec;
    if (word == L"!love") return 0xdf0;
    if (word == L"!angry") return 0xdee;
    if (word == L"!sweat") return 0xdf1;
    return 0;
}
void advance(Bubble &bubble) { ++bubble.page; bubble.elapsedMs = bubble.displayed = 0; }
glm::vec3 transform(glm::vec3 point, const glm::mat4 &matrix) {
    glm::vec4 out;
    for (int lane = 0; lane < 4; ++lane) {
        const int first = lane == 0 ? 1 : 0, second = 1 - first;
        float value = point[first] * matrix[first][lane];
        value = value + point[second] * matrix[second][lane];
        value = value + point.z * matrix[2][lane];
        out[lane] = value + matrix[3][lane];
    }
    const float inverseW = 1.f / out.w;
    return {out.x * inverseW, out.y * inverseW, out.z * inverseW};
}
}
Actor actor(const uint8_t *record) {
    Actor result;
    if (!record) return result;
    result.id = read<int64_t>(record, 8);
    result.physical = read<Point>(record, 0x10);
    result.drawn = read<Point>(record, 0xb10);
    std::memcpy(&result.body.x, record + 0x84, 12);
    result.yaw = read<float>(record, 0x30);
    result.drop = read<float>(record, 0x990);
    result.extraHeight = read<float>(record, 0x17c);
    result.health = read<float>(record, 0x180);
    result.race = read<int32_t>(record, 0x64);
    return result;
}
std::optional<glm::vec2> project(Point point, const Projection &projection, glm::vec2 screen, float guiScale) {
    glm::vec3 position;
    for (int axis = 0; axis < 3; ++axis)
        position[axis] = (float)add(point[axis], projection.offset[axis]) * (1.f / 65536.f);
    const auto view = transform(position, projection.view);
    if (!(view.z > 0)) return {};
    const auto clip = transform(view, projection.perspective);
    const float x = (clip.x * screen.x) * .5f + screen.x * .5f;
    const float y = (clip.y * -screen.y) * .5f + screen.y * .5f;
    return glm::vec2(x, std::min(std::max(y, guiScale * 180.f), screen.y));
}
void Playback::attach(int64_t speaker, int64_t recipient, std::vector<TalkPlay::Page> pages, Point drawn) {
    auto found = std::find_if(content.begin(), content.end(), [&](const Bubble &b) { return b.speaker == speaker; });
    if (found == content.end()) {
        if (pages.empty()) return;
        content.emplace_back();
        found = std::prev(content.end());
    }
    found->speaker = speaker;
    found->recipient = recipient;
    found->anchor = drawn;
    found->pages.insert(found->pages.end(), std::make_move_iterator(pages.begin()), std::make_move_iterator(pages.end()));
}
bool Playback::repeatUse(int64_t speaker) {
    for (auto &bubble : content) if (bubble.speaker == speaker) {
        const int duration = bubble.page < bubble.pages.size() ? dialoguepanel::duration(bubble.pages[bubble.page].words) : 0;
        if (bubble.elapsedMs < duration) bubble.elapsedMs = (float)duration;
        else bubble.elapsedMs += 100000.f;
        return true;
    }
    return false;
}
bool Playback::addressedTo(int64_t recipient) const {
    return std::any_of(content.begin(), content.end(), [&](const Bubble &b) { return b.recipient == recipient; });
}
void Playback::update(double elapsedMs, const Environment &environment) {
    const double step = std::min(elapsedMs, 1000.0);
    // Native timeout checks precede page effects and the current frame's increment.
    std::erase_if(content, [&](Bubble &bubble) {
        if (bubble.page < bubble.pages.size()) {
            const int duration = dialoguepanel::duration(bubble.pages[bubble.page].words);
            if (bubble.elapsedMs > std::max((int)(1.75 * duration), duration + 1000)) advance(bubble);
        }
        if (bubble.page < bubble.pages.size()) return false;
        if (environment.find && environment.find(bubble.speaker) && bubble.speaker != environment.localPlayer && environment.finished)
            environment.finished(bubble.speaker);
        return true;
    });
    const int64_t fraction = (int64_t)(gamemath::blendFactor(step, .01f) * 65536.f);
    for (auto &bubble : content) {
        if (bubble.elapsedMs == 0 && bubble.page < bubble.pages.size()) {
            const auto &page = bubble.pages[bubble.page];
            const int model = modelFor(page);
            if (model) {
                if (environment.hasModel && environment.hasModel(model)) active[bubble.speaker] = {model,0};
                else active.erase(bubble.speaker);
            }
            if (environment.effects) environment.effects(bubble.speaker, bubble.recipient, page, bubble.page + 1 == bubble.pages.size());
            if (model) advance(bubble);
        }
        const auto speaker = environment.find ? environment.find(bubble.speaker) : std::nullopt;
        if (!speaker) continue; // retain the previous root placement/visibility
        Point target = speaker->drawn;
        target[2] = add(target[2], (int64_t)(-speaker->drop * 65536.f));
        for (int axis = 0; axis < 3; ++axis) {
            const int64_t difference = subtract(target[axis], bubble.anchor[axis]);
            const int64_t product = std::bit_cast<int64_t>((uint64_t)difference * (uint64_t)fraction);
            bubble.anchor[axis] = add(bubble.anchor[axis], product / 65536);
        }
        Point anchor = bubble.anchor;
        anchor[2] = add(anchor[2], (int64_t)((speaker->body.z * .5f + 1.f) * 65536.f));
        if (environment.project) {
            const auto point = environment.project(anchor);
            bubble.visible = point.has_value();
            if (point) bubble.screenPoint = *point;
        }
        if (active.count(bubble.speaker)) bubble.visible = false;
    }
    if (!active.empty() && environment.eachActor) {
        std::set<int64_t> retained;
        environment.eachActor([&](const Actor &entry) {
            const auto found = active.find(entry.id);
            if (found == active.end()) return;
            retained.insert(entry.id);
            auto &emote = found->second;
            if (emote.ageMs == 0 && environment.sound) environment.sound(0x3c, entry.physical);
            emote.ageMs = (int32_t)((double)emote.ageMs + step);
            if (emote.ageMs > 2000) active.erase(found);
        });
        std::erase_if(active, [&](const auto &entry) { return !retained.count(entry.first); });
    }
    for (auto &bubble : content) {
        const bool resolved = environment.find && environment.find(bubble.speaker).has_value();
        if (resolved && active.count(bubble.speaker)) bubble.elapsedMs = 0;
        else bubble.elapsedMs = (float)((double)bubble.elapsedMs + step);
    }
}
std::optional<float> Playback::voice(int64_t speaker, const dialoguepanel::Layout &layout, std::optional<float> width,
                                    const std::function<int()> &random) {
    for (auto &bubble : content) if (bubble.speaker == speaker)
        return dialoguepanel::voice(bubble.displayed, layout, width, random);
    return {};
}
EmotePose emotePose(Emote emote) {
    EmotePose result;
    result.model = emote.model;
    if (emote.ageMs < 200) {
        const double phase = (double)((float)emote.ageMs / 200.f) * 3.141592653589793;
        result.bounce = .5f * std::sin((float)phase);
        result.scale = std::sin((float)(phase * (double).6f)) / std::sin(1.8849556446075439f);
    }
    if (emote.ageMs > 1800) result.scale = 1.f - (float)(emote.ageMs - 1800) / 200.f;
    if (emote.model == 0xdf1) result.bounce += .5f - (float)emote.ageMs * .0005f;
    if (emote.model == 0xdee || emote.model == 0xdef) {
        result.tilt = 45.f + 10.f * std::sin((float)emote.ageMs * .01f);
        result.model = (emote.ageMs / 250) % 2 == 0 ? 0xdee : 0xdef;
    }
    return result;
}
glm::mat4 emoteTransform(const Actor &actor, const EmotePose &pose, glm::ivec3 header, Point renderOffset) {
    Point at = actor.physical;
    const float height = ((actor.body.z * .5f + 1.f) - actor.drop) + pose.bounce;
    at[2] = add(at[2], (int64_t)(height * 65536.f));
    glm::vec3 anchor;
    for (int axis = 0; axis < 3; ++axis) anchor[axis] = (float)add(at[axis], renderOffset[axis]) * (1.f/65536.f);
    auto matrix = glm::translate(glm::mat4(1), anchor);
    matrix = glm::rotate(matrix, actor.yaw * .0174532924f, {0,0,1});
    matrix = glm::rotate(matrix, pose.tilt * .0174532924f, {0,1,0});
    const float scale = (.8f / (float)header.x) * pose.scale;
    matrix = glm::scale(matrix, glm::vec3(scale));
    return glm::translate(matrix, glm::vec3(header) * -.5f);
}
ActorSphere actorSphere(const Actor &actor) {
    ActorSphere result{actor.drawn, std::max(actor.body.z, .6f * std::max(actor.body.x, actor.body.y))};
    if (actor.race == 0x12e) {
        result.centre[2] = add(result.centre[2], (int64_t)(actor.extraHeight * .5f * 65536.f));
        result.radius += actor.extraHeight * 1.2f;
    } else result.centre[2] = add(result.centre[2], (int64_t)(-actor.drop * 65536.f));
    return result;
}
} // namespace npcspeech
