#pragma once
// NPC speech and emotes, from remaining-runtime-gaps.md 5.7 and
// steam-npc-speech-rendering.md. Geometry comes from the owner's assets.
#include "DialoguePanel.h"
#include "SceneVisibility.h"
#include <array>
#include <map>

namespace npcspeech {
using Point = std::array<int64_t, 3>;
struct Actor {
    int64_t id = 0;
    Point physical{}, drawn{};
    glm::vec3 body{0};
    float yaw = 0, drop = 0, extraHeight = 0, health = 0;
    int32_t race = 0;
};
Actor actor(const uint8_t *record);
using Projection = scenevisibility::Projection;
std::optional<glm::vec2> project(Point, const Projection &, glm::vec2 screen, float guiScale);
struct Bubble {
    int64_t speaker = 0, recipient = 0;
    std::vector<TalkPlay::Page> pages;
    size_t page = 0;
    float elapsedMs = 0, displayed = 0;
    Point anchor{};
    glm::vec2 screenPoint{0};
    bool visible = false;
};
struct Emote { int model = 0; int32_t ageMs = 0; };
struct Environment {
    int64_t localPlayer = 0;
    std::function<std::optional<Actor>(int64_t)> find;
    std::function<void(const std::function<void(const Actor &)> &)> eachActor;
    std::function<bool(int)> hasModel;
    std::function<std::optional<glm::vec2>(Point)> project;
    std::function<void(int64_t speaker, int64_t recipient, const TalkPlay::Page &, bool last)> effects;
    std::function<void(int64_t speaker)> finished;
    std::function<void(int sound, const Point &)> sound;
};
class Playback {
  public:
    void attach(int64_t speaker, int64_t recipient, std::vector<TalkPlay::Page>, Point drawn);
    bool repeatUse(int64_t speaker);
    void update(double elapsedMs, const Environment &);
    bool addressedTo(int64_t recipient) const;
    const std::vector<Bubble> &bubbles() const { return content; }
    const std::map<int64_t, Emote> &emotes() const { return active; }
    std::optional<float> voice(int64_t speaker, const dialoguepanel::Layout &, std::optional<float> width,
                               const std::function<int()> &random);
  private:
    std::vector<Bubble> content;
    std::map<int64_t, Emote> active;
};
struct EmotePose { int model = 0; float scale = 1, bounce = 0, tilt = 0; };
EmotePose emotePose(Emote);
glm::mat4 emoteTransform(const Actor &, const EmotePose &, glm::ivec3 header, Point renderOffset);
using ActorSphere = scenevisibility::Sphere;
ActorSphere actorSphere(const Actor &);
} // namespace npcspeech
