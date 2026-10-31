// Running the front screens (Frontend.h): the title scene over the ordinary world, the root
// menu, character select and creation, the options, controls and help screens, and the system
// menu over the running game. Written from docs/specs/frontend-title.md,
// frontend-characters.md and frontend-options.md.
#include "Frontend.h"

#include "Assets.h"
#include "Audio.h"
#include "Character.h"
#include "CubModel.h"
#include "Extracted.h"
#include "GameInput.h"
#include "GameMath.h"
#include "MapLook.h"
#include "PanelScroll.h"
#include "NativePlayer.h"
#include "NativeRules.h"
#include "Spawned.h"
#include "SteamSession.h"
#include "World.h"
#include "worldgen/CreatureTables.h"
#include "worldgen/Generator.h"
#include "worldgen/Looks.h"
#include "worldgen/Regions.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>

namespace fs = std::filesystem;

namespace frontend {

namespace {

// --- Constants of the screens ----------------------------------------------------------------

const glm::vec4 WHITE(1), HOVERED(0.2f, 1, 1, 1), ARROW_HOVERED(0, 1, 1, 1), BLACK(0, 0, 0, 1);
const glm::vec4 NAME_COLOUR(0.25f, 1, 1, 1), SPECIALIZATION_COLOUR(0.5f, 0.5f, 1, 1);
const glm::vec4 UNASSIGNED_COLOUR(0.2f, 1, 0.2f, 1), WAITING_COLOUR(1, 0.35f, 0.35f, 1);
constexpr int ALIGN_LEFT = 0, ALIGN_CENTRE = 1, ALIGN_RIGHT = 2, ALIGN_BOTH = 5;

// The sounds of the menus (frontend-title.md §5.2).
constexpr int SOUND_MENU_OPEN = 0x56, SOUND_MENU_CLOSE = 0x57, SOUND_MENU_SELECT = 0x58;

// The ability of rising on the eagle, and the record fields the title sets on the player.
constexpr uint8_t EAGLE_RISE = 0x9d;
constexpr size_t ABILITY = 0x68, TOGGLES = 0x128, HEALTH = 0x180, FLIGHT_POINT = 0x1b0, FLIGHT_START = 0x1b8;
constexpr uint32_t FRONT_BIT = 0x80, LANTERN_BIT = 0x1;
constexpr float NIGHT_FROM = 19.0f, NIGHT_UNTIL = 5.0f; // the lantern on the title (hours)

// Buttons and sizes the spec gives (GUI units).
constexpr glm::vec2 SELECT_SIZE{200, 30}, DELETE_TOGGLE_SIZE{20, 20}, DELETE_SIZE{300, 30}, BACK_SIZE{200, 40};
constexpr glm::vec2 CREATE_SIZE{300, 40}, OPTIONS_BUTTON_SIZE{110, 15}, CONTROLS_BUTTON_SIZE{140, 15};
// The multiplayer menu (netcode-transport.md §3). The spec gives its contents but not its
// geometry: the panel, rows and buttons are placed by CubeVulkan.
constexpr glm::vec2 FRIENDS_SIZE{420, 340}, FRIEND_BUTTON_SIZE{70, 15};
constexpr float FRIENDS_TITLE_Y = 30, FRIENDS_FIRST_ROW_Y = 60, FRIEND_ROW_HEIGHT = 24;
const glm::vec4 FRIEND_JOINED_COLOUR(0.4f, 1, 0.4f, 1), FRIEND_INVITED_COLOUR(1, 1, 0.4f, 1);
const glm::vec4 FRIEND_INVITATION_COLOUR(1, 0.5f, 0.2f, 1); // the colour of "You were invited by"
// Placed by CubeVulkan where the spec leaves the place open (see docs/menu.md).
constexpr glm::vec2 BACK_AT{20, 20};
constexpr float LOOK_VALUE_X = 190, LOOK_LEFT_ARROW_X = 105, LOOK_RIGHT_ARROW_X = 255;
constexpr float NAME_FIELD_FROM_BOTTOM = 50, CREATE_FROM_BOTTOM = 20;

// The character list's creatures (frontend-characters.md §2.2): drawn at (100, 110) in their
// entry, turned (-95, 0, 45) degrees; while hovered the yaw goes down 0.05 degrees a ms and the
// creature walks on the spot.
constexpr glm::vec2 PREVIEW_AT{100, 110};
constexpr float PREVIEW_TILT_DEGREES = 95.0f, PREVIEW_TURN_DEGREES = 45.0f;
constexpr float PREVIEW_YAW_PER_MS = 0.05f;
constexpr float PREVIEW_WALK_SPEED = 8.0f; // blocks a second: the movement intent (0, 8, 0)
// Placed by CubeVulkan: the entry's screen scale (blocks to interface units) and the ease
// toward the walk and back to rest (the rate of the upkeep's smoothing, not given for this).
constexpr float PREVIEW_UNITS_PER_BLOCK = 80.0f, PREVIEW_EASE_RATE = 0.1f;
constexpr float PREVIEW_STANDING_PACE = 0.05f; // below this the creature stands
constexpr float PREVIEW_FEET_BELOW = 1.0f;     // the drawn point is the record's place, a block above the feet
constexpr int START_SEARCH_STEPS = 64 * 4000;  // cells walked before giving up (4000 areas)

std::wstring wide(const std::string &s) {
    std::wstring w;
    for (unsigned char c : s)
        w.push_back((wchar_t)c);
    return w;
}

bool inside(glm::vec2 p, glm::vec2 at, glm::vec2 size) {
    return p.x >= at.x && p.y >= at.y && p.x < at.x + size.x && p.y < at.y + size.y;
}

Hud::ScreenItem text(const std::string &s, glm::vec2 at, float size, float outline, glm::vec4 colour, int align) {
    Hud::ScreenItem t;
    t.kind = Hud::ScreenItem::Text;
    t.text = wide(s), t.at = at, t.fontSize = size, t.outline = outline, t.color = colour, t.align = align;
    return t;
}
Hud::ScreenItem square(glm::vec2 at, glm::vec2 size, glm::vec4 colour) {
    Hud::ScreenItem q;
    q.kind = Hud::ScreenItem::Square;
    q.at = at, q.size = size, q.color = colour;
    return q;
}
Hud::ScreenItem widget(const std::string &name, glm::vec2 at, glm::vec2 size, const std::string &label = "",
                       glm::vec4 tint = WHITE) {
    Hud::ScreenItem w;
    w.kind = Hud::ScreenItem::Template;
    w.name = name, w.at = at, w.size = size, w.text = wide(label), w.color = tint;
    return w;
}
Hud::ScreenItem characterCard(glm::vec2 at) {
    auto item = widget("blackwidget", at, ENTRY_SIZE);
    item.replaceWidget = true;
    return item;
}

// A record for the native player from a saved character (race, class, level, gear, looks).
worldgen::CreatureRecord recordOf(const nativesaves::Character &c, const std::array<uint8_t, 0xb0> *appearance) {
    nativeplayer::Who who;
    who.race = std::max(0, nativesaves::raceOf(c.looks.people(), c.looks.sex()));
    who.cls = c.classType, who.spec = c.specialization, who.level = std::max(1, c.level);
    std::array<nativeplayer::Item, 11> worn{};
    for (size_t k = 0; k < worn.size(); k++)
        std::memcpy(worn[k].data(), c.equipment[k].data(), 0xa0);
    worldgen::CreatureRecord r = nativeplayer::record(who, worn);
    if (appearance)
        std::memcpy(&r[worldgen::creature::APPEARANCE], appearance->data(), appearance->size());
    return r;
}

// The game's display modes: 32-bit modes grouped by size, each with its refresh rates.
std::vector<DisplayMode> displayModes() {
    std::vector<DisplayMode> out;
    int count = 0;
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *modes = monitor ? glfwGetVideoModes(monitor, &count) : nullptr;
    for (int i = 0; i < count; i++) {
        const GLFWvidmode &m = modes[i];
        if (m.redBits + m.greenBits + m.blueBits < 24)
            continue;
        auto same = std::find_if(out.begin(), out.end(), [&](const DisplayMode &d) { return d.width == m.width && d.height == m.height; });
        if (same == out.end())
            same = out.insert(out.end(), DisplayMode{m.width, m.height, {}});
        if (std::find(same->rates.begin(), same->rates.end(), m.refreshRate) == same->rates.end())
            same->rates.push_back(m.refreshRate);
    }
    for (DisplayMode &d : out)
        std::sort(d.rates.begin(), d.rates.end());
    return out;
}

uint32_t wallClockMs() {
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// A model drawn in screen space (the logo, the list's creatures): meshed as the game meshes
// every voxel model (worldmap-rendering.md §2: each face its own corners, the 6 x 6 corner
// occlusion, .cub colours halved), so each voxel keeps its own colour and an open face is flat.
// The colours are doubled back as the model shader does (§3.2) and shaded once face by face
// (screenModelShade): the overlay pass's own light is open question 1 of frontend-title.md.
// The mesh's corner is the model's corner, as the part matrices take it.
uint32_t screenModel(Renderer &renderer, const CubModel &cub, int facing = -1) {
    Mesh m;
    worldmap::meshModel(worldmap::VoxelModel{cub.sx, cub.sy, cub.sz, cub.rgb.data()}, 0, cub.sx - 1, 0, cub.sy - 1, true, m);
    for (Vertex &v : m.vertices) {
        const int *n = FACE_NORMALS[v.normal];
        const int axis = n[0] ? 0 : n[1] ? 1 : 2, sign = n[0] + n[1] + n[2];
        const float shade = 2.0f * screenModelShade(axis, sign, facing);
        v.r = (uint8_t)std::min(255.0f, v.r * shade), v.g = (uint8_t)std::min(255.0f, v.g * shade),
        v.b = (uint8_t)std::min(255.0f, v.b * shade);
        v.flags = NATIVE_MENU;
    }
    return renderer.upload(m);
}

} // namespace

// --- The session: everything the screens keep while they run ---------------------------------

class Session {
  public:
    Session(Services &services, const Request &request) : s(services), req(request), hud(services.hud ? *services.hud : ownHud) {}
    bool start(std::string &error);
    void finish();
    Result runTitle();
    // In play (InGameMenus): the screens over the running game.
    bool startInGame(std::string &error);
    Result::Action gameFrame(double dtMs);
    void gameDraw(std::vector<OverlayVertex> &out);
    bool anyOpen() const { return systemShown || optionsOpen || controlsOpen || helpOpen || friendsOpen; }
    bool ordinaryUi() const { return systemShown || optionsOpen || controlsOpen || friendsOpen; }
    bool capturingControl() const { return controlsOpen && controls.waiting >= 0; }
    bool wheel(int nativeDelta, glm::vec2 cachedPointer);
    void hideSystemForMovement() { systemShown = false; }
    void hideMultiplayerForMovement() { friendsOpen = false; }
    void dismissMenus() {
        while (anyOpen())
            back(true);
    }
    void toggleSystemMenu() { systemShown = !systemShown; }
    void toggleMultiplayerMenu();
    bool interfaceVisible() const { return interfaceShown; }
    Services &services() { return s; }

  private:
    Services &s;
    Request req;
    Hud ownHud;
    Hud &hud;
    VkExtent2D size{0, 0};
    float guiScale = 1;
    glm::vec2 window{0}, mouse{0}; // GUI units
    glm::vec2 cardFrame{0}, selectFrame{0}, deleteToggleFrame{0}, deleteFrame{0};
    bool clicked = false;
    bool pointerDown = false, pointerUp = false, leftHeld = false;
    bool capturingAtPoll = false;
    std::vector<gamepadui::Point> navigationPoints;
    void navigationPoint(glm::vec2 at) {
        int width, height;
        glfwGetWindowSize(s.window, &width, &height);
        navigationPoints.push_back({(int32_t)(at.x * guiScale * (float)width / (float)std::max(1u, size.width)),
                                    (int32_t)(at.y * guiScale * (float)height / (float)std::max(1u, size.height))});
    }
    double dtMs = 0;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    double seconds = 0, nextSteamCheck = 0;
    bool testClicked = false; // Request::clickAfter's click was made
    uint32_t previewPanelMesh = 0;
    float previewPanelScale = 0;

    // The front layers (frontend-title.md §2) and the screens over them.
    bool titleShown = true, charactersShown = false, creationShown = false;
    bool optionsOpen = false, controlsOpen = false, helpOpen = false, interfaceShown = true;
    bool riding() const { return titleShown || charactersShown; }
    bool front() const { return titleShown || charactersShown || creationShown; }

    // The characters of characters.db and the list's state.
    // Each listed character's creature as the entry draws it: its recording wearing its look,
    // one screen model a part, and how far it has turned and walked.
    struct Preview {
        CharacterRecording recording;
        std::vector<uint32_t> meshes; // by part; 0 for none
        double clockMs = 0;           // its animation clock
        double walked = 0;            // blocks walked on the spot
        float yaw = 0;                // degrees turned
        float pace = 0;               // 0 standing .. 1 walking
        bool loaded = false;
    };
    struct Listed {
        nativesaves::Character character;
        hair::Style hairStyle;
        CharacterEntry entry;
        bool playable = false;
        Preview preview;
    };
    std::vector<Listed> listed;
    int cursor = -1;           // the selected entry (G+0x6c0); listed.size() is "New character"
    int selectedSlot = -1;     // the character on stage (G+0x2264)
    bool deleteShown = false;
    std::vector<CharacterFrame> cards;
    float cardScale = 0;
    CharacterScroll characterScroll;
    bool pointerClockStarted = false;
    void layoutCharacterFrames();

    // The player on stage: the native player when the game's creatures run, else a stand-in.
    nativesaves::Character stage;
    std::array<uint8_t, 0xb0> stageLooks{};
    bool stageHasLooks = false;
    glm::vec3 riderFeet{0}; // local blocks (the stand-in's)
    OrbitCamera camera;
    float hour = 10; // set from Services::startHour when the session starts

    // Character creation.
    Look look;
    HairEditor hairEditor;
    hair::Style appliedHairStyle;
    std::string name;
    bool previewing = false;

    OptionsScreen options;
    ControlsScreen controls;

    // The logo, as a screen-space model.
    uint32_t logoMesh = 0;
    glm::vec3 logoSize{0};

    int settled = 0; // frames with the world loaded, for a capture

    // --- Steps ---
    void loadList();
    void makePreviews();
    void releasePreviews();
    void previewItems(std::vector<DrawItem> &opaque);
    void previewPanels(std::vector<DrawItem> &opaque);
    void putOnStage(const nativesaves::Character &c, bool riding);
    void enterWorld(); // Continue, Select, Create Character: the player rebuilt from the save
    void stageDefaultPlayer();
    void applyLook(bool preview);
    void applyHairStyle();
    void secondHairItems(Hud::Screen &out);
    void startCreation();
    nativesaves::Cell startCell(std::array<int64_t, 3> &position) const;
    uint32_t account() const;
    bool frame();
    void readInput(bool poll, double frameMs = 0);
    void handleInterfaceKeys(std::vector<int> &keys);
    void handleKeys(bool inGame);
    void handleClick(bool inGame);
    void stepWorld(Camera &cam, Lighting &light, glm::vec3 &focus, std::vector<DrawItem> &opaque, std::vector<DrawItem> &water);
    Hud::Screen buildScreen(bool inGame);
    void titleItems(Hud::Screen &out);
    void characterItems(Hud::Screen &out);
    void creationItems(Hud::Screen &out);
    void optionsItems(Hud::Screen &out);
    void controlsItems(Hud::Screen &out);
    void helpItems(Hud::Screen &out);
    void systemItems(Hud::Screen &out);
    void openOptions();
    void applyOptions();
    void back(bool inGame);
    void playSound(int number) {
        if (s.externalMenuSounds && (number == SOUND_MENU_OPEN || number == SOUND_MENU_CLOSE))
            return;
        if (s.audio)
            s.audio->playInterface(number);
    }
    void log(const std::string &line) const {
        if (s.log)
            s.log(line);
    }
    glm::vec2 entryAt(int i) const { return i >= 0 && i < (int)cards.size() ? cards[(size_t)i].at / guiScale : entryPosition(i); }
    Hud::ScreenItem characterControl(const std::string &name, glm::vec2 at, glm::vec2 size, const std::string &caption = "") const {
        auto item = widget(name, at, size, caption);
        item.instance = "characters/" + name;
        item.initializeHidden = name != "button2";
        return item;
    }
    CharacterButtons buttonsAt(int i) const {
        const auto select = characterControl("button2", {}, SELECT_SIZE);
        const auto toggle = characterControl("smallbutton", {}, DELETE_TOGGLE_SIZE);
        const auto size = i >= 0 && i < (int)cards.size() ? cards[(size_t)i].size : cardFrame * guiScale;
        auto buttons = characterButtons(entryAt(i) * guiScale, size, hud.templateViewSize(select), hud.templateViewSize(toggle));
        buttons.select /= guiScale;
        buttons.deleteToggle /= guiScale;
        buttons.deleteCharacter /= guiScale;
        return buttons;
    }
    glm::vec2 lookPanelAt() const { return {0, window.y - LOOK_PANEL_SIZE.y}; }
    glm::vec2 centred(glm::vec2 widgetSize) const { return (window - widgetSize) * 0.5f; }
    glm::vec2 templateSize(const std::string &n, glm::vec2 fallback) {
        const glm::vec2 v = hud.templateSize(n);
        return v.x > 0 && v.y > 0 ? v : fallback;
    }
    // The system menu, and what the screens decided.
    bool systemShown = false;
    // The multiplayer menu: open, and its rows as taken when it opened.
    bool friendsOpen = false;
    std::vector<FriendRow> friendRows;
    int32_t firstFriend = 0;
    std::optional<panelscroll::Thumb> controlsThumb, friendsThumb;
    glm::vec2 controlsThumbFrame{-1}, friendsThumbFrame{-1};
    void refreshControlsThumb();
    void refreshFriendsThumb();
    void scrollControls(int nativeDelta);
    void scrollFriends(int nativeDelta);
    void refreshFriendStates();
    int hoveredFriend(glm::vec2 m) const; // the row under the pointer (panel units), -1 if none
    glm::vec2 friendButtonAt(int row, bool right) const;
    void friendsItems(Hud::Screen &out);
    bool friendsClick();
    bool done = false, playAfterHelp = false;
    Result outcome;
    void decide(Result::Action action, int slot = -1) {
        done = true;
        outcome.action = action;
        if (slot >= 0)
            outcome.slot = slot;
    }
    void closeHelp() {
        helpOpen = false;
        if (playAfterHelp)
            decide(Result::Play);
    }
};

bool Session::start(std::string &error) {
    if (!hud.load(s.assets->gameDir(), error))
        return false;
    hud.setGuiScale(s.options ? std::clamp(s.options->guiScale, gameoptions::GUI_SCALE_MIN, gameoptions::GUI_SCALE_MAX) : 1.0f);
    cardFrame = hud.templateSize("blackwidget", ENTRY_SIZE, true);
    selectFrame = hud.templateSize("button2", SELECT_SIZE);
    deleteToggleFrame = hud.templateSize("smallbutton", DELETE_TOGGLE_SIZE);
    deleteFrame = hud.templateSize("button", DELETE_SIZE);
    for (const auto size : {cardFrame, selectFrame, deleteToggleFrame, deleteFrame})
        if (size == glm::vec2(0)) {
            error = "Character selection requires the owner's blackwidget/button2/smallbutton/button templates.";
            return false;
        }
    log("Characters: requested Select 200x30, owner effective frame " + std::to_string(selectFrame.x) + "x" +
        std::to_string(selectFrame.y));
    // The title shows the live world at its own time of day (frontend-title.md §1, §3.2).
    hour = std::fmod(std::max(0.0f, s.startHour), 24.0f);
    if (s.controls)
        controls.values = *s.controls;
    // The logo model, lit once and drawn in screen space.
    if (auto path = s.assets->resolve(fs::path("sprites") / "cubeworld.cub")) {
        CubModel cub;
        if (cub.load(*path)) {
            logoSize = glm::vec3((float)cub.sx, (float)cub.sy, (float)cub.sz);
            logoMesh = screenModel(*s.renderer, cub, +1); // (its +z side faces the viewer)
        }
    }
    loadList();
    // The last character is put on stage (frontend-title.md §1 step 6).
    const std::optional<int> slot = s.store && s.store->isOpen() ? s.store->slot() : std::nullopt;
    if (slot && *slot >= 0 && *slot < (int)listed.size()) {
        selectedSlot = *slot;
        cursor = *slot;
        putOnStage(listed[(size_t)*slot].character, true);
    } else
        stageDefaultPlayer();
    camera.set(90, 0, 180, 8);
    playSound(SOUND_MENU_OPEN);
    return true;
}

void Session::finish() {
    if (logoMesh)
        s.renderer->release(logoMesh);
    logoMesh = 0;
    if (previewPanelMesh)
        s.renderer->release(std::exchange(previewPanelMesh, 0));
    releasePreviews();
}

// The interface keys work in play whatever is open (frontend-options.md §3); the screens are
// the system menu's.
bool Session::startInGame(std::string &error) {
    if (!s.hud && !hud.load(s.assets->gameDir(), error))
        return false;
    titleShown = charactersShown = creationShown = false;
    if (s.controls)
        controls.values = *s.controls;
    size = s.renderer->screenSize();
    return true;
}

void Session::loadList() {
    releasePreviews();
    listed.clear();
    cards.clear();
    cardScale = 0;
    if (s.store && s.store->isOpen())
        for (int i = 0; i < s.store->count(); i++) {
            Listed l;
            s.store->load(i, l.character);
            l.hairStyle = s.importedHairStyle ? s.importedHairStyle(l.character.name) : hair::Style{};
            std::string notes;
            SaveGame metadata;
            if (s.store->readNotes(i, notes) && metadata.parse(notes) && metadata.hairStyle)
                l.hairStyle = *metadata.hairStyle;
            l.playable = s.recorded && s.sprites && nativeprogress::entryFor(l.character, i, *s.recorded, *s.sprites, l.entry);
            listed.push_back(std::move(l));
        }
    makePreviews();
}

// Each character's creature for its entry: the recording of its class wearing its look, one
// screen model a part (the game draws the creature itself into the interface, 0xdb630).
void Session::makePreviews() {
    for (Listed &l : listed) {
        if (!l.playable)
            continue;
        Preview &p = l.preview;
        std::string why;
        if (!p.recording.load(l.entry.dir, why))
            continue;
        p.recording.restyle(l.entry.head, l.entry.hair, l.entry.hand, l.entry.foot, l.entry.hairColor);
        for (const CharacterRecording::Part &part : p.recording.parts()) {
            CubModel cub;
            const auto path = s.assets->resolve(fs::path("sprites") / (part.sprite + ".cub"));
            if (!path || !cub.load(*path) || !cub.sx) {
                p.meshes.push_back(0);
                continue;
            }
            if (part.hair)
                World::tintHair(cub, part.tint, l.hairStyle.dual ? &l.hairStyle : nullptr);
            p.meshes.push_back(screenModel(*s.renderer, cub));
        }
        p.loaded = true;
    }
    size = s.renderer->screenSize();
    hud.layout(*s.renderer, size);
}

void Session::releasePreviews() {
    for (Listed &l : listed)
        for (uint32_t &mesh : l.preview.meshes)
            if (mesh)
                s.renderer->release(std::exchange(mesh, 0));
}

// The preview walks on the spot: its movement intent eases toward (0, 8, 0) but its velocity
// toward zero (frontend-characters.md §2.2), so its legs and arms step without the forward lean of
// a body that is really running. The recorded walk is a run, leaning into its speed: the whole
// pose is turned back so the body (its chest part) stands as in the idle pose, the limbs keeping
// their swing about it.
namespace {
void standUpright(const CharacterRecording &recording, double seconds, std::vector<glm::mat4> &pose) {
    constexpr int BODY_MATRIX = 2; // the chest
    size_t body = recording.parts().size();
    for (size_t k = 0; k < recording.parts().size(); k++)
        if (recording.parts()[k].matrix == BODY_MATRIX)
            body = k;
    std::vector<glm::mat4> idle;
    recording.pose(CharacterRecording::Motion::Idle, seconds, 0, idle);
    if (body >= pose.size() || body >= idle.size() || pose[body][3][3] == 0 || idle[body][3][3] == 0)
        return;
    auto rotationOf = [](const glm::mat4 &m) {
        return glm::mat3(glm::normalize(glm::vec3(m[0])), glm::normalize(glm::vec3(m[1])), glm::normalize(glm::vec3(m[2])));
    };
    const glm::mat3 back = rotationOf(idle[body]) * glm::transpose(rotationOf(pose[body]));
    const glm::vec3 pivot(pose[body][3]);
    const glm::mat4 correction = glm::translate(glm::mat4(1), pivot) * glm::mat4(back) * glm::translate(glm::mat4(1), -pivot);
    for (glm::mat4 &m : pose)
        if (m[3][3] != 0)
            m = correction * m;
}
} // namespace

// The entries' creatures (frontend-characters.md §2.2). The one under the mouse turns 50 degrees
// a second and walks on the spot; the others ease back to rest. Each is drawn at (100, 110) in
// its entry, turned (-95, 0, 45) degrees and its own yaw.
// Reuse the owner's rounded interface frame. It belongs behind the preview creatures;
// drawing it with the final text overlay would darken the creatures as well as the world.
// The reference confirms the translucent panel, but not its exact native opacity.
void Session::previewPanels(std::vector<DrawItem> &opaque) {
    if (previewPanelMesh && previewPanelScale != guiScale)
        s.renderer->release(std::exchange(previewPanelMesh, 0));
    if (!previewPanelMesh) {
        std::vector<OverlayVertex> vertices;
        hud.drawScreen({characterCard({0, 0})}, vertices);
        Mesh panel;
        for (const OverlayVertex &v : vertices) {
            panel.vertices.push_back({v.x, v.y, 0, v.r, v.g, v.b, v.a, 4, NATIVE_MENU, 0, 0});
            panel.indices.push_back((uint32_t)panel.indices.size());
        }
        if (panel.indices.empty())
            return;
        previewPanelMesh = s.renderer->upload(panel);
        previewPanelScale = guiScale;
    }
    for (int i = 0; i <= (int)listed.size(); i++) {
        const glm::vec2 at = entryAt(i) * guiScale;
        DrawItem panel;
        panel.mesh = previewPanelMesh;
        panel.model[0][0] = 2.f / size.width;
        panel.model[1][1] = 2.f / size.height;
        panel.model[3] = glm::vec4(at.x * 2.f / size.width - 1, at.y * 2.f / size.height - 1, .99f, 1);
        panel.flags = NATIVE_MENU | MENU_BACKGROUND;
        panel.castShadow = false;
        opaque.push_back(panel);
    }
}

void Session::previewItems(std::vector<DrawItem> &opaque) {
    const glm::vec2 px((float)size.width, (float)size.height);
    const float ease = gamemath::blendFactor(dtMs, PREVIEW_EASE_RATE);
    for (int i = 0; i < (int)listed.size(); i++) {
        Preview &p = listed[(size_t)i].preview;
        if (!p.loaded)
            continue;
        const glm::vec2 at = entryAt(i);
        const bool hovered = inside(mouse, at, ENTRY_SIZE);
        p.clockMs += dtMs;
        if (hovered)
            p.yaw -= (float)dtMs * PREVIEW_YAW_PER_MS;
        else
            p.yaw += (0.0f - p.yaw) * ease;
        p.pace += ((hovered ? 1.0f : 0.0f) - p.pace) * ease;
        p.walked += p.pace * PREVIEW_WALK_SPEED * dtMs / 1000.0;
        std::vector<glm::mat4> pose;
        const bool walking = p.pace > PREVIEW_STANDING_PACE;
        p.recording.pose(walking ? CharacterRecording::Motion::Walk : CharacterRecording::Motion::Idle, p.clockMs / 1000.0, p.walked, pose);
        if (walking)
            standUpright(p.recording, p.clockMs / 1000.0, pose);
        // The body frame turned to face the viewer (+x toward it), then the entry's 45 degrees
        // and its own yaw; tipped 95 degrees about x so its up is the screen's up, a little from
        // above.
        const float facing = glm::radians(PREVIEW_TURN_DEGREES + p.yaw) - p.recording.forward();
        const float lean = glm::radians(PREVIEW_TILT_DEGREES - 90.0f);
        glm::mat4 view(0.0f);
        // Screen right is the body's left (+y), up its up (+z), toward the viewer its forward (+x).
        view[0] = glm::vec4(0, 0, 1, 0);  // body x -> depth toward the viewer
        view[1] = glm::vec4(1, 0, 0, 0);  // body y -> right
        view[2] = glm::vec4(0, 1, 0, 0);  // body z -> up
        view[3] = glm::vec4(0, -PREVIEW_FEET_BELOW, 0, 1);
        const glm::mat4 tip = glm::rotate(glm::mat4(1), lean, glm::vec3(1, 0, 0));
        const glm::mat4 turn = glm::rotate(glm::mat4(1), facing, glm::vec3(0, 0, 1));
        // Interface units to clip space: x right, y down, depth kept small inside the range.
        const glm::vec2 centre = (at + PREVIEW_AT) * guiScale;
        const float pixels = PREVIEW_UNITS_PER_BLOCK * guiScale;
        glm::mat4 toClip(1);
        toClip[0][0] = pixels * 2.0f / std::max(1.0f, px.x);
        toClip[1][1] = -pixels * 2.0f / std::max(1.0f, px.y);
        toClip[2][2] = -0.01f;
        toClip[3] = glm::vec4(centre.x / std::max(1.0f, px.x) * 2.0f - 1.0f, centre.y / std::max(1.0f, px.y) * 2.0f - 1.0f, 0.5f, 1.0f);
        for (size_t k = 0; k < p.meshes.size() && k < pose.size(); k++) {
            if (!p.meshes[k] || pose[k][3][3] == 0)
                continue;
            DrawItem d;
            d.mesh = p.meshes[k];
            d.model = toClip * tip * view * turn * pose[k];
            d.flags = NATIVE_MENU;
            d.castShadow = false;
            opaque.push_back(d);
        }
    }
}

void Session::putOnStage(const nativesaves::Character &c, bool ride) {
    stage = c;
    appliedHairStyle = s.importedHairStyle ? s.importedHairStyle(c.name) : hair::Style{};
    for (const auto &listedCharacter : listed)
        if (&listedCharacter.character == &c) {
            appliedHairStyle = listedCharacter.hairStyle;
            break;
        }
    if (s.spawned)
        s.spawned->setPlayerHairStyle(appliedHairStyle);
    stageHasLooks = nativeprogress::appearanceOf(c.looks, stageLooks);
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    riderFeet = c.position == std::array<int64_t, 3>{0, 0, 0} && s.extracted ? s.extracted->home()
                                                                             : nativeprogress::localFeet(c.position, frame);
    if (s.spawned && s.extracted) {
        s.spawned->setPlayerGear(recordOf(c, stageHasLooks ? &stageLooks : nullptr));
        s.spawned->setPlayer(*s.extracted, riderFeet, 0, glm::vec3(0), std::max(1.0f, c.health), std::max(1, c.level), false);
        if (nativeai::Creature *p = s.spawned->localPlayer()) {
            p->record[ABILITY] = ride ? EAGLE_RISE : 0;
            const float most = nativerules::maxHealth(p->record);
            std::memcpy(p->record + HEALTH, &most, 4);
        }
    }
}

// The default player of start-up (frontend-title.md §1 step 5): "Wollay", race 0x101, a level 1
// warrior with two iron swords and the race's own looks. CubeVulkan puts it at the world's home
// rather than at (0, 0, 0), where an extracted world has nothing to show.
void Session::stageDefaultPlayer() {
    appliedHairStyle = {};
    if (s.spawned)
        s.spawned->setPlayerHairStyle(appliedHairStyle);
    nativesaves::Character c = newCharacter({0, 0}, {0, 0, 0});
    c.name = "Wollay";
    stage = c;
    riderFeet = s.extracted ? s.extracted->home() : glm::vec3(0);
    stageHasLooks = false;
    if (s.spawned && s.extracted) {
        worldgen::CreatureRecord r = recordOf(c, nullptr);
        constexpr int32_t DEFAULT_RACE = 0x101;
        std::memcpy(&r[worldgen::creature::RACE], &DEFAULT_RACE, 4);
        if (worldgen::creatureTablesLoaded()) {
            auto none = [] { return 0; };
            worldgen::Appearance a = worldgen::Appearance::constructed();
            if (worldgen::makeLooks(worldgen::sharedCreatureTables(), DEFAULT_RACE, false, worldgen::Draw(none), a))
                std::memcpy(&r[worldgen::creature::APPEARANCE], a.bytes.data(), a.bytes.size());
        }
        s.spawned->setPlayerGear(r);
        s.spawned->setPlayer(*s.extracted, riderFeet, 0, glm::vec3(0), nativerules::maxHealth(r.data()), 1, false);
        if (nativeai::Creature *p = s.spawned->localPlayer())
            p->record[ABILITY] = EAGLE_RISE;
    }
}

uint32_t Session::account() const { return s.steam ? (uint32_t)(s.steam->steamId() & 0xffffffffu) : 0; }

// The start spot (frontend-characters.md §4.2, 0x2d1100): a far-away town's area landmark,
// found from a random angle and radius with the C runtime's rand(), tested as startCellFits
// says. Where none is found (a world kept as extracted zones has ground only near its home), the
// 16-block cell of the world's home stands in. The player stands on block (16 cx + 8, 16 cy + 32).
nativesaves::Cell Session::startCell(std::array<int64_t, 3> &position) const {
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    const glm::vec3 home = s.extracted ? s.extracted->home() : glm::vec3(0);
    nativesaves::Cell cell;
    const bool canSearch = s.startSpot.areaCell && s.startSpot.cellObject && s.startSpot.placeKind && s.startSpot.biome && s.startSpot.ground;
    const auto rand = [] { return std::rand(); };
    if (!canSearch || !findStartCell(s.startSpot, rand, s.startOrigin, s.farStart ? START_RADIUS_FROM : 0,
                                     s.farStart ? START_RADIUS_SPREAD : 0, START_SEARCH_STEPS, cell)) {
        const int64_t bx = (int64_t)std::floor(home.x) + (frame.originX >> 16), by = (int64_t)std::floor(home.y) + (frame.originY >> 16);
        cell = {(int32_t)std::floor(bx / 16.0), (int32_t)std::floor(by / 16.0)};
        log("Characters: no start spot passes the tests; starting at the world's home");
    } else
        log("Characters: start spot at cell " + std::to_string(cell[0]) + " " + std::to_string(cell[1]));
    const std::array<int32_t, 2> block = startBlock(cell);
    glm::vec3 feet((float)((int64_t)block[0] - (frame.originX >> 16)), (float)((int64_t)block[1] - (frame.originY >> 16)), home.z);
    float ground;
    if (s.extracted && s.extracted->surface(feet.x, feet.y, ground))
        feet.z = ground;
    position = nativeprogress::gamePosition(feet, frame);
    return cell;
}

// Applying the look (frontend-characters.md §5.4): race, class, wrapped choices, the looks record
// (the preview colour), the appearance rebuilt and the starting kit given.
void Session::applyLook(bool preview) {
    previewing = preview;
    stage.classType = (uint8_t)(look.cls + 1);
    look.wrap();
    stage.looks = look.record();
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    const glm::vec3 feet = nativeprogress::localFeet(stage.position, frame);
    const worldgen::Int2 region = worldgen::regionAt((int32_t)std::floor(feet.x + (float)(frame.originX >> 16)),
                                                     (int32_t)std::floor(feet.y + (float)(frame.originY >> 16)));
    giveStartingKit(stage, account(), {region.x, region.y});
    stage.health = nativeprogress::maxHealth(stage);
    stageHasLooks = nativeprogress::appearanceOf(stage.looks, stageLooks);
    if (s.spawned)
        s.spawned->setPlayerGear(recordOf(stage, stageHasLooks ? &stageLooks : nullptr));
    applyHairStyle();
}

void Session::applyHairStyle() {
    if (appliedHairStyle != hairEditor.preview) {
        appliedHairStyle = hairEditor.preview;
        if (s.spawned)
            s.spawned->setPlayerHairStyle(appliedHairStyle);
    }
}

// Create on the "New character" entry (frontend-characters.md §4.1).
void Session::startCreation() {
    charactersShown = false;
    selectedSlot = (int)listed.size();
    camera.set(90, 0, 0, 8);
    creationShown = true;
    std::array<int64_t, 3> position{};
    const nativesaves::Cell home = startCell(position);
    stage = newCharacter(home, position);
    name.clear();
    look.reset();
    hairEditor.reset(s.newHairStyle);
    applyLook(false);
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    riderFeet = nativeprogress::localFeet(position, frame);
    if (s.spawned && s.extracted) {
        s.spawned->setPlayer(*s.extracted, riderFeet, 0, glm::vec3(0), stage.health, 1, true);
        if (nativeai::Creature *p = s.spawned->localPlayer()) {
            p->record[ABILITY] = 0;
            std::memcpy(p->record + HEALTH, &stage.health, 4);
        }
    }
}

void Session::openOptions() {
    options.open(s.options ? *s.options : gameoptions::Options{}, displayModes());
    optionsOpen = true;
}

// Apply (frontend-options.md §1.1): the screen's copy becomes the game's, options.cfg is
// rewritten and the music volume follows at once.
void Session::applyOptions() {
    if (s.options)
        *s.options = options.values;
    if (!req.readOnly)
        gameoptions::saveOptions(s.root / "options.cfg", options.values);
    if (s.audio) {
        s.audio->setVolumes(options.values.musicVolume, options.values.soundVolume);
        s.audio->setMusicLoop(options.values.musicLoop != 0);
    }
    if (s.applyOptions)
        s.applyOptions(options.values);
}

// Back (0x9ddb0): creation to character select, character select to the title.
void Session::back(bool inGame) {
    if (helpOpen) {
        closeHelp();
        return;
    }
    if (controlsOpen) {
        controlsOpen = false;
        return;
    }
    if (optionsOpen) {
        optionsOpen = false;
        return;
    }
    if (inGame) {
        systemShown = friendsOpen = false;
        return;
    }
    if (creationShown) {
        creationShown = false, charactersShown = true;
        if (selectedSlot >= 0 && selectedSlot < (int)listed.size())
            putOnStage(listed[(size_t)selectedSlot].character, true);
        else
            stageDefaultPlayer();
        camera.set(90, 0, 180, 8);
    } else if (charactersShown)
        charactersShown = false, titleShown = true;
}

// The frame's time and pointer. The title polls the window itself; in play the game has polled
// it and gives the frame's time.
void Session::readInput(bool poll, double frameMs) {
    const glm::vec2 cachedWheelPointer = mouse * guiScale;
    if (poll)
        glfwPollEvents();
    const auto now = std::chrono::steady_clock::now();
    dtMs = poll ? std::min(1000.0, std::chrono::duration<double, std::milli>(now - last).count()) : frameMs;
    if (poll)
        hud.advanceGui(dtMs);
    last = now;
    seconds += dtMs / 1000.0;
    const VkExtent2D current = s.renderer->screenSize();
    if (current.width != size.width || current.height != size.height) {
        size = current;
        hud.layout(*s.renderer, size);
    }
    guiScale = hud.interfaceScale();
    window = glm::vec2((float)size.width, (float)size.height) / guiScale;
    double mx, my;
    int ww, wh;
    glfwGetCursorPos(s.window, &mx, &my);
    s.input->pointerX = mx, s.input->pointerY = my;
    s.input->pointerSampled = true;
    if (req.pointer.x >= 0)
        mx = req.pointer.x, my = req.pointer.y;
    glfwGetWindowSize(s.window, &ww, &wh);
    mouse = glm::vec2((float)(mx * size.width / std::max(1, ww)), (float)(my * size.height / std::max(1, wh))) / guiScale;
    pointerDown = std::find(s.input->buttons.begin(), s.input->buttons.end(), GLFW_MOUSE_BUTTON_LEFT) != s.input->buttons.end();
    pointerUp = std::find(s.input->released.begin(), s.input->released.end(), GLFW_MOUSE_BUTTON_LEFT) != s.input->released.end();
    if (std::any_of(s.input->buttons.begin(), s.input->buttons.end(), [](int button) { return button >= 0 && button < 4; }))
        characterScroll.press();
    if (pointerDown)
        leftHeld = true;
    if (pointerUp)
        leftHeld = false;
    layoutCharacterFrames();
    capturingAtPoll = capturingControl();
    const bool ui = front() || anyOpen() || s.input->gamepadModal;
    const bool focused = req.pointer.x >= 0 || glfwGetWindowAttrib(s.window, GLFW_FOCUSED);
    if (ui && focused && !capturingAtPoll) {
        if (!pointerClockStarted) {
            characterScroll.sampledAt = wallClockMs();
            pointerClockStarted = true;
        }
        // Stationary ordinary-UI polls still advance the filter, before the controller route.
        characterScroll.poll((int32_t)(mouse.y * guiScale), wallClockMs(), charactersShown, leftHeld, (int)size.width, cards);
    }
    s.input->sampleGamepad(s.window, capturingAtPoll, s.controls);
    std::vector<gamepadui::Point> points;
    if (!front())
        points = s.input->gamepadNavigationPoints;
    points.insert(points.end(), navigationPoints.begin(), navigationPoints.end());
    s.input->gamepadPointer = s.input->gamepadInput.update(s.input->gamepad, ui, s.input->gamepadMapMode,
                                                        {(float)mx, (float)my}, points, s.input->gamepadPanels);
    const auto &pad = s.input->gamepadPointer;
    if (pad.padBeforeCallbacks)
        s.input->lastUsedPad = true;
    for (int button = 0; button < 3; ++button) {
        if (pad.pressed[(size_t)button])
            s.input->buttons.push_back(button);
        if (pad.released[(size_t)button])
            s.input->released.push_back(button);
        if (pad.pressed[(size_t)button])
            characterScroll.press();
    }
    if (pad.pressed[0])
        leftHeld = true, pointerDown = true;
    if (pad.released[0])
        leftHeld = false, pointerUp = true;
    if (pad.moveCursor) {
        glfwSetCursorPos(s.window, pad.cursor.x, pad.cursor.y);
    }
    if (pad.navigationSound)
        playSound(SOUND_MENU_SELECT);
    clicked = pointerDown || pointerUp;
    if (front())
        for (const auto &event : s.input->wheelEvents)
            wheel(-event.rowStep, cachedWheelPointer);
}

// --- Keys --------------------------------------------------------------------------------------

// The interface keys (frontend-options.md §3): F1 the help sheet, F2 and F3 the interface's
// scale and F5 and F6 the map's (both saved at once), F4 the whole interface. They are taken
// out of `keys`.
void Session::handleInterfaceKeys(std::vector<int> &keys) {
    for (auto it = keys.begin(); it != keys.end();) {
        const int key = *it;
        switch (key) {
        case GLFW_KEY_F1:
            if (helpOpen)
                closeHelp();
            else
                helpOpen = true;
            break;
        case GLFW_KEY_F2:
        case GLFW_KEY_F3: { // the interface's scale, saved at once
            float g = hud.interfaceScale() * (key == GLFW_KEY_F3 ? 1.25f : 1.0f / 1.25f);
            g = std::clamp(g, gameoptions::GUI_SCALE_MIN, gameoptions::GUI_SCALE_MAX);
            hud.setGuiScale(g);
            hud.layout(*s.renderer, size);
            if (s.options) {
                s.options->guiScale = g;
                if (!req.readOnly)
                    gameoptions::saveOptions(s.root / "options.cfg", *s.options);
            }
            break;
        }
        case GLFW_KEY_F4:
            break; // Applied before focused/binding input below.
        case GLFW_KEY_F5:
        case GLFW_KEY_F6:
            if (s.options) {
                float m = s.options->mapScale * (key == GLFW_KEY_F6 ? 1.1f : 1.0f / 1.1f);
                s.options->mapScale = std::clamp(m, gameoptions::MAP_SCALE_MIN, gameoptions::MAP_SCALE_MAX);
                if (!req.readOnly)
                    gameoptions::saveOptions(s.root / "options.cfg", *s.options);
            }
            break;
        default:
            ++it;
            continue;
        }
        it = keys.erase(it);
    }
}

void Session::handleKeys(bool inGame) {
    MenuInput &in = *s.input;
    // F4 changes the global GUI ancestor even while a controls row waits for input.
    for (int key : in.pressed)
        if (key == GLFW_KEY_F4)
            interfaceShown = !interfaceShown;
    // A controls row waiting for a key takes the next key or mouse button (open question 6).
    if (controlsOpen && controls.waiting >= 0) {
        gameoptions::Binding first{0, 0};
        for (int key : in.pressed) {
            if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                controls.waiting = -1;
                in.pressed.clear();
                return;
            }
            const auto binding = keyBinding(key);
            if (binding.code && (!first.code || binding.code < first.code))
                first = binding;
        }
        if (first.code) {
            controls.bind(first);
            controls.waiting = -1;
            in.pressed.clear();
            return;
        }
        for (int button = 0; button < 4; ++button)
            if (std::find(in.buttons.begin(), in.buttons.end(), button) != in.buttons.end())
                controls.bind({button, 1});
        return; // Mouse capture finishes on release; XInput is not sampled while waiting.
    }
    handleInterfaceKeys(in.pressed);
    for (int key : in.pressed) {
        switch (key) {
        case GLFW_KEY_ESCAPE: // (Esc's own flag, G+0x420, is open question 4; here it goes back)
            back(inGame);
            break;
        case GLFW_KEY_BACKSPACE:
            if (creationShown && !name.empty() && !optionsOpen && !helpOpen) {
                while (!name.empty() && ((unsigned char)name.back() & 0xc0) == 0x80)
                    name.pop_back();
                if (!name.empty())
                    name.pop_back();
            }
            break;
        default:
            if (inGame && s.controls && keyMatches((*s.controls)[gameoptions::SYSTEM_MENU], key) && !optionsOpen && !controlsOpen)
                systemShown = false; // (the binding toggles the menu)
            if (inGame && s.controls && keyMatches((*s.controls)[gameoptions::MULTIPLAYER_MENU], key) && !optionsOpen &&
                !controlsOpen)
                friendsOpen = false; // (so does the multiplayer menu's)
            break;
        }
    }
    if (inGame && s.controls)
        for (int button : in.buttons)
            if (GameInput::buttonMatches((*s.controls)[gameoptions::SYSTEM_MENU], button) && !optionsOpen && !controlsOpen &&
                controls.waiting < 0)
                systemShown = false;
    if (creationShown && !optionsOpen && !helpOpen)
        for (char c : in.typed)
            if (nameAccepted(name + c) || name.size() < 2) // (the field takes any characters)
                name += c;
}

// --- Clicks ------------------------------------------------------------------------------------

void Session::handleClick(bool inGame) {
    const bool finishingCapture = controlsOpen && capturingAtPoll && !s.input->released.empty();
    if (finishingCapture)
        controls.waiting = -1;
    if (!clicked)
        return;
    if (helpOpen) {
        if (pointerDown)
            closeHelp();
        return;
    }
    if (controlsOpen) {
        const glm::vec2 at = centred(CONTROLS_SIZE), m = mouse - at;
        const int rows = ControlsScreen::visibleRows();
        for (int v = 0; v < rows && controls.first + v < gameoptions::BINDINGS; v++) {
            if (controls.first + v < 0)
                continue;
            const float y = ControlsScreen::rowY(v);
            if (pointerUp && !finishingCapture && m.y >= y - 20 && m.y < y + 10 && m.x >= 0 && m.x < CONTROLS_SIZE.x - 40) {
                controls.waiting = controls.first + v;
                return;
            }
        }
        const glm::vec2 up = at + glm::vec2(CONTROLS_SIZE.x - 35, 10), down = at + glm::vec2(CONTROLS_SIZE.x - 35, CONTROLS_SIZE.y - 105);
        if (pointerDown && inside(mouse, up, {25, 25}))
            scrollControls(1);
        if (pointerDown && inside(mouse, down, {25, 25}))
            scrollControls(-1);
        const float by = CONTROLS_SIZE.y - 35;
        const char *const labels[3] = {"Defaults", "OK", "Cancel"};
        for (int b = 0; b < 3; b++)
            if (pointerUp && inside(m, {20.0f + 160.0f * (float)b, by}, CONTROLS_BUTTON_SIZE)) {
                if (b == 0)
                    controls.values = gameoptions::defaultControls();
                if (b == 1) {
                    if (s.controls)
                        *s.controls = controls.values;
                    if (!req.readOnly)
                        gameoptions::saveControls(s.root / "controls.cfg", controls.values);
                }
                if (b >= 1)
                    controlsOpen = false;
                (void)labels;
            }
        return;
    }
    if (optionsOpen) {
        const glm::vec2 m = mouse - centred(OPTIONS_SIZE);
        const float valueX = (OPTIONS_SIZE.x - 250) * 0.5f + 240;
        const glm::vec2 arrow = templateSize("leftbutton", {20, 20});
        for (int row = 0; row < OPTION_ROWS; row++) {
            const float y = OptionsScreen::rowY(row);
            if (pointerDown && inside(m, {valueX - 100, y - arrow.y * 0.5f - 5}, arrow))
                options.arrow(row, -1);
            if (pointerDown && inside(m, {valueX + 100 - arrow.x, y - arrow.y * 0.5f - 5}, arrow))
                options.arrow(row, +1);
        }
        const float by = OPTIONS_SIZE.y - 40;
        for (int b = 0; b < 3; b++)
            if (pointerUp && inside(m, {40.0f + 130.0f * (float)b, by}, OPTIONS_BUTTON_SIZE)) {
                if (b < 2)
                    applyOptions();
                if (b > 0)
                    optionsOpen = false;
            }
        return;
    }
    if (inGame && friendsOpen && pointerDown && friendsClick())
        return;
    if (inGame && systemShown) { // the system menu (frontend-options.md §5)
        if (!pointerUp)
            return;
        const glm::vec2 m = mouse - centred(SYSTEM_MENU_SIZE);
        const int hovered = hoveredLine(systemLines(), m, SYSTEM_MENU_SIZE.x);
        if (hovered < 0)
            return;
        systemShown = false; // the menu hides itself, then acts
        if (hovered == SYSTEM_OPTIONS)
            openOptions();
        else if (hovered == SYSTEM_CONTROLS) {
            controls.values = s.controls ? *s.controls : gameoptions::defaultControls();
            controlsOpen = true;
        } else if (hovered == SYSTEM_START_MENU)
            decide(Result::Title);
        else if (hovered == SYSTEM_EXIT)
            decide(Result::Quit);
        return;
    }
    if (inGame)
        return;
    if (pointerDown && (charactersShown || creationShown) && inside(mouse, BACK_AT, BACK_SIZE)) {
        back(false);
        return;
    }
    if (titleShown) {
        if (!pointerUp)
            return;
        const glm::vec2 m = mouse - rootMenuPosition(window);
        const bool continueShown = selectedSlot >= 0 && selectedSlot < (int)listed.size();
        switch (hoveredLine(rootLines(continueShown), m, ROOT_MENU_SIZE.x)) {
        case START_GAME: titleShown = false, charactersShown = true; break;
        case OPTIONS: openOptions(); break;
        case EXIT: decide(Result::Quit); break;
        case CONTINUE:
            if (listed[(size_t)selectedSlot].playable)
                decide(Result::Play, selectedSlot);
            break;
        default: break;
        }
        return;
    }
    if (charactersShown) {
        if (!pointerUp)
            return;
        if (cursor < 0 || cursor > (int)listed.size())
            return;
        const bool isNew = cursor == (int)listed.size();
        const auto buttons = buttonsAt(cursor);
        auto controlHit = [&](const Hud::ScreenItem &item) { return hud.templateHit(item, mouse, /*passiveDescendants=*/true); };
        // These sibling controls are picked in reverse draw order, after the cards.
        if (!isNew && deleteShown && controlHit(characterControl("button", buttons.deleteCharacter, DELETE_SIZE, "Delete Character"))) {
            // Delete Character (frontend-characters.md §6): no confirmation; "slot" stays.
            if (!req.readOnly)
                s.store->remove(cursor);
            log("Characters: deleted character " + std::to_string(cursor));
            loadList();
            deleteShown = false;
            cursor = std::min(cursor, (int)listed.size());
            return;
        }
        if (!isNew && controlHit(characterControl("smallbutton", buttons.deleteToggle, DELETE_TOGGLE_SIZE))) {
            deleteShown = true;
            return;
        }
        if (controlHit(characterControl("button2", buttons.select, SELECT_SIZE, isNew ? "Create" : "Select"))) {
            if (isNew)
                startCreation();
            else if (listed[(size_t)cursor].playable) { // Select (frontend-characters.md §3)
                charactersShown = false;
                selectedSlot = cursor;
                if (!req.readOnly)
                    s.store->setSlot(cursor);
                decide(Result::Play, cursor);
            }
        }
        return;
    }
    if (creationShown) {
        if (pointerDown && hairEditor.press(mouse - secondHairPanelAt(window), templateSize("leftbutton", {20, 20}))) {
            applyHairStyle();
            playSound(SOUND_MENU_SELECT);
            return;
        }
        const glm::vec2 m = mouse - lookPanelAt();
        const glm::vec2 arrow = templateSize("leftbutton", {20, 20});
        for (int row = 0; row < LOOK_ROWS; row++) {
            const float y = LOOK_ROW_Y[row];
            const int direction = inside(m, {LOOK_LEFT_ARROW_X, y - arrow.y * 0.5f - 5}, arrow) ? -1
                                  : inside(m, {LOOK_RIGHT_ARROW_X, y - arrow.y * 0.5f - 5}, arrow) ? 1 : 0;
            if (pointerDown && direction) {
                playSound(SOUND_MENU_SELECT);
                look.arrow(row, direction);
                applyLook(false);
                return;
            }
        }
        int column, gridRow;
        if (pointerDown && hairCellAt(m, column, gridRow)) { // committing the colour (0x272470)
            look.committed = look.preview;
            applyLook(false);
            return;
        }
        const glm::vec2 createAt{window.x - CREATE_SIZE.x, window.y - CREATE_FROM_BOTTOM - CREATE_SIZE.y};
        if (pointerUp && nameAccepted(name) && inside(mouse, createAt, CREATE_SIZE)) {
            // Create Character (frontend-characters.md §5.8).
            creationShown = false;
            stage.name = name;
            nativesaves::prepareNewCharacter(stage);
            stage.health = nativeprogress::maxHealth(stage);
            int index = (int)listed.size();
            if (!req.readOnly && s.store)
                index = s.store->create(stage);
            if (!req.readOnly && s.store && index < 0) {
                creationShown = true;
                log("Characters: could not save the new character");
                return;
            }
            if (!req.readOnly && s.store) {
                SaveGame metadata;
                metadata.hairStyle = hairEditor.committed;
                if (!s.store->writeNotes(index, metadata.text()))
                    log("Characters: could not save the dual-tone appearance settings");
            }
            log("Characters: created \"" + name + "\" as character " + std::to_string(index));
            outcome.slot = index;
            outcome.created = true;
            outcome.createdHairStyle = hairEditor.committed;
            helpOpen = playAfterHelp = true; // the help sheet; the world follows when it is closed
        }
    }
}

// --- The world under the screens ---------------------------------------------------------------

void Session::stepWorld(Camera &cam, Lighting &light, glm::vec3 &focus, std::vector<DrawItem> &opaque,
                        std::vector<DrawItem> &water) {
    hour = std::fmod(hour + (float)(dtMs / 60000.0) * s.minutesPerSecond / 60.0f * 60.0f, 24.0f);
    s.world->advanceClock(dtMs); // (the plants sway by the world's clock, vegetation-wind.md §2)
    glm::vec3 lookAt;
    nativeai::Creature *p = s.spawned && s.extracted ? s.spawned->localPlayer() : nullptr;
    if (p) {
        uint8_t *r = p->record;
        // The player on the eagle (frontend-title.md §3.1).
        int64_t position[3];
        std::memcpy(position, r + worldgen::creature::POSITION, sizeof position);
        if (riding()) {
            r[ABILITY] = EAGLE_RISE;
            const int32_t block[2] = {(int32_t)(position[0] / 65536), (int32_t)(position[1] / 65536)};
            std::memcpy(r + FLIGHT_POINT, block, sizeof block);
            std::memcpy(r + FLIGHT_START, position, sizeof position);
        } else if (r[ABILITY] == EAGLE_RISE)
            r[ABILITY] = 0;
        uint32_t toggles;
        std::memcpy(&toggles, r + TOGGLES, 4);
        toggles = front() ? toggles | FRONT_BIT : toggles & ~FRONT_BIT;
        if (front()) {
            const bool night = hour < NIGHT_UNTIL || hour >= NIGHT_FROM;
            toggles = night ? toggles | LANTERN_BIT : toggles & ~LANTERN_BIT;
        }
        std::memcpy(r + TOGGLES, &toggles, 4);
        float health;
        std::memcpy(&health, r + HEALTH, 4);
        if (health <= 0)
            r[ABILITY] = 0;
        s.spawned->update(*s.extracted, riderFeet);
        s.spawned->think(*s.extracted, dtMs, hour);
        s.spawned->simulate(*s.extracted, dtMs);
        std::memcpy(position, r + worldgen::creature::POSITION, sizeof position);
        lookAt = s.extracted->toLocal(position) + glm::vec3(0, 0, 1); // the record's place, one block up
        riderFeet = lookAt - glm::vec3(0, 0, 1 + SpawnedCreatures::NATIVE_FEET);
    } else {
        // Without the game's creatures: a stand-in rider drifting as the eagle carries it, at
        // least 150 blocks above the ground.
        if (riding()) {
            riderFeet += glm::vec3(10.0f, 5.0f, 0.0f) * (float)(dtMs / 1000.0);
            float ground = 0;
            if (s.extracted && s.extracted->surface(riderFeet.x, riderFeet.y, ground))
                riderFeet.z += (std::max(riderFeet.z, ground + 150.0f) - riderFeet.z) * std::min(1.0f, (float)(dtMs / 1000.0));
        }
        lookAt = riderFeet + glm::vec3(0, 0, 1 + SpawnedCreatures::NATIVE_FEET);
    }
    // The camera (§3.3).
    if (riding())
        camera.titleTargets(dtMs);
    camera.smooth(dtMs, s.options ? s.options->cameraSmoothness : 80);
    glm::vec3 eye = glm::vec3(camera.eye(glm::dvec3(lookAt)));
    if (s.extracted) { // kept out of the terrain
        const glm::vec3 toEye = eye - lookAt;
        const float length = glm::length(toEye);
        for (float t = 0.3f; t < length; t += 0.2f)
            if (s.extracted->solidAt(lookAt.x + toEye.x * t / length, lookAt.y + toEye.y * t / length, lookAt.z + toEye.z * t / length)) {
                eye = lookAt + toEye * (std::max(0.3f, t - 0.3f) / length);
                break;
            }
    }
    cam = Camera{eye, lookAt};
    focus = lookAt;
    light = lightingAt(hour, s.dayNight);
    s.world->update(focus);
    opaque.clear(), water.clear();
    s.world->collect(opaque, water);
    if (p)
        s.spawned->draw(*s.extracted, *s.world, dtMs, wallClockMs(), riderFeet, s.renderer->settings.fogDistance, opaque);
    // The logo while the title layer is shown (§3.4).
    if (titleShown && logoMesh) {
        DrawItem d;
        d.mesh = logoMesh;
        d.model = logoClip(logoPlacement((float)size.width, (float)size.height, guiScale, wallClockMs()), logoSize,
                           (float)size.width, (float)size.height);
        d.flags = NATIVE_MENU;
        d.castShadow = false;
        opaque.push_back(d);
    }
}

// --- Drawing the screens -----------------------------------------------------------------------

void Session::titleItems(Hud::Screen &out) {
    const glm::vec2 at = rootMenuPosition(window);
    const bool continueShown = selectedSlot >= 0 && selectedSlot < (int)listed.size();
    const auto lines = rootLines(continueShown);
    const int hovered = optionsOpen ? -1 : hoveredLine(lines, mouse - at, ROOT_MENU_SIZE.x);
    static const char *const WORDS[4] = {"Start Game", "Options", "Exit", "Continue"};
    for (const MenuLine &l : lines) {
        navigationPoint(at + glm::vec2((float)(int)(ROOT_MENU_SIZE.x * 0.5f), l.navigationY));
        out.push_back(text(WORDS[l.index], at + glm::vec2(ROOT_MENU_SIZE.x * 0.5f, l.textY), 18, 4,
                           l.index == hovered ? HOVERED : WHITE, ALIGN_CENTRE));
    }
    const glm::vec2 px((float)size.width, (float)size.height);
    out.push_back(text("Copyright (c) 2019 Picroma e.K.", glm::vec2(10, px.y - 15) / guiScale, 12, 3, WHITE, ALIGN_LEFT));
    out.push_back(text("1.0.0-1", glm::vec2(px.x - 10, px.y - 15) / guiScale, 12, 3, WHITE, ALIGN_RIGHT));
}

void Session::layoutCharacterFrames() {
    const int entries = (int)listed.size() + 1;
    if (cards.size() != (size_t)entries) {
        cards.resize((size_t)entries);
        for (int i = 0; i < entries; i++)
            cards[(size_t)i] = {entryPosition(i) * guiScale, cardFrame * guiScale};
        cardScale = guiScale;
    } else if (cardScale != guiScale) {
        for (auto &card : cards) {
            card.at = (card.at / cardScale) * guiScale;
            card.size = cardFrame * guiScale;
        }
        cardScale = guiScale;
    }
}

void Session::characterItems(Hud::Screen &out) {
    const int entries = (int)listed.size() + 1;
    // The entry under the mouse becomes the selected one.
    for (int i = 0; !leftHeld && i < entries; i++)
        if (hud.templateHit(characterCard(entryAt(i)), mouse) && cursor != i) {
            cursor = i;
            deleteShown = false;
        }
    for (int i = 0; i < entries; i++) {
        const glm::vec2 at = entryAt(i);
        // The rounded frame is drawn before the entry's creature in previewPanels().
        if (i == (int)listed.size()) {
            out.push_back(text("New character", at + glm::vec2(ENTRY_SIZE.x * 0.5f, 30), 15, 3, WHITE, ALIGN_BOTH));
            continue;
        }
        const Listed &l = listed[(size_t)i];
        const nativesaves::Character &c = l.character;
        out.push_back(text(c.name, at + glm::vec2(200, 30), 12, 3, NAME_COLOUR, ALIGN_LEFT));
        out.push_back(text(entryLine(std::max(1, c.level), nativesaves::raceOf(c.looks.people(), c.looks.sex()), c.classType),
                           at + glm::vec2(200, 55), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(std::string("Specialization: ") + specializationWord(c.classType, c.specialization),
                           at + glm::vec2(200, 75), 12, 3, SPECIALIZATION_COLOUR, ALIGN_LEFT));
        if (!l.playable)
            out.push_back(text("(no recording of this class)", at + glm::vec2(200, 100), 12, 3, WAITING_COLOUR, ALIGN_LEFT));
    }
    if (!leftHeld && cursor >= 0 && cursor < entries) {
        const bool isNew = cursor == (int)listed.size();
        const auto buttons = buttonsAt(cursor);
        out.push_back(characterControl("button2", buttons.select, SELECT_SIZE, isNew ? "Create" : "Select"));
        if (!isNew) {
            out.push_back(characterControl("smallbutton", buttons.deleteToggle, DELETE_TOGGLE_SIZE));
            if (deleteShown)
                out.push_back(characterControl("button", buttons.deleteCharacter, DELETE_SIZE, "Delete Character"));
        }
    }
    out.push_back(widget("button", BACK_AT, BACK_SIZE, "Back"));
}

void Session::secondHairItems(Hud::Screen &out) {
    const glm::vec2 panel = secondHairPanelAt(window), point = mouse - panel;
    std::optional<glm::u8vec3> hovered;
    int column, row;
    if (!optionsOpen && secondHairCellAt(point, column, row))
        hovered = glm::u8vec3(hairCellColour(column, row) * 255.0f);
    hairEditor.hover(hovered);
    applyHairStyle();
    out.push_back(widget("blackwidget", panel, LOOK_PANEL_SIZE));
    out.push_back(text("Dual-tone hair", panel + glm::vec2(15, 25), 12, 3, {1, .8f, .5f, 1}, ALIGN_LEFT));
    const auto &style = hairEditor.committed;
    const std::array<std::string, 3> values{style.dual ? "On" : "Off", std::to_string((int)std::lround(style.start * 100)) + "%",
                                            std::to_string((int)std::lround(style.blend * 100)) + "%"};
    const char *labels[] = {"Enabled", "Start", "Blend"};
    const auto arrow = templateSize("leftbutton", {20, 20});
    for (int i = 0; i < 3; ++i) {
        const float y = 55.0f + 30.0f * i;
        out.push_back(text(labels[i], panel + glm::vec2(15, y), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(values[(size_t)i], panel + glm::vec2(LOOK_VALUE_X, y), 12, 3, WHITE, ALIGN_CENTRE));
        const glm::vec2 left{SECOND_HAIR_LEFT_ARROW_X, y - arrow.y * .5f - 5}, right{SECOND_HAIR_RIGHT_ARROW_X, y - arrow.y * .5f - 5};
        out.push_back(widget("leftbutton", panel + left, {}, "", inside(point, left, arrow) ? ARROW_HOVERED : WHITE));
        out.push_back(widget("rightbutton", panel + right, {}, "", inside(point, right, arrow) ? ARROW_HOVERED : WHITE));
    }
    out.push_back(text("Second color", panel + glm::vec2(LOOK_PANEL_SIZE.x * .5f, 155), 12, 3, WHITE, ALIGN_CENTRE));
    for (int r = 0; r < HAIR_ROWS; ++r)
        for (int c = 0; c < HAIR_COLUMNS; ++c) {
            const auto color = hairCellColour(c, r);
            const auto at = panel + SECOND_HAIR_GRID + glm::vec2(c, r) * HAIR_CELL;
            const bool selected = glm::u8vec3(color * 255.0f) == style.second;
            out.push_back(square(at, glm::vec2(HAIR_CELL), selected ? WHITE : BLACK));
            out.push_back(square(at + glm::vec2(1), glm::vec2(HAIR_CELL - 2), glm::vec4(color, 1)));
            navigationPoint(at + glm::vec2(HAIR_CELL * .5f));
        }
    out.push_back(text("0% blend = sharp split", panel + glm::vec2(15, 315), 12, 3, WHITE, ALIGN_LEFT));
}

void Session::creationItems(Hud::Screen &out) {
    secondHairItems(out);
    const glm::vec2 panel = lookPanelAt(), m = mouse - panel;
    // Hovering the grid previews its colour on the player; leaving it restores the committed one.
    look.preview = look.committed;
    int column, gridRow;
    if (!optionsOpen && hairCellAt(m, column, gridRow)) {
        const glm::vec3 c = hairCellColour(column, gridRow) * 255.0f;
        look.preview = {(uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b};
    }
    if (look.preview != stage.looks.hairColour())
        applyLook(true);
    out.push_back(widget("blackwidget", panel, LOOK_PANEL_SIZE)); // (the panel's template is not named by the spec)
    static const char *const LABELS[LOOK_ROWS] = {"Race", "Gender", "Class", "Face", "Haircut"};
    const glm::vec2 arrow = templateSize("leftbutton", {20, 20});
    for (int row = 0; row < LOOK_ROWS; row++) {
        const float y = LOOK_ROW_Y[row];
        out.push_back(text(LABELS[row], panel + glm::vec2(15, y), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(look.value(row), panel + glm::vec2(LOOK_VALUE_X, y), 12, 3, WHITE, ALIGN_CENTRE));
        const glm::vec2 left{LOOK_LEFT_ARROW_X, y - arrow.y * 0.5f - 5}, right{LOOK_RIGHT_ARROW_X, y - arrow.y * 0.5f - 5};
        out.push_back(widget("leftbutton", panel + left, glm::vec2(0), "", inside(m, left, arrow) ? ARROW_HOVERED : WHITE));
        out.push_back(widget("rightbutton", panel + right, glm::vec2(0), "", inside(m, right, arrow) ? ARROW_HOVERED : WHITE));
    }
    out.push_back(text("Hair color", panel + glm::vec2(LOOK_PANEL_SIZE.x * 0.5f, HAIR_TITLE_Y), 12, 3, WHITE, ALIGN_CENTRE));
    for (int r = 0; r < HAIR_ROWS; r++)
        for (int c = 0; c < HAIR_COLUMNS; c++) {
            const glm::vec2 at = panel + HAIR_GRID + glm::vec2((float)c, (float)r) * HAIR_CELL;
            out.push_back(square(at, glm::vec2(HAIR_CELL), BLACK));
            out.push_back(square(at + glm::vec2(1), glm::vec2(HAIR_CELL - 2), glm::vec4(hairCellColour(c, r), 1)));
        }
    // The name field and Create Character (where negative y counts from the bottom: open
    // question 4).
    const glm::vec2 edit = templateSize("edit", {300, 30});
    const glm::vec2 editAt{window.x * 0.5f - edit.x * 0.5f, window.y - NAME_FIELD_FROM_BOTTOM - edit.y};
    out.push_back(widget("edit", editAt, glm::vec2(0), name.empty() ? " " : name));
    out.push_back(text("Name", editAt + glm::vec2(0, -8), 12, 3, WHITE, ALIGN_LEFT));
    if (nameAccepted(name))
        out.push_back(widget("button", {window.x - CREATE_SIZE.x, window.y - CREATE_FROM_BOTTOM - CREATE_SIZE.y}, CREATE_SIZE,
                             "Create Character"));
    out.push_back(widget("button", BACK_AT, BACK_SIZE, "Back"));
}

void Session::optionsItems(Hud::Screen &out) {
    const glm::vec2 at = centred(OPTIONS_SIZE), m = mouse - at;
    out.push_back(widget("blackwidget", at, OPTIONS_SIZE));
    out.back().navigation = Hud::ScreenItem::ChildCentres;
    const float valueX = (OPTIONS_SIZE.x - 250) * 0.5f + 240;
    const glm::vec2 arrow = templateSize("leftbutton", {20, 20});
    for (int row = 0; row < OPTION_ROWS; row++) {
        const float y = OptionsScreen::rowY(row);
        out.push_back(text(OptionsScreen::label(row), at + glm::vec2(15, y), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(options.value(row), at + glm::vec2(valueX, y), 12, 3, WHITE, ALIGN_CENTRE));
        const glm::vec2 left{valueX - 100, y - arrow.y * 0.5f - 5}, right{valueX + 100 - arrow.x, y - arrow.y * 0.5f - 5};
        out.push_back(widget("leftbutton", at + left, glm::vec2(0), "", inside(m, left, arrow) ? ARROW_HOVERED : WHITE));
        out.back().navigation = Hud::ScreenItem::ShapeCentre;
        out.push_back(widget("rightbutton", at + right, glm::vec2(0), "", inside(m, right, arrow) ? ARROW_HOVERED : WHITE));
        out.back().navigation = Hud::ScreenItem::ShapeCentre;
    }
    const char *const labels[3] = {"Apply", "OK", "Cancel"};
    for (int b = 0; b < 3; b++) {
        out.push_back(widget("button", at + glm::vec2(40.0f + 130.0f * (float)b, OPTIONS_SIZE.y - 40), OPTIONS_BUTTON_SIZE, labels[b]));
        out.back().navigation = Hud::ScreenItem::ShapeCentre;
    }
}

void Session::refreshControlsThumb() {
    controlsThumbFrame = hud.templateSize("blackwidget", CONTROLS_SIZE);
    const float width = hud.templateSize("scrollbutton").x;
    if (width > 0)
        controlsThumb = panelscroll::controlsThumb(controls.first, controlsThumbFrame.x, controlsThumbFrame.y, width);
}
void Session::refreshFriendsThumb() {
    friendsThumbFrame = hud.templateSize("blackwidget", FRIENDS_SIZE);
    const float width = hud.templateSize("scrollbutton").x;
    if (width > 0)
        friendsThumb = panelscroll::friendsThumb(firstFriend, friendsThumbFrame.x, friendsThumbFrame.y, width, friendRows.size());
}
void Session::scrollControls(int nativeDelta) {
    const auto frame = hud.templateSize("blackwidget", CONTROLS_SIZE);
    const auto changed = panelscroll::controls(controls.first, nativeDelta, frame.y);
    controls.first = changed.first;
    if (changed.refreshThumb)
        refreshControlsThumb();
}
void Session::scrollFriends(int nativeDelta) {
    const auto frame = hud.templateSize("blackwidget", FRIENDS_SIZE);
    const auto changed = panelscroll::friends(firstFriend, nativeDelta, frame.y, friendRows.size());
    firstFriend = changed.first;
    if (changed.refreshThumb)
        refreshFriendsThumb();
}
bool Session::wheel(int nativeDelta, glm::vec2 cachedPointer) {
    const glm::vec2 point = cachedPointer / guiScale;
    if (controlsOpen && hud.templateHit(widget("blackwidget", centred(CONTROLS_SIZE), CONTROLS_SIZE), point)) {
        scrollControls(nativeDelta);
        return true;
    }
    if (friendsOpen && hud.templateHit(widget("blackwidget", centred(FRIENDS_SIZE), FRIENDS_SIZE), point)) {
        scrollFriends(nativeDelta);
        return true;
    }
    return false;
}

void Session::controlsItems(Hud::Screen &out) {
    if (s.input->lastUsedPad)
        controls.waiting = -1;
    const glm::vec2 at = centred(CONTROLS_SIZE), m = mouse - at;
    if (controlsThumbFrame != hud.templateSize("blackwidget", CONTROLS_SIZE))
        refreshControlsThumb();
    out.push_back(widget("blackwidget", at, CONTROLS_SIZE));
    if (controlsThumb) {
        const auto &thumb = *controlsThumb;
        out.push_back(widget("scrollbutton", at + glm::vec2(thumb.x, thumb.y), {thumb.width, thumb.height}));
        out.back().replaceWidget = true;
    }
    const float valueX = (CONTROLS_SIZE.x - 250) * 0.5f + 240;
    const int rows = ControlsScreen::visibleRows();
    for (int v = 0; v < rows && controls.first + v < gameoptions::BINDINGS; v++) {
        const int i = controls.first + v;
        if (i < 0)
            continue;
        const float y = ControlsScreen::rowY(v);
        const gameoptions::Binding &b = controls.values[(size_t)i];
        glm::vec4 colour = WHITE;
        if (b.code == 0 && b.kind == 0)
            colour = UNASSIGNED_COLOUR;
        if (m.y >= y - 20 && m.y < y + 10 && m.x >= 0 && m.x < CONTROLS_SIZE.x - 40)
            colour = HOVERED;
        if (i == controls.waiting)
            colour = WAITING_COLOUR;
        out.push_back(text(gameoptions::bindingLabel(i), at + glm::vec2(20, y), 12, 3, colour, ALIGN_LEFT));
        out.push_back(text(i == controls.waiting ? "Press a key" : gameoptions::bindingName(b), at + glm::vec2(valueX, y), 12, 3, colour,
                           ALIGN_CENTRE));
    }
    out.push_back(widget("upbutton", at + glm::vec2(CONTROLS_SIZE.x - 35, 10), glm::vec2(0)));
    out.push_back(widget("downbutton", at + glm::vec2(CONTROLS_SIZE.x - 35, CONTROLS_SIZE.y - 105), glm::vec2(0)));
    const char *const labels[3] = {"Defaults", "OK", "Cancel"};
    for (int b = 0; b < 3; b++) {
        out.push_back(widget("button", at + glm::vec2(20.0f + 160.0f * (float)b, CONTROLS_SIZE.y - 35), CONTROLS_BUTTON_SIZE, labels[b]));
        out.back().navigation = Hud::ScreenItem::ShapeCentre;
    }
}

// The help sheet (the HelpWidget listing the controls). Its layout is not specified; the
// bindings are listed in two columns.
void Session::helpItems(Hud::Screen &out) {
    const glm::vec2 sheet{560, 60.0f + 20.0f * (float)gameoptions::BINDINGS};
    const glm::vec2 at = centred(sheet);
    out.push_back(widget("blackwidget", at, sheet));
    out.push_back(text("Controls", at + glm::vec2(sheet.x * 0.5f, 30), 15, 3, WHITE, ALIGN_CENTRE));
    const gameoptions::Controls &c = s.controls ? *s.controls : controls.values;
    for (int i = 0; i < gameoptions::BINDINGS; i++) {
        const float y = 60.0f + 20.0f * (float)i;
        out.push_back(text(gameoptions::bindingLabel(i), at + glm::vec2(30, y), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(gameoptions::bindingName(c[(size_t)i]), at + glm::vec2(sheet.x - 30, y), 12, 3, NAME_COLOUR, ALIGN_RIGHT));
    }
}

void Session::systemItems(Hud::Screen &out) {
    const glm::vec2 at = centred(SYSTEM_MENU_SIZE);
    const int hovered = hoveredLine(systemLines(), mouse - at, SYSTEM_MENU_SIZE.x);
    static const char *const WORDS[4] = {"Options", "Controls", "Start Menu", "Exit Game"};
    for (const MenuLine &l : systemLines()) {
        navigationPoint(at + glm::vec2((float)(int)(SYSTEM_MENU_SIZE.x * 0.5f), l.navigationY));
        out.push_back(text(WORDS[l.index], at + glm::vec2(SYSTEM_MENU_SIZE.x * 0.5f, l.textY), 13, 3,
                           l.index == hovered ? HOVERED : WHITE, ALIGN_CENTRE));
    }
}

// --- The multiplayer menu (netcode-transport.md §3) ---------------------------------------------

void Session::toggleMultiplayerMenu() {
    if (friendsOpen || !s.friends) {
        friendsOpen = false;
        return;
    }
    // Opening rebuilds the list (§3.1); it is not rebuilt while the menu stays open.
    friendRows = s.friends->list ? s.friends->list() : std::vector<FriendRow>{};
    refreshFriendsThumb();
    friendsOpen = true;
}

// The rows' states (host, joined, invited, a live invitation) change while the menu is open.
void Session::refreshFriendStates() {
    if (!s.friends || !s.friends->list)
        return;
    const std::vector<FriendRow> now = s.friends->list();
    for (FriendRow &row : friendRows) {
        auto same = std::find_if(now.begin(), now.end(), [&](const FriendRow &r) { return r.id == row.id; });
        if (same != now.end())
            row = *same;
        else
            row.hostOfMine = row.joined = row.invited = row.invitation = false;
    }
}

int Session::hoveredFriend(glm::vec2 m) const {
    const float top = FRIENDS_FIRST_ROW_Y - FRIEND_ROW_HEIGHT * 0.5f - 4;
    if (m.x < 0 || m.x >= FRIENDS_SIZE.x || m.y < top)
        return -1;
    const int row = firstFriend + (int)std::floor((m.y - top) / FRIEND_ROW_HEIGHT);
    return row >= 0 && row < (int)friendRows.size() && friendRows[(size_t)row].eligible ? row : -1;
}

glm::vec2 Session::friendButtonAt(int row, bool right) const {
    const float y = FRIENDS_FIRST_ROW_Y + FRIEND_ROW_HEIGHT * float(row - firstFriend) - FRIEND_BUTTON_SIZE.y * 0.5f - 4;
    return {FRIENDS_SIZE.x - (right ? 1.0f : 2.0f) * (FRIEND_BUTTON_SIZE.x + 10), y};
}

void Session::friendsItems(Hud::Screen &out) {
    refreshFriendStates();
    const glm::vec2 at = centred(FRIENDS_SIZE), m = mouse - at;
    out.push_back(widget("blackwidget", at, FRIENDS_SIZE));
    if (friendsThumbFrame != hud.templateSize("blackwidget", FRIENDS_SIZE))
        refreshFriendsThumb();
    if (friendsThumb) {
        const auto &thumb = *friendsThumb;
        out.push_back(widget("scrollbutton", at + glm::vec2(thumb.x, thumb.y), {thumb.width, thumb.height}));
        out.back().replaceWidget = true;
    }
    out.push_back(text("Friends", at + glm::vec2(FRIENDS_SIZE.x * 0.5f, FRIENDS_TITLE_Y), 15, 3, WHITE, ALIGN_CENTRE));
    if (friendRows.empty() && s.friends)
        out.push_back(
            text(s.friends->emptyText, at + glm::vec2(FRIENDS_SIZE.x * 0.5f, FRIENDS_FIRST_ROW_Y + 10), 12, 3, WHITE, ALIGN_CENTRE));
    const int hovered = hoveredFriend(m);
    for (int i = std::max(0, firstFriend); i < (int)friendRows.size(); i++) {
        const FriendRow &row = friendRows[(size_t)i];
        const float y = FRIENDS_FIRST_ROW_Y + FRIEND_ROW_HEIGHT * float(i - firstFriend);
        if (y > FRIENDS_SIZE.y - FRIEND_ROW_HEIGHT)
            break; // (rows past the panel are not drawn)
        // Row colours by state (the game's are cosmetic constants, §3.2).
        glm::vec4 colour = row.joined || row.hostOfMine ? FRIEND_JOINED_COLOUR : row.invited ? FRIEND_INVITED_COLOUR : WHITE;
        if (row.invitation && !row.hostOfMine)
            colour = FRIEND_INVITATION_COLOUR;
        if (i == hovered)
            colour = HOVERED;
        if (!row.eligible) colour = glm::vec4(.5f, .5f, .5f, 1);
        out.push_back(text(friendRowText(row), at + glm::vec2(15, y), 12, 3, colour, ALIGN_LEFT));
        // The left button only while the row is hovered; the right one also while the friend's
        // invitation is live.
        if (i == hovered)
            out.push_back(widget("button", at + friendButtonAt(i, false), FRIEND_BUTTON_SIZE, hostButtonLabel(row)));
        if (i == hovered || (row.invitation && !row.hostOfMine))
            out.push_back(widget("button", at + friendButtonAt(i, true), FRIEND_BUTTON_SIZE, guestButtonLabel(row)));
    }
}

// A click on a row's buttons (§3.3); false when it was not on the menu.
bool Session::friendsClick() {
    const glm::vec2 m = mouse - centred(FRIENDS_SIZE);
    if (!inside(m, {0, 0}, FRIENDS_SIZE))
        return false;
    const int row = hoveredFriend(m);
    if (row < 0 || !s.friends)
        return true;
    const FriendRow &friendRow = friendRows[(size_t)row];
    if (inside(m, friendButtonAt(row, false), FRIEND_BUTTON_SIZE) && s.friends->hostButton) {
        s.friends->hostButton(friendRow.id);
        playSound(SOUND_MENU_SELECT);
    } else if (inside(m, friendButtonAt(row, true), FRIEND_BUTTON_SIZE) && s.friends->guestButton) {
        s.friends->guestButton(friendRow.id);
        playSound(SOUND_MENU_SELECT);
    }
    return true;
}

Hud::Screen Session::buildScreen(bool inGame) {
    Hud::Screen out;
    navigationPoints.clear();
    if (!interfaceShown)
        return out;
    if (inGame) {
        if (systemShown && !optionsOpen && !controlsOpen)
            systemItems(out);
        if (friendsOpen && !optionsOpen && !controlsOpen)
            friendsItems(out);
    } else if (titleShown)
        titleItems(out);
    else if (charactersShown)
        characterItems(out);
    else if (creationShown)
        creationItems(out);
    if (optionsOpen)
        optionsItems(out);
    if (controlsOpen)
        controlsItems(out);
    if (helpOpen)
        helpItems(out);
    for (auto &item : out)
        if (item.kind == Hud::ScreenItem::Template) {
            item.buttonHover = hud.templateHit(item, mouse, true);
            item.animationStepMs = (int)dtMs;
        }
    return out;
}

// One frame of the title's screens; false when they are done (`outcome` says why).
bool Session::frame() {
    readInput(true);
    if (s.steam) {
        s.steam->pump();
        if (seconds >= nextSteamCheck) {
            if (!s.steam->check(outcome.error)) {
                decide(Result::Quit);
                return false;
            }
            nextSteamCheck = seconds + 5;
        }
    }
    handleKeys(false);
    characterScroll.frame((float)dtMs, charactersShown, leftHeld, {(int)size.width, (int)size.height}, cards);
    if (req.clickAfter >= 0 && !testClicked && seconds >= req.clickAfter && s.world->pendingCount() == 0) {
        clicked = pointerDown = pointerUp = true; // (testing: a complete synthetic click)
        testClicked = true;
    }
    handleClick(false);
    s.input->finishPointerCallbacks();
    s.input->clear();
    if (done)
        return false;
    if (glfwWindowShouldClose(s.window)) {
        decide(Result::Quit);
        return false;
    }

    Camera cam;
    Lighting light;
    glm::vec3 focus;
    std::vector<DrawItem> opaque, water;
    stepWorld(cam, light, focus, opaque, water);
    const Hud::Screen screen = buildScreen(false);
    if (charactersShown) {
        previewPanels(opaque);
        previewItems(opaque);
    }
    std::vector<OverlayVertex> overlay;
    std::vector<glm::vec2> templatePoints;
    hud.drawScreen(screen, overlay, &templatePoints);
    for (const auto point : templatePoints)
        navigationPoint(point / guiScale);
    s.renderer->overlay(std::move(overlay));
    s.renderer->frame(cam, light, focus, seconds, opaque, water);
    if (s.audio) {
        s.audio->menuMusic(dtMs);
        s.audio->update();
    }
    if (!req.capture.empty() && req.clickAfter < 0) {
        settled = s.world->pendingCount() == 0 && seconds >= req.captureAfter ? settled + 1 : 0;
        if (settled == 30)
            s.renderer->capture(req.capture);
        if (settled >= 32 || seconds > 120 + req.captureAfter) {
            decide(Result::Quit);
            return false;
        }
    }
    return true;
}

Result Session::runTitle() {
    if (req.page == "characters")
        titleShown = false, charactersShown = true;
    else if (req.page == "create")
        titleShown = false, startCreation();
    else if (req.page == "options")
        openOptions();
    else if (req.page == "controls")
        controlsOpen = true;
    if (req.page == "characters" || req.page == "create") {
        // (a capture of these screens points at the first entry)
        cursor = 0;
    }
    while (frame()) {
    }
    if (outcome.action == Result::Play) {
        playSound(SOUND_MENU_CLOSE);
        enterWorld();
    }
    return outcome;
}

// Continue, Select and Create Character put the player in the world (frontend-title.md §6.1):
// the record is rebuilt from the character as saved, every field the save does not hold at a
// new creature's value (no velocity, no ability, no eagle flight), the position exactly as
// saved and the facing yaw 0. Only then do the front screens stop riding it. Nothing searches
// for ground: zones not loaded yet read as solid, so the player waits on them.
void Session::enterWorld() {
    const nativesaves::Character *saved = nullptr;
    if (outcome.created)
        saved = &stage; // (just made: what was written)
    else if (outcome.slot >= 0 && outcome.slot < (int)listed.size())
        saved = &listed[(size_t)outcome.slot].character;
    if (!saved)
        return;
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    const glm::vec3 feet = enteredFeet(*saved, frame, s.extracted ? s.extracted->home() : glm::vec3(0));
    if (s.spawned && s.extracted) {
        s.spawned->resetPlayer();
        const bool hasLooks = nativeprogress::appearanceOf(saved->looks, stageLooks);
        s.spawned->setPlayerGear(recordOf(*saved, hasLooks ? &stageLooks : nullptr));
        s.spawned->setPlayer(*s.extracted, feet, 0, glm::vec3(0), std::max(1.0f, saved->health), std::max(1, saved->level), false);
    }
    titleShown = charactersShown = creationShown = false;
    outcome.placed = true;
    outcome.feet = feet;
    // The view cuts from the flyover to the player: the anti-aliasing's history is stale.
    s.renderer->cutHistory();
}

// One frame in play: the interface keys always; while a screen is open, the frame's keys and
// clicks are its own (and taken from the input, so the game does not act on them too).
Result::Action Session::gameFrame(double frameMs) {
    readInput(false, frameMs);
    const bool wasOpen = anyOpen();
    if (!wasOpen) {
        handleInterfaceKeys(s.input->pressed);
        if (anyOpen()) // (F1 opened the help sheet: its keys start next frame)
            playSound(SOUND_MENU_OPEN);
        return Result::Stay;
    }
    handleKeys(true);
    handleClick(true);
    s.input->finishPointerCallbacks();
    s.input->pressed.clear(), s.input->buttons.clear(), s.input->released.clear(), s.input->typed.clear();
    s.input->scroll = 0;
    if (!anyOpen())
        playSound(SOUND_MENU_CLOSE);
    if (done) {
        done = false;
        return outcome.action;
    }
    return Result::Stay;
}

void Session::gameDraw(std::vector<OverlayVertex> &out) {
    if (anyOpen()) {
        std::vector<glm::vec2> templatePoints;
        hud.drawScreen(buildScreen(true), out, &templatePoints);
        for (const auto point : templatePoints)
            navigationPoint(point / guiScale);
    }
    else
        navigationPoints.clear();
}

Result run(Services &s, const Request &request) {
    Session session(s, request);
    Result result;
    if (!session.start(result.error))
        return result;
    result = session.runTitle();
    session.finish();
    return result;
}

// --- The screens in play -----------------------------------------------------------------------

InGameMenus::InGameMenus(Services &s, const Request &request) : session(std::make_unique<Session>(s, request)) {}
InGameMenus::~InGameMenus() { session->finish(); }
bool InGameMenus::start(std::string &error) { return session->startInGame(error); }
void InGameMenus::toggleSystemMenu() {
    const bool wasOpen = session->anyOpen();
    session->toggleSystemMenu();
    if (!wasOpen && session->anyOpen() && session->services().audio && !session->services().externalMenuSounds)
        session->services().audio->playInterface(SOUND_MENU_OPEN);
}
void InGameMenus::toggleMultiplayerMenu() {
    const bool wasOpen = session->anyOpen();
    session->toggleMultiplayerMenu();
    if (!wasOpen && session->anyOpen() && session->services().audio && !session->services().externalMenuSounds)
        session->services().audio->playInterface(SOUND_MENU_OPEN);
}
bool InGameMenus::open() const {
    return session->anyOpen();
}
bool InGameMenus::ordinaryUi() const {
    return session->ordinaryUi();
}
bool InGameMenus::capturingControl() const {
    return session->capturingControl();
}
bool InGameMenus::wheel(int nativeDelta, glm::vec2 cachedPointer) {
    return session->wheel(nativeDelta, cachedPointer);
}
void InGameMenus::hideSystemForMovement() {
    session->hideSystemForMovement();
}
void InGameMenus::hideMultiplayerForMovement() {
    session->hideMultiplayerForMovement();
}

void InGameMenus::dismiss() {
    session->dismissMenus();
}
Result::Action InGameMenus::frame(double dtMs) {
    return session->gameFrame(dtMs);
}
void InGameMenus::draw(std::vector<OverlayVertex> &out) {
    session->gameDraw(out);
}
bool InGameMenus::interfaceShown() const {
    return session->interfaceVisible();
}

} // namespace frontend
