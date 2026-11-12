// Running the front screens (Frontend.h): the title scene over the ordinary world, the root
// menu, character select and creation, the options, controls and help screens, and the system
// menu over the running game. Written from docs/specs/frontend-title.md,
// frontend-characters.md and frontend-options.md.
#include "Frontend.h"
#include <bit>
#include "CharacterCard.h"
#include "CameraMotion.h"

#include "Assets.h"
#include "Audio.h"
#include "Character.h"
#include "ChatLine.h"
#include "CubModel.h"
#include "Extracted.h"
#include "GameInput.h"
#include "HelpActions.h"
#include "NativeTerms.h"
#include "GameMath.h"
#include "MapLook.h"
#include "PanelScroll.h"
#include "NativePlayer.h"
#include "NativeControl.h"
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

MenuInput::MouseEdges MenuInput::windowMouseEdges() const {
    MouseEdges out;
    for (const auto message : interfaceMessages) {
        switch (message.kind) {
        case InterfaceMessage::LeftDown: out.pressed.push_back(0); break;
        case InterfaceMessage::LeftUp: out.released.push_back(0); break;
        case InterfaceMessage::RightDown: out.pressed.push_back(1); break;
        case InterfaceMessage::RightUp: out.released.push_back(1); break;
        case InterfaceMessage::MiddleDown: out.pressed.push_back(2); break;
        case InterfaceMessage::MiddleUp: out.released.push_back(2); break;
        case InterfaceMessage::ToggleVisibility:
        case InterfaceMessage::ToggleHelp:
        case InterfaceMessage::Wheel:
        case InterfaceMessage::KeyDown:
        case InterfaceMessage::KeyUp:
        case InterfaceMessage::Character: break;
        }
    }
    return out;
}

void MenuInput::keyMessage(int key, bool repeat) {
    if (!repeat || key == GLFW_KEY_F1 || key == GLFW_KEY_F4)
        pressed.push_back(key); // Window events are distinct from physical capture edges.
    if (!rawKeyboardSeen && key==GLFW_KEY_F1)
        interfaceMessages.emplace_back(InterfaceMessage::ToggleHelp);
    if (!rawKeyboardSeen && key==GLFW_KEY_F4)
        interfaceMessages.emplace_back(InterfaceMessage::ToggleVisibility);
}

void MenuInput::rawKeyMessage(InterfaceMessage kind,uint16_t unit) {
    rawKeyboardSeen=true;
    interfaceMessages.emplace_back(kind,0,unit);
}

void MenuInput::wheelMessage(int16_t delta) {
    interfaceMessages.emplace_back(InterfaceMessage::Wheel, delta < 0 ? -1 : delta > 0 ? 1 : 0);
}

bool MenuInput::samplePhysical(const gameoptions::Controls &controls, bool fixtureForeground) {
    const bool admitted = physicalEnabled && (fixtureForeground || devices.foreground());
    // The explicit CLI pointer adapter supplies neutral device bytes. It must
    // not turn a hidden capture into background keyboard/controller input.
    physical.sample(admitted, [&](auto &output) { if (fixtureForeground) output = {}; else devices.readKeyboard(output); },
                    [&](auto &output) { if (fixtureForeground) output = {}; else devices.readMouse(output); },
                    [&] { return physicalTarget(controls); });
    if (!admitted)
        physicalinput::Devices::inactiveWait();
    lastUsedPadAtMousePoll = lastUsedPad;
    return physical.phase == physicalinput::Poll::Normal;
}

void MenuInput::routeInterfaceRelease(Hud &hud, int button) {
    const auto current = gameMouseTarget;
    const auto gui = [&](int value) {
        if (value == 0) hud.interfaceLeftUp();
        else hud.interfaceSecondaryButton(value, false);
    };
    if (current) {
        gamemouse::release(gameMouse, [&] {
            auto target = current();
            target.guiRelease = gui;
            target.controlsRelease = [&](int value) { controlEditor.release(value); };
            return target;
        }, button);
        return;
    }
    const auto continuation = gameRelease; // Retain the entry Game before GUI callbacks.
    if (uint32_t(button) <= 2) gui(button);
    if (continuation) continuation(button);
    controlEditor.release(button);
}

void MenuInput::routeInterfacePress(Hud &hud,int button) {
    const auto current = gameMouseTarget;
    const auto continuation = gamePress;
    const auto gui = [&](int value) {
        if (value == 0) interfacePresses.push_back(hud.interfaceLeftDown());
        else hud.interfaceSecondaryButton(value, true);
        if (value == 1 && !hud.interfacePointerTarget()) {
            gameKeyboard.menu = 0;
            hud.clearInventoryPanelNodes();
            const auto callback = gameDismissPanels;
            if (callback) callback();
        }
    };
    if (current) {
        if (gamemouse::press(gameMouse, [&] {
            auto target = current();
            target.guiY = [&] { return std::bit_cast<uint32_t>(hud.interfacePointerPosition().y); };
            target.guiPress = gui;
            return target;
        }, button)) gamePresses.push_back(button);
    } else {
        if (uint32_t(button) <= 2) gui(button);
        if (!continuation || continuation(button)) gamePresses.push_back(button);
    }
}

void MenuInput::routeInterfaceMessages(Hud &hud,const WheelCallback &wheel,const std::function<void(uint8_t)> &special,
                                        const std::function<void(gamewindow::Target &)> &currentKeyboard) {
    interfacePresses.clear();
    gamePresses.clear();
    const auto keyboard=[&](gamewindow::Key kind,uint16_t unit) {
        gamewindow::dispatch(kind,unit,[&](gamewindow::Key event,uint16_t key) {
            hud.interfaceKeyboard(event==gamewindow::Key::Down ? plasma::Input::Key::Down :
                                  event==gamewindow::Key::Up ? plasma::Input::Key::Up : plasma::Input::Key::Character,key);
        },[&] {
            gamewindow::Target target;
            target.keyboard=&gameKeyboard;
            target.special=[&](uint8_t key) {
                if (key==0x70) hud.setHelpShown(!hud.helpShown());
                else if (key==0x73) hud.setInterfaceShown(!hud.interfaceShown());
                else if (special) special(key);
            };
            // Fetch the current ports after the GUI receiver has returned.
            target.down=gameKeyDown;
            target.character=gameCharacter;
            if (currentKeyboard) currentKeyboard(target); // Fresh ports after the GUI phase; no retained Session callback.
            return target;
        });
    };
    for (const auto message : interfaceMessages) {
        switch (message.kind) {
        case InterfaceMessage::KeyDown: keyboard(gamewindow::Key::Down,message.unit); break;
        case InterfaceMessage::KeyUp: keyboard(gamewindow::Key::Up,message.unit); break;
        case InterfaceMessage::Character: keyboard(gamewindow::Key::Character,message.unit); break;
        case InterfaceMessage::LeftDown:
            routeInterfacePress(hud,0);
            break;
        case InterfaceMessage::LeftUp:
            routeInterfaceRelease(hud,0);
            break;
        case InterfaceMessage::RightDown: routeInterfacePress(hud,1); break;
        case InterfaceMessage::RightUp: routeInterfaceRelease(hud,1); break;
        case InterfaceMessage::MiddleDown: routeInterfacePress(hud,2); break;
        case InterfaceMessage::MiddleUp: routeInterfaceRelease(hud,2); break;
        case InterfaceMessage::ToggleVisibility:
            hud.setInterfaceShown(!hud.interfaceShown());
            break;
        case InterfaceMessage::ToggleHelp:
            hud.setHelpShown(!hud.helpShown());
            break;
        case InterfaceMessage::Wheel:
            if (wheel)
                wheel(message.wheelDelta, hud.interfacePointerPosition());
            break;
        }
    }
    interfaceMessages.clear();
}

void MenuInput::routeGamepadButtons(Hud &hud) {
    for (int button = 0; button < 3; ++button) {
        if (gamepadPointer.pressed[(size_t)button]) routeInterfacePress(hud,button);
        if (gamepadPointer.released[(size_t)button]) routeInterfaceRelease(hud,button);
    }
}

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
constexpr uint32_t LANTERN_BIT = 0x1;
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
// World creation (world-list contract 2.3-2.4): the seed label sits 50 units above the name
// label, and the multiplayer toggle is 300 x 40 at the top right.
constexpr float SEED_FIELD_FROM_BOTTOM = 100;
constexpr glm::vec2 WORLD_TOGGLE_SIZE{300, 40};

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
constexpr int START_SEARCH_STEPS = 64 * 4000; // engine abort bound; native candidate loop is unbounded

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
Hud::ScreenItem text(const std::wstring &value, glm::vec2 at, float size, float outline, glm::vec4 colour, int align) {
    auto item = text(std::string{}, at, size, outline, colour, align);
    item.text = value;
    return item;
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

// A fallback list creature drawn in screen space: meshed as the game meshes
// every voxel model (worldmap-rendering.md §2: each face its own corners, the 6 x 6 corner
// occlusion, .cub colours halved), so each voxel keeps its own colour and an open face is flat.
// The colours are doubled back as the model shader does (§3.2) and shaded once face by face
// (screenModelShade). Native animated portraits and the logo use the WORLD path instead.
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
    Session(Services &services, const Request &request)
        : s(services), req(request), hud(services.hud ? *services.hud : ownHud),
          detachedControls(services.input ? nullptr : std::make_unique<ControlsScreen>()),
          controlsOpen((services.input ? services.input->controlEditor : *detachedControls).visible),
          controls(services.input ? services.input->controlEditor : *detachedControls) {}
    bool start(std::string &error);
    void finish();
    Result runTitle();
    // In play (InGameMenus): the screens over the running game.
    bool startInGame(std::string &error);
    Result::Action gameFrame(double dtMs);
    void gameDraw(std::vector<OverlayVertex> &out);
    bool anyOpen() const { return systemShown || optionsOpen || controlsOpen || hud.helpShown() || friendsOpen; }
    bool ordinaryUi() const { return systemShown || optionsOpen || controlsOpen || friendsOpen; }
    bool capturingControl() const { return controls.waiting >= 0; }
    int bindingCaptureIndex() const { return controls.waiting; }
    bool systemMenuShown() const { return systemShown; }
    bool wheel(int nativeDelta, glm::vec2 cachedPointer);
    void hideSystemForMovement() { systemShown = false; }
    void hideMultiplayerForMovement() { friendsOpen = false; }
    void dismissMenus() {
        while (ordinaryUi())
            compatibilityBack(true);
    }
    void toggleSystemMenu() { systemShown = !systemShown; }
    void toggleMultiplayerMenu();
    bool interfaceVisible() const { return hud.interfaceShown(); }
    std::array<int32_t, 3> characterSaveVisibility() const {
        return {charactersShown ? 1 : 0, (titleShown || systemShown) ? 1 : 0, creationShown ? 1 : 0};
    }
    Services &services() { return s; }

  private:
    Services &s;
    Request req;
    Hud ownHud;
    Hud &hud;
    const charactercreation::AccountProvider accountProvider = [steam = s.steam] { return steam ? steam->steamId() : 0; };
    VkExtent2D size{0, 0};
    float guiScale = 1;
    glm::vec2 window{0}, mouse{0}; // GUI units
    glm::vec2 cardFrame{0}, selectFrame{0}, deleteToggleFrame{0}, deleteFrame{0};
    bool clicked = false;
    bool nativeBackPressed = false, nativeControlsHandled = false, producingControlDraw = false;
    bool gameplaySession = false; // Runner mode survives callbacks that clear all frontend pages.
    bool sampledPointerVisited = false;
    uint8_t sampledAtPointer = 0;
    unsigned sampledFixtureStage = 0;
    bool sampledFixturePassed = true;
    bool pointerDown = false, pointerUp = false, leftHeld = false;
    std::vector<gamepadui::Point> navigationPoints;
    void navigationPoint(glm::vec2 at) {
        int width, height;
        glfwGetWindowSize(s.window, &width, &height);
        navigationPoints.push_back({(int32_t)(at.x * guiScale * (float)width / (float)std::max(1u, size.width)),
                                    (int32_t)(at.y * guiScale * (float)height / (float)std::max(1u, size.height))});
    }
    double dtMs = 0;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    // The frame driver's entry tick and its one budget sleep before present (schedule §2.3);
    // the title, selection and creation pages share the gameplay frame driver.
    uint32_t frameEntryTick = 0;
    bool framePaced = false;
    void paceFrame() {
        if (!framePaced && s.options)
            framepacing::sleepBeforePresent(frameEntryTick, uint32_t(s.options->shortestFrameMs()));
        framePaced = true;
    }
    double seconds = 0, nextSteamCheck = 0;
    bool testClicked = false; // Request::clickAfter's click was made
    uint32_t previewPanelMesh = 0;
    float previewPanelScale = 0;

    // The front layers (frontend-title.md §2) and the screens over them.
    std::array<bool,5> frontendPages{true,false,false,false,false};
    bool &titleShown=frontendPages[0], &charactersShown=frontendPages[1], &creationShown=frontendPages[2];
    bool &worldsShown=frontendPages[3], &newWorldShown=frontendPages[4];
    bool optionsOpen = false;
    std::unique_ptr<ControlsScreen> detachedControls; // Owned API permits menu queries without an input service.
    bool &controlsOpen;
    bool riding() const { return titleShown || charactersShown; }
    bool front() const { return std::any_of(frontendPages.begin(),frontendPages.end(),[](bool shown){return shown;}); }
    // Page stores never publish the HUD parent; only the late update does (first-frame contract 2.2).
    void syncInterfaceState() { hud.publishFrontendState({frontendPages,optionsOpen,controlsOpen}); }

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
        bool native = false;
        worldgen::CreatureRecord creature;
        nativepose::Sparks sparks;
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
    std::shared_ptr<nativesaves::Character> stageOwner = std::make_shared<nativesaves::Character>();
    nativesaves::Character &stage = *stageOwner;
    std::array<uint8_t, 0xb0> stageLooks{};
    bool stageHasLooks = false;
    glm::vec3 riderFeet{0}; // local blocks (the stand-in's)
    OrbitCamera camera;
    cameramotion::FieldOfView ownFieldOfView;
    cameramotion::FieldOfView &fieldOfView() { return s.cameraFieldOfView ? *s.cameraFieldOfView : ownFieldOfView; }
    cameraposition::State cameraPosition; // Native Game construction starts all fixed anchors at zero.
    terrainray::World cameraTerrain;
    bool cameraPositionValid = false;
    float hour = 10; // set from Services::startHour when the session starts

    // Character creation.
    Look look;
    HairEditor hairEditor;
    hair::Style appliedHairStyle;
    std::u16string initialName; // initial/test caption; live input belongs to the retained scene editor

    // The world pages (steam-world-list-pages-and-world-entry-persistence-20261111.md). The
    // list lives for this session only: Steam never opens or reads its world store (H2).
    std::vector<worldlist::Entry> worlds;
    int32_t worldCurrent = 0; // G+0x1770: sticky, set by the card under the pointer
    bool worldMultiplayer = false, worldDeleteShown = false;
    const char *worldToggleCaption = worldlist::INITIAL_TOGGLE_CAPTION;
    std::vector<int> worldCardIndex; // card -> list index; the last card is New world
    std::vector<CharacterFrame> worldCards;
    float worldCardScale = 0;
    CharacterScroll worldScroll;
    int worldCharacter = -1; // engine extension: the character chosen before the world
    void rebuildWorldCards();
    void layoutWorldFrames();
    void worldItems(Hud::Screen &out);
    void newWorldItems(Hud::Screen &out);
    void selectWorld();
    bool createWorld(std::u16string_view name, std::u16string_view seed);
    void toggleWorldMode();
    void deleteWorld();
    void enterWithWorld(const worldlist::Entry &world);
    void recordWorld(const worldlist::Entry &world);
    glm::vec2 worldCardAt(size_t k) const { return worldCards[k].at / guiScale; }

    OptionsScreen options;
    ControlsScreen &controls;

    // The logo, as a screen-space model.
    uint32_t logoMesh = 0;
    glm::ivec3 logoSize{0};
    std::optional<guiitem::Request> queuedLogo;

    int settled = 0; // frames with the world loaded, for a capture

    // --- Steps ---
    void loadList();
    void makePreviews();
    void releasePreviews();
    void previewItems(std::vector<DrawItem> &opaque);
    void previewPanels(std::vector<DrawItem> &opaque);
    std::vector<guiitem::Request> characterPreview(Listed &, const Hud::ScreenItem::ModelContext &);
    void putOnStage(const nativesaves::Character &c, bool riding);
    void enterWorld(); // Separate retained-actor load and Continue/Select/creation publications.
    void stageDefaultPlayer();
    void applyLook();
    void applyHairStyle();
    void secondHairItems(Hud::Screen &out);
    void startCreation();
    void confirmCreation();
    nativesaves::Cell startCell(std::array<int64_t, 3> &position) const;
    uint32_t account() const;
    bool frame();
    void readInput(bool poll, double frameMs = 0);
    void routeMessages();
    void publishHudParent() { hud.publishGameplayParent(); }
    gamewindow::Chat chatSource();
    void handleInterfaceKeys(std::vector<int> &keys,bool native = false);
    void handleKeys(bool inGame);
    void handleClick(bool inGame);
    void stepWorld(Camera &cam, Lighting &light, glm::vec3 &focus, std::vector<DrawItem> &opaque, std::vector<DrawItem> &water);
    Hud::Screen buildScreen(bool inGame, bool drawing = true);
    void titleItems(Hud::Screen &out);
    void characterItems(Hud::Screen &out);
    void creationItems(Hud::Screen &out);
    void optionsItems(Hud::Screen &out);
    void controlsItems(Hud::Screen &out);
    void helpItems(Hud::Screen &out);
    void drawHelp(const helpactions::Draw &publish);
    void systemItems(Hud::Screen &out);
    void openOptions();
    void applyOptions();
    void back();
    void compatibilityBack(bool inGame);
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
    Hud::ScreenItem worldControl(const std::string &name, glm::vec2 at, glm::vec2 size, const std::string &caption = "") const {
        auto item = characterControl(name, at, size, caption);
        item.instance = "worlds/" + name;
        return item;
    }
    // The current card's Select, small and Delete World buttons use the character rule (3.4).
    CharacterButtons worldButtonsAt(size_t k) const {
        const auto select = worldControl("button2", {}, SELECT_SIZE);
        const auto toggle = worldControl("smallbutton", {}, DELETE_TOGGLE_SIZE);
        auto buttons = characterButtons(worldCards[k].at, worldCards[k].size, hud.templateViewSize(select),
                                        hud.templateViewSize(toggle));
        buttons.select /= guiScale;
        buttons.deleteToggle /= guiScale;
        buttons.deleteCharacter /= guiScale;
        return buttons;
    }
    Hud::ScreenItem worldToggle() const {
        auto item = widget("button", {window.x - WORLD_TOGGLE_SIZE.x - 20, 20}, WORLD_TOGGLE_SIZE, worldToggleCaption);
        item.instance = "worlds/multiplayer";
        item.branch = uiroot::Branch::Root;
        return item;
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
    Hud::ScreenItem controlsPanel() const;
    void refreshControlsThumb();
    void refreshFriendsThumb();
    void scrollControls(int nativeDelta);
    void dragControls(plasma::Input &, plasma::Scene &, plasma::SceneNode &);
    void scrollFriends(int nativeDelta);
    void refreshFriendStates();
    int hoveredFriend(glm::vec2 m) const; // the row under the pointer (panel units), -1 if none
    glm::vec2 friendButtonAt(int row, bool right) const;
    void friendsItems(Hud::Screen &out);
    bool friendsClick();
    bool done = false;
    Result outcome;
    void decide(Result::Action action, int slot = -1) {
        done = true;
        outcome.action = action;
        if (slot >= 0)
            outcome.slot = slot;
    }
};

bool Session::start(std::string &error) {
    if (s.extracted) {
        cameraTerrain.hasZone = [this](int32_t x, int32_t y) { return s.extracted->gameZoneLoaded(x, y); };
        cameraTerrain.block = [this](int32_t x, int32_t y, int32_t z) {
            const auto block = s.extracted->gameBlock(x, y, z);
            return terrainray::Cell{block[4], block[5]};
        };
    }
    if (!hud.ensureLoaded(s.assets->gameDir(), error))
        return false;
    syncInterfaceState();
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
    // The logo model, lit once and drawn in screen space.
    if (auto path = s.assets->resolve(fs::path("sprites") / "cubeworld.cub")) {
        CubModel cub;
        if (cub.load(*path)) {
            logoSize = {cub.sx, cub.sy, cub.sz};
            logoMesh = s.renderer->upload(guiitem::voxelMesh(cub));
        }
    }
    loadList();
    rebuildWorldCards(); // construction ends with the world card rebuild (3.2)
    // The last character is put on stage (frontend-title.md §1 step 6).
    const std::optional<int> slot = s.store && s.store->isOpen() ? s.store->slot() : std::nullopt;
    if (slot && *slot >= 0 && *slot < (int)listed.size()) {
        selectedSlot = *slot;
        cursor = *slot;
        putOnStage(listed[(size_t)*slot].character, true);
    } else
        stageDefaultPlayer();
    camera.set(90, 0, 180, 8);
    // No publication here: the HUD parent keeps its value (one from construction, or the
    // gameplay value on a return) until this visit's first late update (first-frame contract 2).
    playSound(SOUND_MENU_OPEN);
    return true;
}

void Session::finish() {
    hud.clearFrontendBindings();
    hud.publishFrontendScreen({});
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
    gameplaySession = true;
    if (!s.hud && !hud.ensureLoaded(s.assets->gameDir(), error))
        return false;
    titleShown = charactersShown = creationShown = false;
    syncInterfaceState();
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
        Preview &p = l.preview;
        if (p.loaded) continue;
        if (!p.creature.empty()) {
            p.native = p.loaded = l.playable = true;
            continue;
        }
        std::array<uint8_t, 0xb0> appearance;
        if (s.spawned && s.world && nativeprogress::appearanceOf(l.character.looks, appearance)) {
            p.creature = recordOf(l.character, &appearance);
            std::memcpy(p.creature.data() + worldgen::creature::POSITION, l.character.position.data(), sizeof l.character.position);
            std::memcpy(p.creature.data() + 0xb10, l.character.position.data(), sizeof l.character.position);
            std::memcpy(p.creature.data() + HEALTH, &l.character.health, sizeof l.character.health);
            std::memcpy(p.creature.data() + 0x9a4, &l.character.field9a4, sizeof l.character.field9a4);
            std::memcpy(p.creature.data() + 0x9a8, &l.character.field9a8, sizeof l.character.field9a8);
            p.native = p.loaded = l.playable = true;
            continue;
        }
        if (!l.playable)
            continue;
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
// This recording-only fallback quantizes owner frame colors into a byte mesh.
// Native cards preserve the live blackwidget data in the Plasma draw traversal
// (docs/specs/steam-frontend-panel-backgrounds.md).
void Session::previewPanels(std::vector<DrawItem> &opaque) {
    if (std::none_of(listed.begin(), listed.end(), [](const Listed &l) { return !l.preview.native; }))
        return;
    if (previewPanelMesh && previewPanelScale != guiScale)
        s.renderer->release(std::exchange(previewPanelMesh, 0));
    if (!previewPanelMesh) {
        std::vector<OverlayVertex> vertices;
        hud.drawScreen({characterCard({0, 0})}, vertices);
        Mesh panel;
        // The recording-only compatibility panel uses the ordinary byte-color mesh.
        const auto channel = [](float value) { return uint8_t(std::clamp(value, 0.f, 255.f)); };
        for (const OverlayVertex &v : vertices) {
            panel.vertices.push_back({v.x, v.y, 0, channel(v.r), channel(v.g), channel(v.b), channel(v.a), 4, NATIVE_MENU, 0, 0});
            panel.indices.push_back((uint32_t)panel.indices.size());
        }
        if (panel.indices.empty())
            return;
        previewPanelMesh = s.renderer->upload(panel);
        previewPanelScale = guiScale;
    }
    for (int i = 0; i < (int)listed.size(); i++) {
        if (listed[(size_t)i].preview.native)
            continue;
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
        if (!p.loaded || p.native)
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

std::vector<guiitem::Request> Session::characterPreview(Listed &listedCharacter, const Hud::ScreenItem::ModelContext &context) {
    std::vector<guiitem::Request> requests;
    const auto placement =
        charactercard::geometry(context.matrix, context.pivot, {(int)size.width, (int)size.height}, s.renderer->settings.nativeZoneRadius);
    if (!placement)
        return requests;
    auto &preview = listedCharacter.preview;
    std::vector<nativepose::Draw> draws;
    const charactercard::Frame frame{hud.guiTiming().step, float(dtMs), context.hovered, wallClockMs()};
    charactercard::prepare(
        preview.creature.data(), frame, s.spawned->poseModels(), [this] { return s.spawned->drawWorldRandom(); }, preview.sparks, draws);
    const size_t posed = draws.size();
    for (size_t part = 0; part < posed; ++part)
        if (draws[part].kind == nativepose::WEAPON_EFFECT_DRAW)
            nativepose::spiritDraws(draws[part], draws);
    int16_t hairId;
    std::memcpy(&hairId, preview.creature.data() + 0x92, sizeof hairId);
    for (const auto &part : draws) {
        if (!part.model || part.kind == nativepose::WEAPON_EFFECT_DRAW || !(part.opacity >= 1.f))
            continue; // this native helper never consumes its faded queues
        const int id = int(reinterpret_cast<intptr_t>(part.model->user));
        const auto asset = s.spawned->spriteAsset(*s.world, id);
        if (!asset)
            continue;
        int model = asset->model;
        glm::vec4 material;
        std::memcpy(&material[0], part.tint, sizeof part.tint);
        if (id == hairId && listedCharacter.hairStyle.dual) {
            const glm::u8vec3 primary(glm::clamp(glm::vec3(material), glm::vec3(0), glm::vec3(1)) * 255.f);
            model = s.world->loadModel(s.spawned->spriteName(id), primary, true, &listedCharacter.hairStyle);
            material = {1, 1, 1, material.a};
        }
        const uint32_t mesh = s.world->modelMesh(model);
        if (!mesh)
            continue;
        glm::mat4 transform;
        std::memcpy(&transform[0][0], part.matrix, sizeof part.matrix);
        const auto size = s.world->modelSize(model);
        guiitem::Request request;
        request.kind = guiitem::Request::Kind::Character;
        request.mesh = mesh;
        request.model = (placement->base * transform) * glm::translate(glm::mat4(1), {size.x * .5f, size.y * .5f, 0});
        request.projection = placement->projection;
        request.material = material;
        request.opacity = part.opacity;
        request.depthOffset = part.depthOffset;
        request.mirrored = part.kind == nativepose::MIRRORED_DRAW;
        request.state = guiitem::characterState();
        request.state->glow = part.glow;
        request.state->shininess = part.shininess;
        request.state->white = part.white;
        requests.push_back(std::move(request));
    }
    std::stable_sort(requests.begin(), requests.end(), [](const auto &left, const auto &right) {
        return std::tie(left.mirrored, left.mesh) < std::tie(right.mirrored, right.mesh);
    });
    guiitem::Request setup;
    setup.kind = guiitem::Request::Kind::Character;
    requests.insert(requests.begin(), setup); // nonnull card setup still runs when its pose has no drawable parts
    return requests;
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
    const bool canSearch = s.startSpot.ready();
    const auto rand = [] { return std::rand(); };
    if (!canSearch || !findStartCell(s.startSpot, rand, s.startOrigin, s.farStart ? START_RADIUS_FROM : 0,
                                     s.farStart ? START_RADIUS_SPREAD : 0, START_SEARCH_STEPS, cell)) {
        const int64_t bx = (int64_t)std::floor(home.x) + (frame.originX >> 16), by = (int64_t)std::floor(home.y) + (frame.originY >> 16);
        cell = {(int32_t)std::floor(bx / 16.0), (int32_t)std::floor(by / 16.0)};
        log("Characters: no start spot passes the tests; starting at the world's home");
    } else
        log("Characters: start spot at cell " + std::to_string(cell[0]) + " " + std::to_string(cell[1]));
    const std::array<int32_t, 2> block = startBlock(cell);
    if (s.extracted && s.extracted->generated() && !worldgen::usingAlphaTerrain() && s.startSpot.ground) {
        position = homesearch::position(cell, s.startSpot.ground(block[0], block[1]));
        return cell;
    }
    // Alpha/captured geometry keeps its explicit voxel-surface placement policy.
    float height = home.z;
    if (s.extracted) s.extracted->surface(double(block[0]) - double(frame.originX) / 65536.0,
                                         double(block[1]) - double(frame.originY) / 65536.0, height);
    position = nativeprogress::gamePosition({0, 0, height}, frame);
    position[0] = int64_t(block[0]) * 65536;
    position[1] = int64_t(block[1]) * 65536;
    return cell;
}

// Applying the look (frontend-characters.md §5.4): race, class, wrapped choices, the looks record
// (the preview colour), the appearance rebuilt and the starting kit given.
void Session::applyLook() {
    look.wrap();
    const auto choices = look.record();
    const uint8_t cls = uint8_t(look.cls + 1);
    const bool live = s.spawned && s.spawned->rebuildCharacterCreation(stage, choices, cls);
    if (!live) {
        stage.classType = cls;
        stage.looks = choices;
        const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
        const glm::vec3 feet = nativeprogress::localFeet(stage.position, frame);
        const auto region = worldgen::regionAt((int32_t)std::floor(feet.x + (float)(frame.originX >> 16)),
                                               (int32_t)std::floor(feet.y + (float)(frame.originY >> 16)));
        giveStartingKit(stage, account(), {region.x, region.y});
        stage.health = nativeprogress::maxHealth(stage);
    }
    stageHasLooks = nativeprogress::appearanceOf(stage.looks, stageLooks);
    if (!live && s.spawned)
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
    initialName.clear();
    hud.setFrontendCaption("creation-name",u"");
    look.reset();
    hairEditor.reset(s.newHairStyle);
    const auto home = [&] {
        charactercreation::Placement placement;
        placement.home = startCell(placement.position);
        return placement;
    };
    const bool live = s.spawned && s.spawned->beginCharacterCreation(stageOwner, home);
    if (!live) {
        const auto placement = home();
        stage = newCharacter(placement.home, placement.position);
        applyLook();
    } else {
        stageHasLooks = nativeprogress::appearanceOf(stage.looks, stageLooks);
        applyHairStyle();
    }
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    riderFeet = nativeprogress::localFeet(stage.position, frame);
    if (!live && s.spawned && s.extracted) {
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

// Original Back is a pure prioritized page transition. The live actor/camera remain.
void Session::back() {
    uiroot::State state;
    state.frontend=frontendPages;
    if (uiroot::back(state)) {
        frontendPages=state.frontend;
        syncInterfaceState();
    }
}

// Existing application dismissal adapter, pending the full Game-key phase.
// Its overlay handling is separate from the native Back event2 connection.
void Session::compatibilityBack(bool inGame) {
    if (controlsOpen) {controlsOpen=false;return;}
    if (optionsOpen) {optionsOpen=false;return;}
    if (inGame) {systemShown=friendsOpen=false;return;}
    back();
}

// The frame's time and pointer. The title polls the window itself; in play the game has polled
// it and gives the frame's time.
gamewindow::Chat Session::chatSource() {
    gamewindow::Chat source;
    source.selected=[this] {const auto *node=hud.interfaceBranchRoot(uiroot::Branch::Gameplay); return node && node->visible;};
    source.focused=[this] {return hud.interfaceHasFocus();};
    source.capture=[this] {return controls.waiting;};
    source.current=[this] {return s.chat;};
    source.shift=[this] {return hud.interfaceKeyHeld(0x10);};
    source.submit=s.submitChat;
    return source;
}
void Session::routeMessages() {
    s.input->routeInterfaceMessages(hud, [this](int delta, glm::vec2 point) {
        if (s.inventoryWheel && s.inventoryWheel(delta, point))
            return;
        if (wheel(delta, point))
            return;
        if (s.zoomWheel)
            s.zoomWheel(delta, point);
    },[this](uint8_t key) {
        std::vector<int> keys{GLFW_KEY_F1+int(key-0x70)};
        handleInterfaceKeys(keys,true);
    },[this](gamewindow::Target &target) {
        if (gameplaySession || !s.chat) return;
        target.down=[this](uint8_t key) {gamewindow::chatKeyDown(chatSource(),key);};
        target.character=[this](uint8_t unit) {gamewindow::chatCharacter(chatSource(),unit);};
    });
}

void Session::readInput(bool poll, double frameMs) {
    nativeBackPressed=false;
    nativeControlsHandled=false;
    if (poll)
        glfwPollEvents();
    // Window messages use the UI owner's retained coordinates. Polling
    // a new pointer sample comes after those messages and before XInput.
    const auto &messages = s.input->interfaceMessages;
    pointerDown = std::any_of(messages.begin(), messages.end(), [](const auto &event) {
        return event.kind == MenuInput::InterfaceMessage::LeftDown;
    });
    pointerUp = std::any_of(messages.begin(), messages.end(), [](const auto &event) {
        return event.kind == MenuInput::InterfaceMessage::LeftUp;
    });
    routeMessages();
    if (poll) {
        static const auto defaults=gameoptions::defaultControls();
        hud.updateMenuCaptions(s.controls ? *s.controls : defaults);
    }
    frameEntryTick = framepacing::entryTick(), framePaced = false;
    const auto now = std::chrono::steady_clock::now();
    dtMs = poll ? gamemath::frameMilliseconds(
                      gamemath::millisecondsFromNanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count()))
                : frameMs;
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
    const glm::vec2 framebufferPointer((float)(mx * size.width / std::max(1, ww)), (float)(my * size.height / std::max(1, wh)));
    mouse = framebufferPointer / guiScale;
    leftHeld = (hud.interfaceHeldButtons() & 1) != 0;
    if (std::any_of(s.input->buttons.begin(), s.input->buttons.end(), [](int button) { return button >= 0 && button < 4; })) {
        characterScroll.press();
        worldScroll.press();
    }
    layoutCharacterFrames();
    layoutWorldFrames();
    if (req.page=="frontend-sampled-state-test" && sampledFixtureStage==4) {
        frontendPages.fill(false); // Last frontend visit after a synchronous page transition.
        syncInterfaceState();
    }
    const bool ui = front() || ordinaryUi() || (s.ordinaryUi ? s.ordinaryUi() : s.input->gamepadModal);
    static const auto defaultBindings = gameoptions::defaultControls();
    const auto &bindings = s.controls ? *s.controls : defaultBindings;
    const auto samplePhysical = [&] {
        if (req.page != "frontend-sampled-state-test")
            return s.input->samplePhysical(bindings,req.pointer.x>=0);
        // Explicit CLI device-buffer adapter, at the real producer boundary;
        // no window-key injection or access to the user's physical keyboard.
        const auto binding=bindings[gameoptions::INVENTORY];
        s.input->controlEditor.waiting=sampledFixtureStage==1 ? 0 : -1;
        const bool held=sampledFixtureStage!=3;
        s.input->physical.sample(sampledFixtureStage!=2,
            [&](auto &bytes) {
                bytes={};
                if (held && binding.kind==0 && uint32_t(binding.code)<bytes.size()) bytes[size_t(binding.code)]=0x80;
            },[&](auto &bytes) {
                bytes={};
                if (held && binding.kind==1 && uint32_t(binding.code)<4) bytes[size_t(12+binding.code)]=0x80;
            },[&] {return s.input->physicalTarget(bindings);});
        s.input->lastUsedPadAtMousePoll=s.input->lastUsedPad;
        return s.input->physical.phase==physicalinput::Poll::Normal;
    };
    const bool mouseAdmitted=samplePhysical();
    if (s.sampledControls)
        s.sampledControls->publishPhysical(s.input->physical,s.input->bindingLevels);
    sampledPointerVisited=false;
    if (mouseAdmitted && ui)
        hud.moveInterfacePointer({float(worldgen::trunc32(framebufferPointer.x)), float(worldgen::trunc32(framebufferPointer.y))},
                                 s.inventory);
    if (ui && mouseAdmitted) {
        if (!pointerClockStarted) {
            characterScroll.sampledAt = worldScroll.sampledAt = wallClockMs();
            pointerClockStarted = true;
        }
        // Stationary ordinary-UI polls still advance the filter, before the controller route.
        const uint32_t polledAt = wallClockMs();
        characterScroll.poll((int32_t)(mouse.y * guiScale), polledAt, charactersShown, leftHeld, (int)size.width, cards);
        // World cards drag only while the character page is hidden (contract 3.3).
        worldScroll.poll((int32_t)(mouse.y * guiScale), polledAt, worldsShown && !charactersShown, leftHeld, (int)size.width,
                         worldCards);
    }
    if (mouseAdmitted) {
        if (s.mouseMotion)
            s.mouseMotion({{mx, my}, framebufferPointer, ui, s.input->physical.relativeMotion()});
        else if (ui && !s.input->lastUsedPadAtMousePoll && (hud.interfaceHeldButtons() & 1) && !hud.interfacePointerTarget()) {
            const auto delta = hud.interfacePointerDelta();
            const auto &activeOptions = s.options ? *s.options : options.values;
            cameramotion::mouseTargets(camera.targetPitch, camera.targetYaw, delta.x, delta.y,
                                       activeOptions.cameraSpeed, activeOptions.invertY != 0);
        }
    }
    if (req.page=="frontend-sampled-state-test" && mouseAdmitted) {
        sampledPointerVisited=true;
        sampledAtPointer=s.sampledControls ? s.sampledControls->level(gameoptions::INVENTORY) : 0;
    }
    s.input->sampleGamepad(bindings, req.pointer.x >= 0);
    // Gameplay's caller completes this same sample once, after its menu pass.
    // Frontend owns its completion even when a callback just cleared every page.
    if (!gameplaySession && s.sampledControls)
        s.sampledControls->update(s.input->physical,s.input->bindingLevels,s.input->gamepad,
                                 ui,s.input->gamepadMapMode,s.input->gamepadPanels.any());
    s.input->gamepadPointer = {};
    if (mouseAdmitted) {
        std::vector<gamepadui::Point> points;
        if (!front())
            points = s.input->gamepadNavigationPoints;
        points.insert(points.end(), navigationPoints.begin(), navigationPoints.end());
        s.input->gamepadPointer = s.input->gamepadInput.update(s.input->gamepad, ui, s.input->gamepadMapMode,
                                                            {(float)mx, (float)my}, points, s.input->gamepadPanels);
    }
    const auto &pad = s.input->gamepadPointer;
    if (pad.padBeforeCallbacks)
        s.input->lastUsedPad = true;
    s.input->routeGamepadButtons(hud);
    for (int button = 0; button < 3; ++button) {
        if (pad.pressed[(size_t)button])
            s.input->buttons.push_back(button);
        if (pad.released[(size_t)button])
            s.input->released.push_back(button);
        if (pad.pressed[(size_t)button]) {
            characterScroll.press();
            worldScroll.press();
        }
    }
    leftHeld = (hud.interfaceHeldButtons() & 1) != 0;
    if (pad.pressed[0])
        pointerDown = true;
    if (pad.released[0])
        pointerUp = true;
    if (pad.moveCursor) {
        glfwSetCursorPos(s.window, pad.cursor.x, pad.cursor.y);
    }
    if (pad.navigationSound)
        playSound(SOUND_MENU_SELECT);
    s.input->finishPhysical();
    clicked = pointerDown || pointerUp;
}

// --- Keys --------------------------------------------------------------------------------------

// The interface keys (frontend-options.md §3): F1 the help sheet, F2 and F3 the interface's
// scale and F5 and F6 the map's (both saved at once), F4 the whole interface. They are taken
// out of `keys`.
void Session::handleInterfaceKeys(std::vector<int> &keys,bool native) {
    for (auto it = keys.begin(); it != keys.end();) {
        const int key = *it;
        if (!native && s.input->rawKeyboardSeen && key>=GLFW_KEY_F1 && key<=GLFW_KEY_F6) {
            it=keys.erase(it); // the actual message already ran its synchronous Game effect
            continue;
        }
        switch (key) {
        case GLFW_KEY_F2:
        case GLFW_KEY_F3: { // the interface's scale, saved at once
            const float g=gamewindow::guiScale(hud.interfaceScale(),key==GLFW_KEY_F3);
            hud.setGuiScale(g);
            hud.layout(*s.renderer, size);
            if (s.options) {
                s.options->guiScale = hud.interfaceScale(); // reread after synchronous layout
                if (!req.readOnly)
                    gameoptions::saveOptions(s.root / "options.cfg", *s.options);
            }
            break;
        }
        case GLFW_KEY_F1:
        case GLFW_KEY_F4:
            break; // Window dispatch already applied these before the pointer poll.
        case GLFW_KEY_F5:
        case GLFW_KEY_F6:
            if (s.options) {
                s.options->mapScale=gamewindow::mapScale(s.options->mapScale,key==GLFW_KEY_F6);
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
    // Capture already consumed device-buffer edges before the ordinary frame.
    if (in.physical.phase == physicalinput::Poll::Capturing)
        return;
    handleInterfaceKeys(in.pressed);
    for (int key : in.pressed) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            if (!in.rawKeyboardSeen) compatibilityBack(inGame); // compatibility input has no WM Game phase
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
}

// --- Clicks ------------------------------------------------------------------------------------

void Session::handleClick(bool inGame) {
    if (nativeBackPressed || nativeControlsHandled) return; // the retained callback already handled this UI action
    if (!clicked || controlsOpen)
        return; // Retained Controls callbacks and the cached-row release suffix already ran.
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
        else if (hovered == SYSTEM_CONTROLS)
            controlsOpen = !controlsOpen;
        else if (hovered == SYSTEM_START_MENU)
            decide(Result::Title);
        else if (hovered == SYSTEM_EXIT)
            decide(Result::Quit);
        return;
    }
    if (inGame)
        return;
    if (titleShown) {
        if (!pointerUp)
            return;
        const glm::vec2 m = mouse - rootMenuPosition(window);
        const bool continueShown = selectedSlot >= 0 && selectedSlot < (int)listed.size();
        switch (hoveredLine(rootLines(continueShown), m, ROOT_MENU_SIZE.x)) {
        case START_GAME:
            titleShown = false, charactersShown = true;
            break;
        case OPTIONS:
            openOptions();
            break;
        case EXIT:
            decide(Result::Quit);
            break;
        case CONTINUE:
            if (listed[(size_t)selectedSlot].playable) {
                outcome.continued=true;
                decide(Result::Play, selectedSlot);
            }
            break;
        default:
            break;
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
                if (s.worldPages) { // engine extension: choose the world before gameplay
                    worldCharacter = cursor;
                    worldsShown = true;
                    syncInterfaceState();
                    return;
                }
                outcome.continued=false;
                decide(Result::Play, cursor);
            }
        }
        return;
    }
    if (worldsShown) {
        if (!pointerUp)
            return;
        auto controlHit = [&](const Hud::ScreenItem &item) { return hud.templateHit(item, mouse, /*passiveDescendants=*/true); };
        if (controlHit(worldToggle())) {
            toggleWorldMode();
            return;
        }
        const auto card = std::find(worldCardIndex.begin(), worldCardIndex.end(), worldCurrent);
        if (card == worldCardIndex.end())
            return;
        const auto buttons = worldButtonsAt(size_t(card - worldCardIndex.begin()));
        const bool entry = int64_t(worldCurrent) < int64_t(worlds.size());
        if (entry && worldDeleteShown && controlHit(worldControl("button", buttons.deleteCharacter, DELETE_SIZE, "Delete World"))) {
            deleteWorld();
            return;
        }
        if (entry && controlHit(worldControl("smallbutton", buttons.deleteToggle, DELETE_TOGGLE_SIZE))) {
            worldDeleteShown = true; // 0x9f630: the small button only reveals Delete World
            return;
        }
        if (controlHit(worldControl("button2", buttons.select, SELECT_SIZE, worldlist::selectCaption(worldCurrent, worlds.size()))))
            selectWorld();
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
                applyLook();
                return;
            }
        }
        int column, gridRow;
        if (pointerDown && hairCellAt(m, column, gridRow)) { // committing the colour (0x272470)
            look.committed = look.preview;
            applyLook();
            return;
        }
    }
}

// Bound separately from admission; a native event3 connection invokes this synchronously.
void Session::confirmCreation() {
    // Create Character (frontend-characters.md §5.8).
    creationShown = false;
    syncInterfaceState(); // native callback hides the actual page before reading/committing its name
    const auto name=hud.frontendCaption("creation-name").value_or(initialName);
    if (!(s.spawned && s.spawned->confirmCharacterCreation(stage, name))) {
        stage.name = charactercreation::nameBytes(name);
        nativesaves::prepareNewCharacter(stage);
    }
    hud.setHelpShown(true);
    int index = (int)listed.size();
    if (!req.readOnly && s.store)
        index = s.store->create(stage);
    if (!req.readOnly && s.store && index < 0) {
        creationShown = true;
        syncInterfaceState();
        hud.setHelpShown(false);
        log("Characters: could not save the new character");
        return;
    }
    if (!req.readOnly && s.store) {
        SaveGame metadata;
        metadata.hairStyle = hairEditor.committed;
        if (!s.store->writeNotes(index, metadata.text()))
            log("Characters: could not save the dual-tone appearance settings");
    }
    log("Characters: created \"" + stage.name + "\" as character " + std::to_string(index));
    Listed entry;
    entry.character = stage;
    if (s.spawned)
        if (auto snapshot = s.spawned->creationSelectionSnapshot(stage)) {
            entry.character = std::move(snapshot->character);
            entry.preview.creature = std::move(snapshot->record);
        }
    entry.hairStyle = hairEditor.committed;
    if (s.recorded && s.sprites)
        entry.playable = nativeprogress::entryFor(entry.character, index, *s.recorded, *s.sprites, entry.entry);
    listed.push_back(std::move(entry));
    cards.clear(); cardScale = 0;
    makePreviews();
    selectedSlot = outcome.slot = index;
    outcome.created = true;
    outcome.createdHairStyle = hairEditor.committed;
    decide(Result::Play); // Help remains selected in the shared HUD; no dismissal is required.
}

// --- The world under the screens ---------------------------------------------------------------

void Session::stepWorld(Camera &cam, Lighting &light, glm::vec3 &focus, std::vector<DrawItem> &opaque,
                        std::vector<DrawItem> &water) {
    if (s.spawned && s.steam) s.spawned->publishPlayerAccount(accountProvider);
    hour = std::fmod(hour + (float)(dtMs / 60000.0) * s.minutesPerSecond / 60.0f * 60.0f, 24.0f);
    s.world->advanceClock(dtMs); // (the plants sway by the world's clock, vegetation-wind.md §2)
    glm::vec3 lookAt;
    nativeai::Creature *p = s.spawned && s.extracted ? s.spawned->localPlayer() : nullptr;
    if (p) {
        uint8_t *r = p->record;
        nativecontrol::frontendState(r, titleShown, charactersShown, creationShown);
        // The player on the eagle (frontend-title.md §3.1).
        int64_t position[3];
        std::memcpy(position, r + worldgen::creature::POSITION, sizeof position);
        if (riding()) {
            const int32_t block[2] = {(int32_t)(position[0] / 65536), (int32_t)(position[1] / 65536)};
            std::memcpy(r + FLIGHT_POINT, block, sizeof block);
            std::memcpy(r + FLIGHT_START, position, sizeof position);
        }
        uint32_t toggles;
        std::memcpy(&toggles, r + TOGGLES, 4);
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
        s.spawned->smooth((float)dtMs); // ordinary drawn/link phase, once before camera and pose
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
    glm::vec3 eye;
    auto subject = s.spawned ? s.spawned->cameraSubject(creationShown) : std::nullopt;
    const bool nativeCamera = p && s.extracted && subject.has_value();
    if (nativeCamera) {
        const auto forward = camera.forward();
        cameraPosition.distance = camera.distance;
        cameraposition::update(cameraPosition, *subject, {float(forward.x), float(forward.y), float(forward.z)}, dtMs, cameraTerrain);
        p->record[ABILITY] = subject->ability;
        camera.distance = cameraPosition.distance;
        lookAt = s.extracted->toLocal(cameraPosition.lookAt.data());
        eye = s.extracted->toLocal(cameraPosition.eye.data());
        cameraPositionValid = true;
        fieldOfView().update(s.spawned->playerToggles(), dtMs);
    } else {
        eye = glm::vec3(camera.eye(glm::dvec3(lookAt)));
        if (s.extracted) { // Compatibility stand-in camera, without canonical creature inputs.
            const glm::vec3 toEye = eye - lookAt;
            const float length = glm::length(toEye);
            for (float t = 0.3f; t < length; t += 0.2f)
                if (s.extracted->solidAt(lookAt.x + toEye.x * t / length, lookAt.y + toEye.y * t / length,
                                         lookAt.z + toEye.z * t / length)) {
                    eye = lookAt + toEye * (std::max(0.3f, t - 0.3f) / length);
                    break;
                }
        }
    }
    cam = Camera{eye, lookAt};
    if (nativeCamera) {
        cam.nativeViewRotation = cameramotion::viewRotation(camera.pitch, camera.roll, camera.yaw);
        cam.displayedYawDegrees = float(camera.yaw);
        cam.nativeFixedEye = cameraPosition.eye;
        cam.nativeFixedLookAt = cameraPosition.lookAt;
        cam.nativeFovRadians = fieldOfView().radians();
    }
    focus = lookAt;
    light = lightingAt(hour, s.dayNight);
    s.world->update(focus);
    opaque.clear(), water.clear();
    s.world->collect(opaque, water);
    if (p) {
        s.renderer->prepareScene(cam, light, seconds);
        creaturescene::View view;
        view.pose = s.renderer->steamPoseFrame();
        view.flags = titleShown ? 0x60 : 0x20;
        view.renderOffset = s.renderer->steamRenderOffset();
        view.visible = [&](const auto &sphere) { return s.renderer->steamActorVisible(sphere); };
        s.spawned->draw(*s.extracted, *s.world, dtMs, wallClockMs(), view, opaque);
    }
    queuedLogo.reset();
    if (titleShown && logoMesh) {
        const auto placement = logoPlacement((float)size.width, (float)size.height, guiScale, wallClockMs());
        const float farPlane = (float)s.renderer->settings.nativeZoneRadius * 64.f;
        queuedLogo = logoRequest(placement, logoSize, logoMesh, farPlane);
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
        if (i == (int)listed.size()) {
            out.push_back(characterCard(at));
            out.push_back(text("New character", at + glm::vec2(ENTRY_SIZE.x * 0.5f, 30), 15, 3, WHITE, ALIGN_BOTH));
            continue;
        }
        Listed &l = listed[(size_t)i];
        if (l.preview.native) {
            auto card = characterCard(at);
            card.instance = "character-card/" + std::to_string(i);
            card.modelPointer = mouse * guiScale;
            card.drawModels = [this, character = &l](const auto &context) { return characterPreview(*character, context); };
            out.push_back(std::move(card));
        }
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
    out.push_back(widget("button",BACK_AT,BACK_SIZE,"Back"));
    out.back().instance="frontend-back";
    out.back().branch=uiroot::Branch::Root;
    out.back().press=[this]{nativeBackPressed=true;back();};
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
    out.push_back(widget("blackwidget", panel, LOOK_PANEL_SIZE)); // native outer look-panel source
    out.back().instance = "character-style";
    // Publishing navigation/layout must not rebuild equipment. The owner callback
    // runs only when the actual widget draw is admitted by the scene traversal.
    out.back().drawModels = [this](const Hud::ScreenItem::ModelContext &) {
        if (look.drawPalette(mouse - lookPanelAt()))
            applyLook();
        return std::vector<guiitem::Request>{};
    };
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
    const auto currentName=hud.frontendCaption("creation-name").value_or(initialName);
    const glm::vec2 edit = templateSize("edit", {300, 30});
    const glm::vec2 editAt{window.x * 0.5f - edit.x * 0.5f, window.y - NAME_FIELD_FROM_BOTTOM - edit.y};
    out.push_back(widget("edit",editAt,{300,30}));
    out.back().instance="creation-name";
    out.back().retainText=true;
    out.back().text={initialName.begin(),initialName.end()};
    out.push_back(text("Name", editAt + glm::vec2(0, -8), 12, 3, WHITE, ALIGN_LEFT));
    out.push_back(widget("button",{window.x-CREATE_SIZE.x,window.y-CREATE_FROM_BOTTOM-CREATE_SIZE.y},CREATE_SIZE,
                         "Create Character"));
    out.back().instance="creation-confirm";
    out.back().visible=currentName.size()>=2 && currentName.size()<=15;
    out.back().release=[this]{confirmCreation();};
    out.push_back(widget("button",BACK_AT,BACK_SIZE,"Back"));
    out.back().instance="frontend-back";
    out.back().branch=uiroot::Branch::Root;
    out.back().press=[this]{nativeBackPressed=true;back();};
}

// --- The world pages (steam-world-list-pages-and-world-entry-persistence-20261111.md) ---------

// 0x108450: one card per entry of the current mode, then New world, laid out from the top.
void Session::rebuildWorldCards() {
    worldCardIndex = worldlist::cards(worlds, worldMultiplayer);
    worldCards.assign(worldCardIndex.size(), {});
    for (size_t k = 0; k < worldCards.size(); ++k)
        worldCards[k] = {entryPosition(int(k)) * guiScale, cardFrame * guiScale};
    worldCardScale = guiScale;
}

void Session::layoutWorldFrames() {
    if (worldCardScale == guiScale || worldCardScale == 0)
        return;
    for (auto &card : worldCards) {
        card.at = (card.at / worldCardScale) * guiScale;
        card.size = cardFrame * guiScale;
    }
    worldCardScale = guiScale;
}

void Session::worldItems(Hud::Screen &out) {
    const size_t entries = worlds.size();
    // The card under the pointer becomes current; it stays current off the cards (3.3 step 4).
    for (size_t k = 0; !leftHeld && k < worldCards.size(); ++k)
        if (hud.templateHit(characterCard(worldCardAt(k)), mouse) && worldCurrent != worldCardIndex[k]) {
            worldDeleteShown = false;
            worldCurrent = worldCardIndex[k];
        }
    for (size_t k = 0; k < worldCards.size(); ++k) {
        const glm::vec2 at = worldCardAt(k);
        out.push_back(characterCard(at));
        const int index = worldCardIndex[k];
        if (size_t(index) == entries) {
            out.push_back(text("New world", at + glm::vec2(ENTRY_SIZE.x * 0.5f, 30), 15, 3, WHITE, ALIGN_BOTH));
            continue;
        }
        // 0x294eb0: the name at the card's starting size 15, then two lines at size 12. The
        // 3-D preview needs a source model the contract leaves open (L6) and is not drawn.
        const worldlist::Entry &world = worlds[size_t(index)];
        out.push_back(text(world.name, at + glm::vec2(200, 30), 15, 3, NAME_COLOUR, ALIGN_LEFT));
        out.push_back(text(worldlist::exploredText(world.explored), at + glm::vec2(200, 55), 12, 3, WHITE, ALIGN_LEFT));
        out.push_back(text(worldlist::seedText(world.seed), at + glm::vec2(200, 80), 12, 3, SPECIALIZATION_COLOUR, ALIGN_LEFT));
    }
    const auto card = std::find(worldCardIndex.begin(), worldCardIndex.end(), worldCurrent);
    if (!leftHeld && card != worldCardIndex.end()) {
        const auto buttons = worldButtonsAt(size_t(card - worldCardIndex.begin()));
        out.push_back(worldControl("button2", buttons.select, SELECT_SIZE, worldlist::selectCaption(worldCurrent, entries)));
        if (int64_t(worldCurrent) < int64_t(entries)) {
            out.push_back(worldControl("smallbutton", buttons.deleteToggle, DELETE_TOGGLE_SIZE));
            if (worldDeleteShown)
                out.push_back(worldControl("button", buttons.deleteCharacter, DELETE_SIZE, "Delete World"));
        } else
            worldDeleteShown = false;
    }
    out.push_back(worldToggle());
    out.push_back(widget("button", BACK_AT, BACK_SIZE, "Back"));
    out.back().instance = "frontend-back";
    out.back().branch = uiroot::Branch::Root;
    out.back().press = [this] { nativeBackPressed = true; back(); };
}

// The name and seed fields follow the character name field's geometry: the contract leaves
// the label value that places them open (U5). Both start empty and keep their text.
void Session::newWorldItems(Hud::Screen &out) {
    const glm::vec2 edit = templateSize("edit", {300, 30});
    const auto field = [&](const char *instance, const char *label, float fromBottom) {
        const glm::vec2 at{window.x * 0.5f - edit.x * 0.5f, window.y - fromBottom - edit.y};
        out.push_back(widget("edit", at, {300, 30}));
        out.back().instance = instance;
        out.back().retainText = true;
        out.push_back(text(label, at + glm::vec2(0, -8), 12, 3, WHITE, ALIGN_LEFT));
    };
    field("worlds/name", "Name", NAME_FIELD_FROM_BOTTOM);
    field("worlds/seed", "Seed (must be a number)", SEED_FIELD_FROM_BOTTOM);
    const auto name = hud.frontendCaption("worlds/name").value_or(u"");
    const auto seed = hud.frontendCaption("worlds/seed").value_or(u"");
    out.push_back(widget("button", {window.x - CREATE_SIZE.x - 20, window.y - CREATE_SIZE.y - 20}, CREATE_SIZE, "Create World"));
    out.back().instance = "worlds/create";
    out.back().visible = worldlist::createAdmitted(name, seed);
    out.back().release = [this] {
        createWorld(hud.frontendCaption("worlds/name").value_or(u""), hud.frontendCaption("worlds/seed").value_or(u""));
    };
    out.push_back(widget("button", BACK_AT, BACK_SIZE, "Back"));
    out.back().instance = "frontend-back";
    out.back().branch = uiroot::Branch::Root;
    out.back().press = [this] { nativeBackPressed = true; back(); };
}

// 0x9fd10: hide the page and set the camera; an entry is entered, New world opens creation.
void Session::selectWorld() {
    if (worldCurrent < 0)
        return; // engine guard: the handler itself would read before the list (6.2)
    worldsShown = false;
    syncInterfaceState();
    camera.set(90, 0, 180, 8);
    if (size_t(worldCurrent) < worlds.size()) {
        enterWithWorld(worlds[size_t(worldCurrent)]);
        return;
    }
    newWorldShown = true;
    syncInterfaceState();
}

// 0x9f060. The "num" and entry writes go to a store Steam never opens (H2), so nothing is
// persisted. An unadmitted name or seed is refused: the button is hidden then (4.2).
bool Session::createWorld(std::u16string_view name, std::u16string_view seed) {
    if (!worldlist::createAdmitted(name, seed))
        return false;
    newWorldShown = false;
    syncInterfaceState();
    camera.set(90, 0, 180, 8);
    worldlist::Entry world{charactercreation::nameBytes(name), worldlist::parseSeed(seed), 0};
    worlds.push_back(world);
    worldCurrent = int32_t(worlds.size()) - 1;
    rebuildWorldCards();
    log("Worlds: created \"" + world.name + "\" with seed " + std::to_string(world.seed));
    enterWithWorld(world);
    return true;
}

// 0x9ffa0: flip the list mode and its caption, then rebuild the cards.
void Session::toggleWorldMode() {
    worldMultiplayer = !worldMultiplayer;
    worldToggleCaption = worldlist::toggleCaption(worldMultiplayer);
    rebuildWorldCards();
}

// 0x9e5e0. Steam also deletes Save\world_<name>.db and Save\map_<name>.db; whether those
// files ever exist is open (U4), and names are free text, so no file is deleted here.
void Session::deleteWorld() {
    const std::string name = worldCurrent >= 0 && size_t(worldCurrent) < worlds.size() ? worlds[size_t(worldCurrent)].name : "";
    if (!worldlist::remove(worlds, worldCurrent))
        return;
    log("Worlds: deleted \"" + name + "\"");
    rebuildWorldCards();
}

void Session::enterWithWorld(const worldlist::Entry &world) {
    outcome.world = world;
    outcome.continued = false;
    decide(Result::Play, worldCharacter);
}

// 0x8a500 after the character entry, as in Steam where the character loads before the page.
void Session::recordWorld(const worldlist::Entry &world) {
    if (!s.requestedWorld)
        return;
    if (s.spawned)
        s.spawned->recordWorldSelection(*s.requestedWorld, world.seed, world.name);
    else
        *s.requestedWorld = {world.seed, world.name};
    log("Worlds: recorded \"" + world.name + "\" with seed " + std::to_string(world.seed));
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

Hud::ScreenItem Session::controlsPanel() const {
    auto panel=widget("blackwidget",centred(CONTROLS_SIZE),CONTROLS_SIZE);
    panel.instance="controls-frame";
    panel.branch=uiroot::Branch::Controls;
    return panel;
}
void Session::refreshControlsThumb() {
    controlsThumbFrame = hud.templateContentSize(controlsPanel());
    auto *thumb = hud.frontendNode("controls-thumb");
    if (!thumb) {
        auto item=widget("scrollbutton",{},{});
        item.instance="controls-thumb";
        item.branch=uiroot::Branch::Controls;
        (void)hud.templateContentSize(item); // Only the initial optional clone.
        thumb=hud.frontendNode(item.instance);
    }
    if (!thumb || !thumb->widget) {
        controlsThumb.reset();
        return;
    }
    const float width=thumb->widget->effectiveSize().x;
    controlsThumb=panelscroll::controlsThumb(controls.first,controlsThumbFrame.x,controlsThumbFrame.y,width);
    if (controlsThumb) {
        const auto &bar=*controlsThumb;
        hud.publishFrontendGeometry("controls-thumb",centred(CONTROLS_SIZE)+glm::vec2(bar.x,bar.y),{bar.width,bar.height});
    }
}
void Session::dragControls(plasma::Input &input, plasma::Scene &scene, plasma::SceneNode &thumb) {
    if (!(input.heldButtons() & 1))
        return; // No geometry queries before the manager's held-level admission.
    const auto *frame = hud.frontendNode("controls-frame");
    if (!frame || !controlsThumb)
        return; // Owned scene adapter: absent templates have no invented geometry.
    const float height = scene.contentSize(*frame).y;
    // Frontend templates currently share a branch rather than a native Q parent.
    // Project their actual geometry into that frame; general native matrix/provider
    // equivalence remains a separate boundary, not an axis-aligned hit substitute.
    const auto origin = scene.contentPosition(thumb);
    const auto position = scene.widgetPointer(*frame,origin);
    const float thumbHeight = (scene.widgetPointer(*frame,origin+scene.screenSize(thumb))-position).y;
    const auto delta = scene.widgetPointer(*frame,input.pointer())-scene.widgetPointer(*frame,input.previousPointer());
    const auto changed = panelscroll::controlsDrag(controls.first,height,thumbHeight,position.y,delta.y);
    if (!changed)
        return; // Defined finite-domain policy; native exceptional geometry is not emulated.
    if (req.page == "controls-input-test")
        log("Controls motion operands: height="+std::to_string(height)+" thumbHeight="+std::to_string(thumbHeight)+
            " position="+std::to_string(position.y)+" delta="+std::to_string(delta.y)+
            " row="+std::to_string(changed->first)+" y="+std::to_string(changed->y));
    controls.first = changed->first;
    controlsThumb->y = changed->y;
    const auto current = scene.viewPosition(thumb); // Fresh query after the row publication.
    const auto displacement = glm::mat2(scene.placementMatrix(*frame))*glm::vec2(0,changed->y-position.y);
    scene.moveTo(thumb,{current.x,current.y+displacement.y});
}

void Session::refreshFriendsThumb() {
    friendsThumbFrame = hud.templateSize("blackwidget", FRIENDS_SIZE);
    const float width = hud.templateSize("scrollbutton").x;
    if (width > 0)
        friendsThumb = panelscroll::friendsThumb(firstFriend, friendsThumbFrame.x, friendsThumbFrame.y, width, friendRows.size());
}
void Session::scrollControls(int nativeDelta) {
    const auto frame = hud.templateContentSize(controlsPanel());
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
    const glm::vec2 at = centred(CONTROLS_SIZE);
    auto panel = controlsPanel();
    const glm::vec2 frame = hud.templateContentSize(panel);
    const auto rows = controls.rows(frame.y);
    const glm::vec2 local = hud.templateLocalPoint(panel, hud.interfacePointerPosition());
    if (producingControlDraw)
        controls.beginDraw(s.input->lastUsedPad);
    if (controlsThumbFrame != frame)
        refreshControlsThumb();
    out.push_back(panel);
    if (controlsThumb) {
        const auto &thumb = *controlsThumb;
        out.push_back(widget("scrollbutton", at + glm::vec2(thumb.x, thumb.y), {thumb.width, thumb.height}));
        out.back().instance = "controls-thumb";
        out.back().motion = [this](auto &input,auto &scene,auto &node) { dragControls(input,scene,node); };
    }
    float baseline = 27.f;
    for (int i = rows[0]; i < rows[1]; ++i, baseline += 30.f) {
        const auto &binding = controls.values[size_t(i)];
        glm::vec4 colour = binding == gameoptions::Binding{0,0} ? UNASSIGNED_COLOUR : WHITE;
        const bool waiting = i == controls.waiting;
        if (waiting)
            colour = WAITING_COLOUR;
        else if (producingControlDraw && controls.offerRow(i, baseline, local, hud.templateContentSize(panel).x,
                                                          s.input->lastUsedPad))
            colour = HOVERED;
        out.push_back(text(gameoptions::bindingLabel(i), at + glm::vec2(20, baseline), 12, 3, colour, ALIGN_LEFT));
        const std::wstring formatted = waiting ? L"Press a key" : gameoptions::bindingName(binding);
        const float valueX = (hud.templateContentSize(panel).x - 250.f) * .5f + 240.f;
        out.push_back(text(formatted, at + glm::vec2(valueX, baseline), 12, 3, colour, ALIGN_CENTRE));
    }
    out.push_back(widget("upbutton", at + glm::vec2(frame.x - 30, 10), glm::vec2(0)));
    out.back().instance = "controls-up";
    out.back().press = [this] { nativeControlsHandled = true; scrollControls(1); };
    out.push_back(widget("downbutton", at + glm::vec2(frame.x - 30, frame.y - 140), glm::vec2(0)));
    out.back().instance = "controls-down";
    out.back().press = [this] { nativeControlsHandled = true; scrollControls(-1); };
    const glm::vec2 view = hud.templateViewSize(panel) / guiScale;
    const auto buttonSize = [&](const char *label) {
        auto item = widget("button",{},CONTROLS_BUTTON_SIZE,label);
        item.instance = std::string("controls-")+label;
        item.branch = uiroot::Branch::Controls;
        return hud.templateViewSize(item) / guiScale;
    };
    const auto defaultsSize = buttonSize("Defaults"), okSize = buttonSize("OK"), cancelSize = buttonSize("Cancel");
    const glm::vec2 positions[3] = {{20.f,view.y-defaultsSize.y-20.f},
        {(view.x-40.f-okSize.x)*.5f+20.f,view.y-okSize.y-20.f},
        {view.x-20.f-cancelSize.x,view.y-okSize.y-20.f}};
    const char *const labels[3] = {"Defaults", "OK", "Cancel"};
    for (int button = 0; button < 3; ++button) {
        out.push_back(widget("button",at+positions[button],CONTROLS_BUTTON_SIZE,labels[button]));
        out.back().instance = std::string("controls-") + labels[button];
        out.back().navigation = Hud::ScreenItem::ShapeCentre;
        out.back().release = [this, button] {
            nativeControlsHandled = true;
            if (button == 0)
                controls.values = gameoptions::defaultControls();
            else {
                if (button == 1) {
                    if (s.controls) *s.controls = controls.values;
                    if (!req.readOnly)
                        gameoptions::saveControls(s.root / "controls.cfg", s.controls ? *s.controls : controls.values);
                }
                controlsOpen = false;
                syncInterfaceState();
            }
        };
    }
}

// The retained Help branch supplies action text, not a cloned panel template.
void Session::helpItems(Hud::Screen &out) {
    Hud::ScreenItem group;
    group.kind=Hud::ScreenItem::TextGroup;
    group.drawTexts=[this](const auto &publish) {drawHelp(publish);};
    out.push_back(std::move(group));
}

void Session::drawHelp(const helpactions::Draw &publish) {
    helpactions::Sources source;
    source.modifier=[&] {return s.sampledControls ? s.sampledControls->level(gameoptions::CLASS_SKILL) : uint8_t(0);};
    source.actor=s.helpActor;
    source.binding=[&](int index) {
        const auto &bindings=s.controls ? *s.controls : controls.values;
        return gameoptions::bindingName(bindings[size_t(index)]);
    };
    source.description=[&](int32_t action) {
        const auto result=s.helpTerms ? s.helpTerms->describe(action) : nativeterms::Text{};
        return std::wstring(result.begin(),result.end());
    };
    source.primary=[](auto *actor,uint8_t modifier) {return nativeai::nextHit(actor,modifier!=0);};
    source.secondary=[](auto *actor,uint8_t modifier) {return nativeai::nextCharge(actor,modifier!=0);};
    helpactions::draw(source,publish);
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
    // The provider/list/scroll refresh precedes the toggle in both directions.
    friendRows = s.friends && s.friends->list ? s.friends->list() : std::vector<FriendRow>{};
    refreshFriendsThumb();
    friendsOpen = !friendsOpen;
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

Hud::Screen Session::buildScreen(bool inGame, bool drawing) {
    producingControlDraw = drawing && hud.interfaceShown();
    Hud::Screen out;
    navigationPoints.clear();
    syncInterfaceState();
    const auto append = [&](auto builder, uiroot::Branch branch) {
        const size_t first = out.size();
        (this->*builder)(out);
        for (size_t index = first; index < out.size(); ++index)
            out[index].branch = branch;
    };
    if (inGame) {
        if (systemShown && !optionsOpen && !controlsOpen)
            append(&Session::systemItems, uiroot::Branch::Gameplay);
        if (friendsOpen && !optionsOpen && !controlsOpen)
            append(&Session::friendsItems, uiroot::Branch::Root);
    } else if (titleShown)
        append(&Session::titleItems, uiroot::Branch::Title);
    else if (charactersShown)
        append(&Session::characterItems, uiroot::Branch::Characters);
    else if (creationShown)
        append(&Session::creationItems, uiroot::Branch::Creation);
    else if (worldsShown)
        append(&Session::worldItems, uiroot::Branch::Worlds);
    else if (newWorldShown)
        append(&Session::newWorldItems, uiroot::Branch::NewWorld);
    if (optionsOpen)
        append(&Session::optionsItems, uiroot::Branch::Options);
    if (controlsOpen)
        append(&Session::controlsItems, uiroot::Branch::Controls);
    if (drawing && hud.interfaceShown() && hud.helpShown() && !front())
        append(&Session::helpItems, uiroot::Branch::Help);
    hud.publishFrontendScreen(out);
    if (!hud.interfaceShown()) {
        navigationPoints.clear();
        return {};
    }
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
    if (req.page=="frontend-sampled-state-test") {
        static constexpr std::array<physicalinput::Poll::Phase,5> phases{
            physicalinput::Poll::Normal,physicalinput::Poll::Capturing,physicalinput::Poll::Skipped,
            physicalinput::Poll::Normal,physicalinput::Poll::Normal};
        static constexpr const char *labels[]{"normal frontend publication","capture retains current Game sample",
            "inactive retains current Game sample","normal frontend release","transition frame still publishes"};
        const uint8_t expected=sampledFixtureStage==3 ? 0 : 1;
        const bool pass=s.sampledControls && s.input->physical.phase==phases[sampledFixtureStage] &&
            s.input->bindingLevels[gameoptions::INVENTORY]==expected &&
            s.sampledControls->level(gameoptions::INVENTORY)==expected &&
            s.sampledControls->pressed(gameoptions::INVENTORY)==(sampledFixtureStage==0 || sampledFixtureStage==4) &&
            (phases[sampledFixtureStage]==physicalinput::Poll::Normal ?
                sampledPointerVisited && sampledAtPointer==expected : !sampledPointerVisited);
        log(std::string("Frontend sampled: ")+(pass ? "PASS " : "FAIL ")+labels[sampledFixtureStage]+
            " raw="+std::to_string(s.input->bindingLevels[gameoptions::INVENTORY])+
            " sampled="+std::to_string(s.sampledControls ? s.sampledControls->level(gameoptions::INVENTORY) : 0)+
            " pointer="+(sampledPointerVisited ? std::to_string(sampledAtPointer) : "skip"));
        sampledFixturePassed=sampledFixturePassed && pass;
        if (++sampledFixtureStage==5) {
            s.input->controlEditor.waiting=-1;
            if (!sampledFixturePassed) outcome.error="Frontend sampled owner publication failed";
            decide(Result::Quit);
            return false;
        }
    }
    if (req.page=="first-frame-hud-test") {
        // Explicit owned fixture (steam-first-frame-hud-parent-and-interface-update-order-20261112.md):
        // messages are queued directly, no OS input injection. It runs before this visit's
        // first message drain and late publication.
        bool passed=true;
        const auto verify=[&](bool value,const char *label) {
            log(std::string("First-frame HUD: ")+(value ? "PASS " : "FAIL ")+label); passed=passed && value;
        };
        if (!s.chat) {outcome.error="First-frame HUD fixture missing Chat owner"; decide(Result::Quit); return false;}
        const auto originalChat=*s.chat; const auto originalPages=frontendPages;
        const auto originalKeyboard=s.input->gameKeyboard; const int originalCapture=controls.waiting;
        const bool originalGameplay=gameplaySession;
        const auto *parent=hud.interfaceBranchRoot(uiroot::Branch::Gameplay);
        const auto key=[&](uint16_t unit) {s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::KeyDown,0,unit);};
        verify(parent && parent->visible && titleShown,
               "before the first late publication the HUD parent keeps its default one while the title is shown (2.1)");
        *s.chat=ChatLine{}; controls.waiting=-1;
        key(13); routeMessages();
        verify(s.chat->typing() && s.chat->text().empty(),
               "a first-drain Enter passes all three gates and opens typing on the title page (example 2)");
        publishHudParent();
        verify(parent && !parent->visible, "the first late publication writes zero while the title root is shown (example 1)");
        key(13); routeMessages();
        verify(s.chat->typing(), "a later title Enter fails the HUD-parent gate and typing stays open (example 2)");
        // Return direction in a gameplay-owned visit: the title store is a callback, not a publication.
        gameplaySession=true; frontendPages.fill(false); hud.setInterfaceState({frontendPages,optionsOpen,controlsOpen});
        *s.chat=ChatLine{};
        titleShown=true; syncInterfaceState();
        verify(parent && parent->visible, "return to title retains the gameplay value one until the late update (2.3)");
        // The frontend Chat owner takes the following key (a gameplay-owned visit leaves Chat
        // keys to the game's frame); the parent is still unpublished.
        gameplaySession=false;
        key(13); routeMessages();
        verify(s.chat->typing(), "an Enter after the title callback in the same drain still opens typing (example 5)");
        publishHudParent();
        verify(parent && !parent->visible, "that iteration's publication then writes zero (example 5)");
        *s.chat=originalChat; frontendPages=originalPages; gameplaySession=originalGameplay;
        controls.waiting=originalCapture; s.input->gameKeyboard=originalKeyboard;
        syncInterfaceState(); publishHudParent();
        if (!passed) outcome.error="First-frame HUD-parent contract failed";
        decide(Result::Quit); return false;
    }
    if (req.page=="frontend-chat-test") {
        bool passed=true;
        const auto verify=[&](bool value,const char *label) {
            log(std::string("Frontend Chat: ")+(value ? "PASS " : "FAIL ")+label); passed=passed && value;
        };
        verify(s.chat && !hud.interfaceHasFocus(),"retained Chat owner and null GUI focus are available");
        if (!s.chat) {outcome.error="Frontend Chat fixture missing owner"; decide(Result::Quit); return false;}
        auto originalChat=*s.chat; const auto originalPages=frontendPages;
        const auto originalKeyboard=s.input->gameKeyboard;
        const int originalCapture=controls.waiting;
        const bool originalShown=hud.interfaceShown();
        const auto originalSubmit=s.submitChat;
        const auto draft=[&] {*s.chat=ChatLine{}; s.chat->keyDown(13,false); s.chat->type("kept");};
        const auto parent=[&](bool shown) {
            frontendPages.fill(false); titleShown=!shown;
            hud.setInterfaceState({frontendPages,optionsOpen,controlsOpen});
        };
        const auto key=[&](uint16_t unit) {s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::KeyDown,0,unit);};
        const auto character=[&](uint16_t unit) {s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::Character,0,unit);};
        controls.waiting=-1; parent(false); draft();
        character(0x00e9); routeMessages();
        verify(s.chat->typing() && s.chat->text()==u"kept\uffe9",
               "hidden HUD parent still admits retained typing through the low-byte character route");
        parent(false); draft(); key(8); routeMessages();
        verify(s.chat->text()==u"kept" && s.chat->typing() && s.input->gameKeyboard.raw[8]==1,
               "keydown raw publication precedes its separate HUD-parent Chat gate");
        parent(true); draft(); controls.waiting=0; key(8); character('Z'); routeMessages();
        verify(s.chat->text()==u"keptZ" && s.chat->typing(),
               "Controls capture blocks key editing but does not add a character gate");
        controls.waiting=-1; parent(true); draft(); hud.setInterfaceShown(false);
        key(8); character('Y'); routeMessages();
        verify(s.chat->text()==u"kepY" && s.chat->typing(),
               "hidden manager ancestor does not replace the HUD parent's own selected integer");
        hud.setInterfaceShown(originalShown); parent(false); draft();
        frontendPages.fill(false); syncInterfaceState(); key(8); routeMessages();
        const auto *hudParent=hud.interfaceBranchRoot(uiroot::Branch::Gameplay);
        verify(hudParent && !hudParent->visible && s.chat->text()==u"kept",
               "page callback retains the previous HUD-parent publication until the late update");
        publishHudParent();
        bool duringSubmit=false; std::u16string submitted;
        s.submitChat=[&](const std::u16string &line) {
            duringSubmit=!s.chat->typing() && s.chat->text()==u"kept";
            submitted=line; s.chat->receive(int64_t(-1),line);
        };
        key(13); routeMessages();
        verify(duringSubmit && submitted==u"kept" && !s.chat->typing() && s.chat->text().empty() && s.chat->pending()==1,
               "late publication admits submission before draft clear and retains the pending record");
        draft(); s.input->gameKeyboard.menu=0; key(0x1b); character(0x0100); routeMessages();
        verify(s.input->gameKeyboard.menu==1 && s.chat->typing() && s.chat->text()==u"kept",
               "Escape toggles its independent byte while retained typing ignores a control character");
        *s.chat=std::move(originalChat); frontendPages=originalPages;
        controls.waiting=originalCapture; s.submitChat=originalSubmit; s.input->gameKeyboard=originalKeyboard;
        hud.setInterfaceShown(originalShown); hud.setInterfaceState({frontendPages,optionsOpen,controlsOpen});
        if (!passed) outcome.error="Frontend Chat ingress contract failed";
        decide(Result::Quit); return false;
    }
    if (req.page=="world-pages-test") {
        // Explicit owned page fixture: the handlers are called directly, no OS input injection.
        bool passed=true;
        const auto verify=[&](bool value,const char *label) {
            log(std::string("World pages: ")+(value ? "PASS " : "FAIL ")+label); passed=passed && value;
        };
        const auto savedPages=frontendPages;
        verify(!s.worldPages && !worldsShown && !newWorldShown && worlds.empty() && worldCurrent==0 &&
               worldCardIndex==std::vector<int>{0} && std::string(worldToggleCaption)=="Multiplayer Worlds...",
               "Steam flow by default: hidden pages, an empty session list and one New world card");
        frontendPages.fill(false); newWorldShown=true; back();
        const bool toWorlds=worldsShown && !newWorldShown;
        back();
        verify(toWorlds && charactersShown && !worldsShown, "Back goes world creation, world selection, character selection");
        frontendPages.fill(false); worldsShown=true; syncInterfaceState();
        selectWorld();
        verify(!worldsShown && newWorldShown && !done, "Create on the New world card opens world creation");
        worldCharacter=3;
        verify(!createWorld(u"A",u"5") && !createWorld(u"Alpha",u"5x") && newWorldShown && worlds.empty(),
               "a short name or a non-digit seed creates nothing");
        verify(createWorld(u"Alpha",u"42") && !newWorldShown && worlds.size()==1 && worlds[0].name=="Alpha" &&
               worlds[0].seed==42 && worldCurrent==0 && worldCardIndex==std::vector<int>({0,1}) && done &&
               outcome.action==Result::Play && outcome.slot==3 && outcome.world && outcome.world->seed==42,
               "Create World appends, selects and enters gameplay with the chosen character");
        done=false; outcome={};
        frontendPages.fill(false); worldsShown=true;
        toggleWorldMode();
        const bool multiplayerCards=worldCardIndex==std::vector<int>{1} && std::string(worldToggleCaption)=="Singleplayer worlds...";
        toggleWorldMode();
        worlds.push_back({"online_Shared",7,0}); rebuildWorldCards();
        verify(multiplayerCards && worldCardIndex==std::vector<int>({0,2}) && std::string(worldToggleCaption)=="Multiplayer worlds...",
               "the toggle switches list modes and online_ entries keep their list index");
        worldCurrent=0; selectWorld();
        verify(done && outcome.world && outcome.world->name=="Alpha" && !worldsShown && !newWorldShown,
               "Select on an entry enters gameplay with that world");
        done=false; outcome={}; worldsShown=true;
        worldCurrent=0; deleteWorld();
        verify(worlds.size()==1 && worlds[0].name=="online_Shared" && worldCurrent==-1 && worldCardIndex==std::vector<int>({1}),
               "Delete World closes the gap and leaves no current entry");
        if (s.requestedWorld) {
            const auto previous=*s.requestedWorld;
            recordWorld({"Beta",-9,0});
            verify(s.requestedWorld->seed==-9 && s.requestedWorld->name=="Beta", "the selection is recorded into the requested World pair");
            *s.requestedWorld=previous;
        } else
            verify(false, "the selection is recorded into the requested World pair");
        worlds.clear(); worldCurrent=0; rebuildWorldCards();
        frontendPages=savedPages; syncInterfaceState();
        outcome={}; if (!passed) outcome.error="World pages contract failed";
        decide(Result::Quit); return false;
    }
    if (req.page=="continue-play-test") {
        auto *actor=s.spawned ? s.spawned->localPlayer() : nullptr;
        if (!actor || !actor->record || !s.extracted || selectedSlot<0) {
            outcome.error="Frontend entry fixture requires a selected live character";
            decide(Result::Quit); return false;
        }
        actor->counted={3101}; actor->struck={3202}; actor->skills={3303,3304};
        actor->summonRaces={3405}; actor->group={3506};
        actor->record[0x61]=0xa7; actor->record[0x7d]=0xb8;
        const uint32_t omitted=0x7f123456; std::memcpy(actor->record+0xbbc,&omitted,4);
        camera.set(22,33,44,3); camera.targetPitch=55; camera.targetRoll=66; camera.targetYaw=77;
        outcome.continued=true; decide(Result::Play,selectedSlot); return false;
    }
    if (req.page=="continue-entry-test") {
        // In-memory backend and explicit actual Session entry: no OS injection or owner writes.
        bool passed=true;
        const auto verify=[&](bool value,const char *label) {
            log(std::string("Continue entry: ")+(value ? "PASS " : "FAIL ")+label);
            passed=passed && value;
        };
        nativesaves::CharacterStore database;
        auto *actor=s.spawned ? s.spawned->localPlayer() : nullptr;
        const bool ready=actor && actor->record && database.open(":memory:");
        verify(ready,"owned memory backend and retained live actor are available");
        if (!ready) {outcome.error="Continue fixture missing owned actor/backend"; decide(Result::Quit); return false;}
        nativesaves::Character saved;
        saved.name="ENTRY_TEST"; saved.classType=2; saved.specialization=1;
        saved.position={1000LL*65536+32768,-2000LL*65536-16384,70LL*65536};
        saved.health=-2.5f; saved.level=0;
        saved.inventory.resize(6); saved.inventory[0].push_back({3,{}});
        saved.buffs.resize(1); saved.buffs[0].bytes[0]=0x0d;
        saved.buffs[0].bytes[1]=0xa7; saved.buffs[0].bytes[2]=0xb8; saved.buffs[0].bytes[3]=0xc9;
        if (database.create(saved)!=0) {outcome.error="Continue fixture memory write failed"; decide(Result::Quit); return false;}
        database.setSlot(7); // Independent backend slot must not be published by Continue.
        const auto originalStore=s.store; const bool originalReadOnly=req.readOnly;
        auto originalListed=std::move(listed);
        const auto originalPages=frontendPages;
        const auto originalCamera=camera;
        const int originalSelected=selectedSlot,originalCursor=cursor;
        s.store=&database; req.readOnly=false;
        listed.emplace_back(); listed.back().playable=true;
        listed.back().character=saved;
        listed.back().character.position={11*65536,22*65536,33*65536};
        listed.back().character.health=42; listed.back().character.level=4;
        listed.back().character.buffs.clear(); // Deliberately stale preview data.
        selectedSlot=cursor=0;
        auto *record=actor->record;
        actor->counted={3101}; actor->struck={3202}; actor->group={3506};
        actor->skills={3303,3304}; actor->summonRaces={3405};
        record[0x61]=0xa7; record[0x7d]=0xb8;
        const uint32_t omitted=0x7f123456; std::memcpy(record+0xbbc,&omitted,4);
        camera.set(22,33,44,3); camera.targetPitch=55; camera.targetRoll=66; camera.targetYaw=77; camera.targetDistance=4;
        frontendPages.fill(false); titleShown=true;
        outcome={}; outcome.action=Result::Play; outcome.slot=0; outcome.continued=true;
        enterWorld();
        actor=s.spawned->localPlayer();
        float hp; std::memcpy(&hp,actor->record+0x180,4);
        int32_t level; std::memcpy(&level,actor->record+0x144,4);
        verify(std::memcmp(actor->record+0x10,saved.position.data(),24)==0 && hp==-2.5f && level==0 && actor->record[0x14c]==2,
               "Continue decodes fresh backend position, raw negative health, level and class");
        verify(actor->record==record && actor->counted==std::set<int64_t>{3101} && actor->struck==std::set<int64_t>{3202} &&
               actor->group==std::set<int64_t>{3506} && actor->skills==std::vector<int32_t>{3303,3304} && actor->summonRaces==std::vector<int32_t>{3405},
               "Continue keeps the live allocation and separately retained collections");
        uint32_t afterOmitted; std::memcpy(&afterOmitted,actor->record+0xbbc,4);
        verify(actor->record[0x61]==0xa7 && actor->record[0x7d]==0xb8 && afterOmitted==omitted,
               "entry preserves omitted core and appearance bytes and the outer partial-store boundary");
        verify(actor->buffs.size()==1 && actor->buffs[0]==saved.buffs[0].bytes,
               "loaded buff bytes survive the entry decoder without a later blanket reset");
        verify(camera.pitch==22 && camera.roll==33 && camera.yaw==44 && camera.targetPitch==55 && camera.targetRoll==66 && camera.targetYaw==77 &&
               camera.distance==8 && camera.targetDistance==8,"Continue keeps all six angles and publishes both distances after loading");
        verify(database.slot()==7,"Continue does not rewrite the independent backend slot key");
        std::array<uint8_t,nativepose::CREATURE_SIZE> beforeMiss;
        std::memcpy(beforeMiss.data(),actor->record,beforeMiss.size());
        const auto beforeCounted=actor->counted;
        frontendPages.fill(false); titleShown=true;
        camera.distance=3; camera.targetDistance=4;
        outcome={}; outcome.action=Result::Play; outcome.slot=99; outcome.continued=true;
        enterWorld(); actor=s.spawned->localPlayer();
        verify(std::memcmp(beforeMiss.data(),actor->record,beforeMiss.size())==0 && actor->counted==beforeCounted &&
               !front() && camera.distance==8 && camera.targetDistance==8 && outcome.enteredCharacter &&
               !outcome.enteredCharacter->inventory.empty() && !outcome.enteredCharacter->inventory[0].empty() &&
               outcome.enteredCharacter->inventory[0][0].count==3,
               "backend miss keeps full entity/data state but still publishes frontend exit and camera distances");
        saved.health=0; saved.classType=3; database.save(0,saved);
        frontendPages.fill(false); charactersShown=true;
        camera.set(22,33,44,3); camera.targetPitch=55; camera.targetRoll=66; camera.targetYaw=77;
        outcome={}; outcome.action=Result::Play; outcome.slot=0; outcome.continued=false;
        enterWorld(); actor=s.spawned->localPlayer(); std::memcpy(&hp,actor->record+0x180,4);
        verify(hp==0 && actor->record[0x14c]==3 && database.slot()==0 && camera.pitch==90 && camera.roll==0 && camera.yaw==180 &&
               camera.targetPitch==90 && camera.targetRoll==0 && camera.targetYaw==180 && camera.distance==8 && camera.targetDistance==8,
               "stored Select loads zero health before slot publication and its distinct camera reset");
        int inventoryCount=0;
        if (actor->inventorySource) actor->inventorySource([&](nativeai::InventorySlot slot) {inventoryCount+=slot.count(); return true;});
        verify(outcome.enteredCharacter && outcome.enteredCharacter->health==0 && outcome.enteredCharacter->position==saved.position && inventoryCount==3,
               "gameplay receives the already-live entry data and current canonical inventory");
        s.store=originalStore; req.readOnly=originalReadOnly; listed=std::move(originalListed);
        frontendPages=originalPages; camera=originalCamera; selectedSlot=originalSelected; cursor=originalCursor;
        outcome={}; if (!passed) outcome.error="Selected saved-character entry contract failed";
        decide(Result::Quit); return false;
    }
    if (req.page=="help-action-test") {
        // Explicit owned Help invocation; no OS injection or original-runtime claim.
        Hud::Screen sheet;
        drawHelp([&](const auto &value,auto at,auto colour) {
            sheet.push_back(text(value,at,12,3,colour,ALIGN_LEFT));
            return glm::vec2(339.875f,999.f); // Semantic fixture; actual font flow checked below.
        });
        bool passed=true;
        const auto verify=[&](bool value,const char *label) {
            log(std::string("Help actions: ")+(value ? "PASS " : "FAIL ")+label);
            passed=passed && value;
        };
        const auto at=[&](std::wstring_view value,glm::vec2 point) {
            return std::any_of(sheet.begin(),sheet.end(),[&](const auto &item) {
                return item.kind==Hud::ScreenItem::Text && item.text==value && item.at==point &&
                    item.fontSize==12 && item.outline==3;
            });
        };
        verify(!sheet.empty() && std::all_of(sheet.begin(),sheet.end(),[](const auto &item) {
            return item.kind==Hud::ScreenItem::Text;
        }),"empty-root action text without a panel template");
        verify(!sheet.empty() && sheet.front().at==glm::vec2(0),"primary binding at the native origin");
        const auto description=[&](float y) {
            return std::any_of(sheet.begin(),sheet.end(),[&](const auto &item) {
                return item.kind==Hud::ScreenItem::Text && item.at==glm::vec2(180,y) && !item.text.empty();
            });
        };
        verify(description(0),"primary action uses the installed term dictionary");
        verify(description(20),"secondary action uses the installed term dictionary");
        verify(std::any_of(sheet.begin(),sheet.end(),[](const auto &item) {
            return item.kind==Hud::ScreenItem::Text && item.at==glm::vec2(0,40) && item.text.starts_with(L"Hold ");
        }),"held class action precedes its description");
        verify(at(L"Dodge",{180,60}) && at(L"[25 STAMINA]",{320,60}),"fixed Dodge description and cost");
        verify(at(L"Call/Ride Pet",{180,100}) || at(L"Call/Ride Pet",{180,80}),"pet row follows the optional class action");
        const auto savedPages=frontendPages;
        const bool savedHelp=hud.helpShown(),savedInterface=hud.interfaceShown();
        const auto savedActor=s.helpActor;
        int queries=0;
        s.helpActor=[&] {++queries; return savedActor ? savedActor() : nullptr;};
        frontendPages.fill(false);
        syncInterfaceState();
        publishHudParent(); // Help hangs under the HUD parent, which only the late update selects.
        hud.setHelpShown(true);
        (void)buildScreen(false,false);
        verify(queries==0,"layout publication does not invoke the Help draw body");
        const auto admitted=buildScreen(false,true);
        const bool deferred=queries==0 && std::any_of(admitted.begin(),admitted.end(),[](const auto &item) {
            return item.branch==uiroot::Branch::Help && item.kind==Hud::ScreenItem::TextGroup;
        });
        std::vector<OverlayVertex> helpVertices;
        hud.drawScreen(admitted,helpVertices);
        verify(deferred && queries>0 && !helpVertices.empty(),
               "retained HUD Help body queries only inside its admitted text draw");
        const float savedScale=hud.interfaceScale();
        const auto *helpNode=hud.interfaceBranchRoot(uiroot::Branch::Help);
        hud.layout(*s.renderer,size);
        verify(helpNode && helpNode->transform.translation==glm::vec2(20,float(size.height)/savedScale-250),
               "ordinary Help layout retains its height-relative local translation");
        hud.setGuiScale(2);
        hud.layout(*s.renderer,{640,800});
        verify(helpNode && helpNode->transform.translation==glm::vec2(20,150),
               "scale-two height-800 Help placement matches the original conditional reference");
        hud.layout(*s.renderer,{640,1000});
        verify(helpNode && helpNode->transform.translation==glm::vec2(20,250),
               "height-1000 layout changes Help position without a width term");
        auto localText=text(L"F1",{0,0},12,3,WHITE,ALIGN_LEFT);
        localText.branch=uiroot::Branch::Help;
        auto expectedText=localText;
        expectedText.branch=uiroot::Branch::Unspecified;
        expectedText.at={20,250};
        std::vector<OverlayVertex> actualVertices,expectedVertices;
        hud.drawScreen({localText},actualVertices);
        hud.drawScreen({expectedText},expectedVertices);
        bool sameVertices=!actualVertices.empty() && actualVertices.size()==expectedVertices.size();
        for (size_t index=0;sameVertices && index<actualVertices.size();++index)
            sameVertices=actualVertices[index].x==expectedVertices[index].x &&
                         actualVertices[index].y==expectedVertices[index].y;
        verify(sameVertices,"local Help text uses the actual retained parent hierarchy");
        hud.setGuiScale(1.5625f);
        hud.layout(*s.renderer,{640,1000});
        localText.at={0,20};
        expectedText.at={31.f/1.5625f,640.f/1.5625f};
        actualVertices.clear(); expectedVertices.clear();
        hud.drawScreen({localText},actualVertices);
        hud.drawScreen({expectedText},expectedVertices);
        sameVertices=!actualVertices.empty() && actualVertices.size()==expectedVertices.size();
        for (size_t index=0;sameVertices && index<actualVertices.size();++index)
            sameVertices=actualVertices[index].x==expectedVertices[index].x &&
                         actualVertices[index].y==expectedVertices[index].y;
        verify(sameVertices,"fractional Help scale rounds the root before the separate raster-space row point");
        hud.setGuiScale(savedScale);
        hud.layout(*s.renderer,size);
        Hud::ScreenItem flowGroup;
        flowGroup.kind=Hud::ScreenItem::TextGroup;
        flowGroup.branch=uiroot::Branch::Help;
        glm::vec2 annotationFlow{0},continuation{0};
        bool sawFirst=false,sawSecond=false;
        flowGroup.drawTexts=[&](const auto &paint) {
            std::array<uint8_t,0x970> record{}; record[0x14c]=4; record[0x14d]=1;
            nativeai::Creature actor; actor.record=record.data();
            helpactions::Sources source;
            source.actor=[&] {return &actor;};
            source.secondary=[](auto *,uint8_t) {return 50;};
            helpactions::draw(source,[&](const auto &value,auto at,auto colour) {
                if (value==L"[25 STAMINA]" && at.y==20) {continuation=at; sawSecond=true;}
                const auto flow=paint(value,at,colour);
                if (value==L"[ALL MP]") {annotationFlow=flow; sawFirst=true;}
                return flow;
            });
        };
        helpVertices.clear();
        hud.drawScreen({flowGroup},helpVertices);
        verify(sawFirst && sawSecond && annotationFlow.x>320 &&
               continuation==glm::vec2(float(int64_t(std::trunc(annotationFlow.x))+20),20) && !helpVertices.empty(),
               "second annotation follows actual installed-font flow from the first annotation");
        queries=0;
        hud.setInterfaceShown(false);
        (void)buildScreen(false,true);
        verify(queries==0 && hud.helpShown(),"hidden interface skips Help work and retains its selection");
        helpVertices.clear();
        hud.drawScreen(admitted,helpVertices);
        verify(queries==0 && helpVertices.empty(),"hidden ancestor also suppresses a previously prepared Help draw body");
        s.helpActor=savedActor;
        frontendPages=savedPages;
        hud.setHelpShown(savedHelp);
        hud.setInterfaceShown(savedInterface);
        if (!passed) outcome.error="Help action row contract failed";
        decide(Result::Quit);
        return false;
    }
    handleKeys(false);
    characterScroll.frame((float)dtMs, charactersShown, leftHeld, {(int)size.width, (int)size.height}, cards);
    worldScroll.frame((float)dtMs, worldsShown, leftHeld, {(int)size.width, (int)size.height}, worldCards);
    bool syntheticClick=false;
    if (req.clickAfter >= 0 && !testClicked && seconds >= req.clickAfter && s.world->pendingCount() == 0) {
        syntheticClick=true;
        clicked = pointerDown = pointerUp = true; // (testing: a complete synthetic click)
        testClicked = true;
    }
    (void)buildScreen(false,false); // Bind controls without producing another draw-time candidate.
    if (req.page == "controls-input-test") {
        // Explicit owned queue/scene adapter: no OS injection or native-runtime claim.
        bool passed = true;
        const auto verify = [&](bool value, const char *what) {
            log(std::string("Controls input: ") + (value ? "PASS " : "FAIL ") + what);
            passed = passed && value;
        };
        const auto retained = controls.values;
        const auto active = s.controls ? *s.controls : gameoptions::defaultControls();
        controls.waiting = 0;
        controlsOpen = false;
        verify(capturingControl(), "hidden capture remains sign-admitted");
        s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::RightUp);
        s.input->routeInterfaceMessages(hud);
        verify(controls.waiting == -1, "hidden right release resets capture");
        controlsOpen = true;
        controls.waiting = -1;
        controls.values[gameoptions::UP] = {123, 2};
        auto screen = buildScreen(false);
        const auto oldGameKey = s.input->gameKeyDown;
        bool defaultsBeforeKey = false;
        s.input->gameKeyDown = [&](uint8_t) { defaultsBeforeKey = controls.values == gameoptions::defaultControls(); };
        const auto clickControl = [&](const char *label) {
            const auto found = std::find_if(screen.begin(), screen.end(), [&](const auto &item) {
                return item.kind == Hud::ScreenItem::Template && item.branch == uiroot::Branch::Controls && item.text == wide(label);
            });
            if (found == screen.end()) { verify(false, "retained button exists"); return; }
            const glm::vec2 point = found->at * guiScale + hud.templateViewSize(*found) * .5f;
            hud.moveInterfacePointer(point);
            s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::LeftDown);
            s.input->interfaceMessages.emplace_back(MenuInput::InterfaceMessage::LeftUp);
            s.input->rawKeyMessage(MenuInput::InterfaceMessage::KeyDown,0x41);
            s.input->rawKeyMessage(MenuInput::InterfaceMessage::KeyUp,0x41);
            s.input->routeInterfaceMessages(hud);
        };
        clickControl("Defaults");
        verify(controls.values == gameoptions::defaultControls(), "Defaults runs in the GUI release");
        verify(defaultsBeforeKey, "queued key observes the completed Defaults callback");
        controls.values[gameoptions::UP] = {123, 2};
        clickControl("Cancel");
        verify(!controlsOpen && controls.values[gameoptions::UP] == gameoptions::Binding{123, 2},
               "Cancel hides while retaining pending edits");
        verify(!s.controls || *s.controls == active, "Cancel leaves active bindings unchanged");
        controlsOpen = true;
        screen = buildScreen(false);
        clickControl("OK");
        verify(!controlsOpen && (!s.controls || (*s.controls)[gameoptions::UP] == gameoptions::Binding{123, 2}),
               "OK copies active bindings before hiding");
        s.input->gameKeyDown = oldGameKey;
        controlsOpen = true;
        const bool oldPad = s.input->lastUsedPad, oldInterface = hud.interfaceShown();
        s.input->lastUsedPad = false;
        hud.setInterfaceShown(true);
        screen = buildScreen(false,false);
        const auto panel = std::find_if(screen.begin(),screen.end(),[](const auto &item) { return item.instance == "controls-frame"; });
        if (panel != screen.end()) {
            controls.waiting = -1;
            hud.moveInterfacePointer((panel->at + glm::vec2(100,87)) * guiScale);
            (void)buildScreen(false); // Real cached-point draw produces row2.
            verify(controls.candidate == 2, "owner draw produces the cached row");
            hud.moveInterfacePointer({-10000,-10000});
            hud.setInterfaceShown(false);
            s.input->routeInterfaceRelease(hud,0);
            verify(controls.waiting == 2, "left release arms stale row through hidden ancestor without fresh hit");
            (void)buildScreen(false);
            verify(controls.waiting == 2 && controls.candidate == 2, "hidden ancestor does not execute the draw producer");
            s.input->routeInterfaceRelease(hud,-7);
            verify(controls.waiting == -1, "unknown signed release still resets capture");
            hud.setInterfaceShown(true);
            s.input->lastUsedPad = true;
            controls.waiting = 4;
            (void)buildScreen(false);
            verify(controls.waiting == -1 && controls.candidate == -1, "admitted pad-mode draw clears capture and candidate");
        } else verify(false,"retained Controls frame exists");
        // Independent H600 contract references through the retained owner scene.
        s.input->lastUsedPad = false;
        controls.first = 0;
        controls.waiting = -1;
        controls.candidate = 7;
        refreshControlsThumb();
        screen = buildScreen(false,false);
        const auto control = [&](const char *owner) -> const Hud::ScreenItem * {
            const auto found = std::find_if(screen.begin(),screen.end(),[&](const auto &item) { return item.instance == owner; });
            return found == screen.end() ? nullptr : &*found;
        };
        const auto *frameItem = control("controls-frame"), *upItem = control("controls-up"), *downItem = control("controls-down");
        const auto *defaultsItem = control("controls-Defaults"), *okItem = control("controls-OK"), *cancelItem = control("controls-Cancel");
        if (frameItem && upItem && downItem && defaultsItem && okItem && cancelItem) {
            const auto content = hud.templateContentSize(*frameItem);
            const auto view = hud.templateViewSize(*frameItem) / guiScale;
            const auto defaultsSize = hud.templateViewSize(*defaultsItem) / guiScale;
            const auto okSize = hud.templateViewSize(*okItem) / guiScale;
            const auto cancelSize = hud.templateViewSize(*cancelItem) / guiScale;
            verify(upItem->at == frameItem->at + glm::vec2(content.x - 30.f,10.f) &&
                   downItem->at == frameItem->at + glm::vec2(content.x - 30.f,content.y - 140.f),
                   "scroll arrows use their distinct native layout insets");
            verify(defaultsItem->at == frameItem->at + glm::vec2(20.f,view.y-defaultsSize.y-20.f) &&
                   okItem->at == frameItem->at + glm::vec2((view.x-40.f-okSize.x)*.5f+20.f,view.y-okSize.y-20.f) &&
                   cancelItem->at == frameItem->at + glm::vec2(view.x-20.f-cancelSize.x,view.y-okSize.y-20.f),
                   "buttons use current transformed sizes and the OK height for Cancel");
        } else verify(false,"all retained Controls layout nodes exist");
        const auto *thumbNode = hud.frontendNode("controls-thumb");
        const auto thumbFrame = hud.frontendFrame("controls-thumb");
        if (thumbNode && thumbNode->widget && thumbFrame && frameItem) {
            const auto frameAt = frameItem->at;
            const auto start = thumbFrame->position, size = thumbFrame->size;
            const auto centre = start + size * .5f;
            hud.moveInterfacePointer(centre);
            hud.interfaceLeftDown();
            verify(hud.interfacePointerTarget() == thumbNode && (hud.interfaceHeldButtons() & 1) && thumbNode->widget->buttonHeld,
                   "ordinary thumb press retains the real owner target");
            hud.moveInterfacePointer(centre + glm::vec2(0,45.f*guiScale));
            const auto dragged = hud.frontendFrame("controls-thumb")->position;
            verify(controls.first == 2 && std::abs(dragged.y-start.y-45.f*guiScale) < .01f,
                   "motion publishes continuous thumb Y and truncated row before the next draw");
            verify(controls.candidate == 7 && controls.waiting == -1,
                   "scroll motion preserves the retained candidate and capture row");
            hud.setInterfaceShown(false);
            hud.moveInterfacePointer(centre + glm::vec2(0,1000.f*guiScale));
            verify(controls.first == 9 && hud.interfacePointerTarget() == thumbNode,
                   "captured thumb still moves through a hidden ancestor");
            hud.interfaceLeftUp();
            const auto released = hud.frontendFrame("controls-thumb")->position;
            hud.moveInterfacePointer(centre);
            verify(hud.frontendFrame("controls-thumb")->position == released && !(hud.interfaceHeldButtons() & 1),
                   "release clears the held bit before its reentrant motion callback");
            hud.setInterfaceShown(true);
            scrollControls(1);
            const auto snapped = hud.frontendFrame("controls-thumb")->position; // Before another screen/draw.
            screen = buildScreen(false,false);
            verify(controls.first == 8 && std::abs(snapped.y-(frameAt.y+195.f)*guiScale) < .01f,
                   "later up publication snaps the continuous thumb to the discrete quotient");
        } else verify(false,"retained Controls thumb exists");
        controls.first = 0;
        hud.setInterfaceShown(oldInterface);
        s.input->lastUsedPad = oldPad;
        controls.values = retained;
        controls.waiting = -1;
        controls.candidate = -1;
        controlsOpen = false;
        if (s.controls) *s.controls = active;
        s.input->interfaceMessages.clear();
        s.input->interfacePresses.clear();
        if (!passed) outcome.error = "Controls input fixture failed";
        decide(Result::Quit);
        return false;
    }
    if (syntheticClick) { // explicit CLI test adapter, not an OS input observation
        const auto beforeCamera=camera;
        const auto *beforeActor=s.spawned ? s.spawned->localPlayer() : nullptr;
        hud.interfaceLeftDown();
        hud.interfaceLeftUp();
        if (req.page=="back-test") {
            const bool cameraRetained=camera.pitch==beforeCamera.pitch && camera.roll==beforeCamera.roll &&
                camera.yaw==beforeCamera.yaw && camera.targetPitch==beforeCamera.targetPitch &&
                camera.targetRoll==beforeCamera.targetRoll && camera.targetYaw==beforeCamera.targetYaw &&
                camera.distance==beforeCamera.distance && camera.targetDistance==beforeCamera.targetDistance;
            const bool actorRetained=(s.spawned ? s.spawned->localPlayer() : nullptr)==beforeActor;
            const bool pageChanged=nativeBackPressed && charactersShown && !creationShown && !titleShown;
            const bool passed=cameraRetained && actorRetained && pageChanged && !outcome.created;
            log(std::string("Back event2: ")+(passed ? "PASS" : "FAIL")+
                " page="+std::to_string(pageChanged)+" camera="+std::to_string(cameraRetained)+
                " actor="+std::to_string(actorRetained));
            if (!passed) outcome.error="Back callback changed non-page state or did not dispatch";
            decide(Result::Quit);
        }
    }
    handleClick(false);
    s.input->finishPointerCallbacks();
    s.input->clear();
    // Steam's frame driver runs on every iteration after the drain, so the late publication
    // also follows a callback that ends this visit; only quitting leaves before it (contract 3, 5).
    if (done) {
        if (outcome.action != Result::Quit)
            publishHudParent();
        return false;
    }
    if (glfwWindowShouldClose(s.window)) {
        decide(Result::Quit);
        return false;
    }

    publishHudParent(); // After this visit's messages and page callbacks, independent of F4/draw admission.
    Camera cam;
    Lighting light;
    glm::vec3 focus;
    std::vector<DrawItem> opaque, water;
    bool menuDeferred=true;
    if (req.page=="escape-test")
        for (int index=0;index<6;++index) {
            const auto *node=hud.menuControl(Hud::Menu(index));
            menuDeferred=menuDeferred && node && !node->visible;
        }
    stepWorld(cam, light, focus, opaque, water);
    hud.publishMenuControls(s.input->gameKeyboard.menu); // Same late UI phase with frontend ancestor hidden.
    if (req.page=="escape-test") {
        bool menuPublished=true;
        for (int index=0;index<6;++index) {
            const auto *node=hud.menuControl(Hud::Menu(index));
            menuPublished=menuPublished && node && node->visible;
        }
        const bool pageRetained=frontendPages==std::array<bool,5>{true,false,false,false,false} &&
            !hud.interfaceBranchRoot(uiroot::Branch::Gameplay)->visible;
        const bool passed=menuDeferred && menuPublished && pageRetained && s.input->gameKeyboard.menu==1 &&
            s.input->gameKeyboard.raw[0x1B]==1;
        log(std::string("Frontend Escape: ")+(passed ? "PASS deferred six-node publication and retained title" : "FAIL"));
        if (!passed) outcome.error="Frontend Escape publication changed its dispatch/page boundary";
        decide(Result::Quit);
        return false;
    }
    const Hud::Screen screen = buildScreen(false);
    if (charactersShown) {
        previewPanels(opaque);
        previewItems(opaque);
    }
    std::vector<OverlayVertex> overlay;
    std::vector<glm::vec2> templatePoints;
    hud.clearItemDraws();
    hud.drawScreen(screen, overlay, &templatePoints);
    for (const auto point : templatePoints)
        navigationPoint(point / guiScale);
    auto items = hud.itemDraws();
    auto layers = hud.clipDraws();
    if (queuedLogo) {
        items.insert(items.begin(), *queuedLogo);
        // Every Plasma cut follows the logo, including layers with no preceding triangles.
        for (auto &layer : layers)
            ++layer.before.items;
    }
    s.renderer->overlay(std::move(overlay), std::move(items), std::move(layers));
    s.renderer->beforePresent = [this] { paceFrame(); };
    s.renderer->frame(cam, light, focus, seconds, opaque, water);
    s.renderer->beforePresent = nullptr;
    paceFrame(); // a frame the renderer did not present still takes its budget sleep
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
    if (req.page=="escape-test") s.input->rawKeyMessage(MenuInput::InterfaceMessage::KeyDown,0x1B);
    if (req.page == "characters")
        titleShown = false, charactersShown = true;
    else if (req.page == "worlds" || req.page == "new-world") // explicit captures of unreachable pages
        titleShown = false, (req.page == "worlds" ? worldsShown : newWorldShown) = true;
    else if (req.page == "create" || req.page == "back-test" || req.page.starts_with("create-test:")) {
        titleShown = false;
        startCreation();
        if (req.page.starts_with("create-test:")) {
            const auto initial=wide(req.page.substr(12));
            initialName={initial.begin(),initial.end()};
            hud.setFrontendCaption("creation-name",initialName);
        }
    }
    else if (req.page == "options")
        openOptions();
    else if (req.page == "controls" || req.page == "controls-input-test")
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
        if (outcome.world)
            recordWorld(*outcome.world);
        if (req.page=="continue-play-test" && s.extracted && s.spawned && s.spawned->localPlayer()) {
            // Model a reached synchronous post-load mutation. A second startup
            // decoder would wrongly clear this reset-owned map before gameplay.
            auto guard=s.extracted->lockSourceState();
            s.spawned->localPlayer()->aggro[0x6677]=3.5f;
        }
        outcome.camera = camera;
        if (cameraPositionValid) {
            outcome.cameraPosition = cameraPosition;
            // Entry may reset distance without rebuilding the retained eye/look-at.
            outcome.cameraPosition->distance = camera.distance;
        }
    }
    return outcome;
}

// Confirmed creation enters gameplay with its already-live Entity and Help selected.
// Continue/Select retain their separate saved-character entry path.
void Session::enterWorld() {
    if (outcome.created) {
        if (s.spawned && s.extracted) {
            auto guard = s.extracted->lockSourceState();
            s.spawned->playerSaveSource()(stage); // current confirmed actor, without a second decoder/reset
        }
        outcome.createdCharacter = stageOwner;
        const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
        outcome.feet = nativeprogress::localFeet(stage.position, frame);
        outcome.placed = true;
        titleShown = charactersShown = creationShown = false;
        s.renderer->cutHistory();
        return;
    }
    // Stored Select has an outer World scope. Continue only holds the selected
    // load scope; its later UI/camera publications also occur after a backend miss.
    auto selectionScope=!outcome.continued && s.extracted ? s.extracted->lockSourceState() : sourcestates::Store::Guard{};
    const nativeprogress::Frame frame{s.extracted ? s.extracted->originDotsX() : 0, s.extracted ? s.extracted->originDotsY() : 0};
    if (s.spawned && s.extracted) {
        auto loadScope=s.extracted->lockSourceState();
        auto saved=std::make_shared<nativesaves::Character>();
        const bool found=s.store && s.store->isOpen() && s.store->load(outcome.slot,*saved);
        if (found && s.spawned->loadPlayerSave(saved))
            outcome.enteredCharacter=std::move(saved);
        else {
            // A normally returning lookup miss leaves the actor/data alone. The
            // gameplay compatibility adapter still needs an owned view of that data.
            auto current=std::make_shared<nativesaves::Character>();
            if (s.spawned->playerCharacterSource()(*current)) outcome.enteredCharacter=std::move(current);
        }
        if (const auto *actor=s.spawned->playerRecord()) {
            std::array<int64_t,3> position;
            std::memcpy(position.data(),actor+worldgen::creature::POSITION,sizeof position);
            outcome.feet=nativeprogress::localFeet(position,frame);
            outcome.placed=true;
        }
    } else if (outcome.slot>=0 && outcome.slot<int(listed.size())) {
        // Recorded-model fallback retains its established home-placement policy.
        outcome.feet=enteredFeet(listed[size_t(outcome.slot)].character,frame,s.extracted ? s.extracted->home() : glm::vec3(0));
        outcome.placed=true;
    }
    if (!outcome.continued) {
        // The native callback rereads its choice for this publication after load.
        if (!req.readOnly && s.store && s.store->isOpen()) s.store->setSlot(cursor);
        if (s.spawned && s.spawned->playerRecord()) hud.configureCraftingTabs(s.spawned->playerRecord()[0x14c]);
        camera.set(90,0,180,8);
    } else camera.continueFromTitle();
    titleShown = charactersShown = creationShown = false;
    // The view cuts from the flyover to the player: the anti-aliasing's history is stale.
    s.renderer->cutHistory();
}

// Blocking panels consume their input. Help has its own visibility and never
// clears gameplay input or delays the ordinary control/World paths.
Result::Action Session::gameFrame(double frameMs) {
    readInput(false, frameMs);
    const bool wasOpen = ordinaryUi();
    if (!wasOpen) {
        handleInterfaceKeys(s.input->pressed);
        if (ordinaryUi())
            playSound(SOUND_MENU_OPEN);
        return Result::Stay;
    }
    handleKeys(true);
    (void)buildScreen(true,false);
    handleClick(true);
    s.input->finishPointerCallbacks();
    s.input->pressed.clear(), s.input->buttons.clear(), s.input->released.clear();
    if (!ordinaryUi())
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
    const bool wasOpen = session->ordinaryUi();
    session->toggleSystemMenu();
    if (!wasOpen && session->ordinaryUi() && session->services().audio && !session->services().externalMenuSounds)
        session->services().audio->playInterface(SOUND_MENU_OPEN);
}
void InGameMenus::toggleMultiplayerMenu() {
    const bool wasOpen = session->ordinaryUi();
    session->toggleMultiplayerMenu();
    if (!wasOpen && session->ordinaryUi() && session->services().audio && !session->services().externalMenuSounds)
        session->services().audio->playInterface(SOUND_MENU_OPEN);
}
bool InGameMenus::open() const {
    return session->ordinaryUi();
}
bool InGameMenus::ordinaryUi() const {
    return session->ordinaryUi();
}
bool InGameMenus::capturingControl() const {
    return session->capturingControl();
}
int InGameMenus::bindingCaptureIndex() const {return session->bindingCaptureIndex();}
bool InGameMenus::systemMenuShown() const {return session->systemMenuShown();}
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
std::array<int32_t, 3> InGameMenus::characterSaveVisibility() const {
    return session->characterSaveVisibility();
}

} // namespace frontend
