#pragma once
// NPC speech and emotes, from remaining-runtime-gaps.md 5.7 and
// steam-npc-speech-rendering.md. Geometry comes from the owner's assets.
#include "DialoguePanel.h"
#include "SceneVisibility.h"
#include "CreatureScene.h"
#include <array>
#include <map>

namespace npcspeech {
using Point = std::array<int64_t, 3>;
using creaturescene::Actor;
using creaturescene::actor;
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
// The model drawn over an admitted living actor (steam-creature-overhead-models.md §1): a class
// marker, then the creature-collection marker, then the active emote, else the buff-7 heart.
struct OverheadInputs {
    uint8_t creatureClass = 0;              // record +0x14c, unsigned
    bool collected = false;                 // the actor's ID is in the creature collection
    bool selected = false;                  // ... and equals its selected ID
    const Emote *emote = nullptr;           // the actor's active emote
    std::optional<float> heartStrength;     // the first buff-7 entry's strength float
};
struct Overhead {
    EmotePose pose;          // the emote's own pose, or the model at scale multiplier
    glm::vec4 material{1};   // white, or the selected collection member's orange
};
constexpr glm::vec4 SELECTED_MATERIAL{1, .25f, 0, 1};
std::optional<Overhead> overhead(const OverheadInputs &);
using ActorSphere = scenevisibility::Sphere;
using creaturescene::actorSphere;
} // namespace npcspeech
