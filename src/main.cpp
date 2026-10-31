// CubeVulkan: a standalone Vulkan engine that plays in a world built from the
// Steam Cube World install's own sprites, music and sounds.
#include "Assets.h"
#include "Audio.h"
#include "Character.h"
#include "ClassMoves.h"
#include "Progression.h"
#include "PyroCreatures.h"
#include "GameMath.h"
#include "Body.h"
#include "DeathEffects.h"
#include "Speech.h"
#include "DialogueChoices.h"
#include "DialoguePanel.h"
#include "NpcSpeech.h"
#include "SteamTransientLights.h"
#include "Multiplayer.h"
#include "Creatures.h"
#include "NativeLoot.h"
#include "NativeRules.h"
#include "NativePlayer.h"
#include "Actors.h"
#include "Spawned.h"
#include "NativeControl.h"
#include "Combat.h"
#include "Hud.h"
#include "Frontend.h"
#include "GameInput.h"
#include "GameOptions.h"
#include "NativeProgress.h"
#include "ZoneStates.h"
#include "Gear.h"
#include "Saves.h"
#include "TalkScripts.h"
#include "TalkGen.h"
#include "ItemCatalog.h"
#include "CubModel.h"
#include <deque>
#include <set>
#include "Config.h"
#include "Extracted.h"
#include "worldgen/Generator.h"
#include "worldgen/Height.h"
#include "worldgen/Noise.h"
#include "worldgen/Places.h"
#include "worldgen/ZoneContents.h"
#include "SoundRules.h"
#include "GameNames.h"
#include "GameShop.h"
#include "ItemNames.h"
#include "GameCraft.h"
#include "GroundItems.h"
#include "NativeInteract.h"
#include "WorldMap.h"
#include "OverworldEvents.h"
#include "Mods.h"
#include "Missions.h"
#include "Lore.h"
#include "MapMarks.h"
#include "UsePrompt.h"
#include "ItemSelector.h"
#include "Renderer.h"
#include "HudCoin.h"
#include "ChatLine.h"
#include "ChatCommands.h"
#include "FeatText.h"
#include "IdentifyWindow.h"
#include "UpgradeWidget.h"
#include "SteamSession.h"
#include "World.h"
#include <glm/gtc/matrix_transform.hpp>
#include <windows.h> // before GLFW, which defines APIENTRY only when Windows has not
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <dbghelp.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <optional>
#include <thread>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

namespace {
std::ofstream logFile;
void logLine(const std::string &line) {
    if (logFile) {
        logFile << line << "\n";
        logFile.flush();
    }
    std::printf("%s\n", line.c_str());
}
void fatal(const std::string &message) {
    logLine("Fatal: " + message);
    MessageBoxA(nullptr, message.c_str(), "CubeVulkan", MB_ICONERROR | MB_OK);
}
fs::path exeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

MenuInput input;
struct ChatInputEvent {
    enum Kind { Key, Character, MouseDown } kind;
    int value = 0;
    bool shift = false, repeat = false;
};
std::vector<ChatInputEvent> chatInput;
bool captureChatInput = false;

// Third-person player state (Cube World style orbit camera).
struct Player {
    glm::vec3 position{32, 32, 80};
    float yaw = .8f, pitch = .45f, zoom = 14;
    bool fly = false;
    float verticalSpeed = 0;
};

// What the music choice reads of the place at absolute block column (x, y) (sound.md §3.3):
// its place cell, region biome, ground height and place. `shrine` is the shrine flag of the
// player's block; the boss, spirit-world and success states are the caller's.
soundrules::MusicPlace musicPlaceAt(int32_t x, int32_t y, bool shrine) {
    soundrules::MusicPlace m;
    m.cellX = worldgen::floorDiv(x, worldgen::PLACE_SHIFT);
    m.cellY = worldgen::floorDiv(y, worldgen::PLACE_SHIFT);
    m.biome = worldgen::biomeAt(x, y);
    m.ground = worldgen::groundHeight(x, y, 0);
    // A place counts from half its weight in. HOOK: inside a building the game takes the big
    // structure's kind (0x2aa270) instead; CubeVulkan does not yet tell when the player is
    // inside one (the spec does not say how the game does).
    const worldgen::Place p = worldgen::placeAt(x, y);
    m.placeKind = p.weight >= soundrules::PLACE_WEIGHT_MIN ? p.kind : 0;
    m.forestWeight = worldgen::forestRegion(worldgen::neighbourhood(x, y), p.kind, p.weight);
    m.placeHashOdd = (worldgen::random(worldgen::cellKey(m.cellX, m.cellY)) & 1) != 0;
    m.shrine = shrine;
    return m;
}

// The daylight level blended from the regions around a column (the Steam look's lights; its sky
// colours are the Steam palette's, Renderer.cpp).
RegionLook regionLook(const ColumnInfo &c) {
    RegionLook look{0};
    for (int b = 0; b < BIOMES; b++) {
        const RegionSky &s = regionSky(b);
        look.daylight += c.weights[b] * s.daylight;
    }
    return look;
}
} // namespace

static int runSession(int argc, char **argv, bool returningToMenu) {
    const fs::path root = exeDir();
    logFile.close();
    fs::path logName = "CubeVulkan.log"; // --log FILE: another log (two instances side by side)
    for (int i = 1; i + 1 < argc; i++)
        if (std::string(argv[i]) == "--log")
            logName = argv[i + 1];
    logFile.open(root / logName, std::ios::trunc);
    input.clear();
    logLine("CubeVulkan starting");

    // Command line (mostly for screenshots and testing):
    //   --pos X Y  --z Z  --hour H  --yaw R  --pitch R  --zoom Z  --capture FILE.png
    std::string capture;
    std::string captureFeat; // --capture-feat FILE: local display fixture, never an award or network send
    bool featFixtureQueued = false, featFixtureCaptured = false;
    std::vector<std::string> chatTestLines; // --chat-line TEXT: local capture fixture, never transmitted
    std::string captureSpeech; // --capture-speech FILE: first visible eight-unit NPC prefix (testing)
    bool speechCaptured = false;
    std::string captureDeath; // --capture-death PREFIX: frames 0.3 s and 1.2 s after the first death burst (testing)
    bool forceMenu = returningToMenu, directPlay = false;
    std::vector<std::pair<std::string, std::string>> settingOverrides; // --set Section.Key=Value (after CubeVulkan.ini)
    std::string menuPage = "title";
    float menuTime = -1;
    bool checkSteamOnly = false;
    // --showcase "name:scale,name:scale,..." places sprites in a row in front of
    // the player, for browsing the game's .cub files.
    std::string showcase;
    // --part-origin corner|centre: where the game's part matrices put a sprite's voxel origin.
    bool partCorner = true;
    float turn = 0; // --turn R: start facing R radians from away-from-camera (screenshots).
    bool autoAttack = false; // --auto-attack: attack the nearest monster every 0.6 s (combat testing).
    bool autoWalk = false; // --auto-walk: hold forward, turn and jump now and then ([Player] Native testing).
    bool logSounds = false; // --log-sounds ([Audio] LogSounds): every sound placed goes to the log (testing).
    bool autoDodge = false; // --auto-dodge: press the dodge key every 3 s (with --auto-walk: a roll; standing: the class move).
    int64_t crosshairCreature = 0; // the native creature under the crosshair (Game's the targeted creature)
    std::string landName;          // the player's region's name (the info text; the equipment tab's region)
    float guiScaleTest = 0;
    float hoverTest = -1; // --hover-item N: the inventory's item N is under the pointer (testing only).
    float craftTabTest = -1; // --open-crafting T: the crafting widget open on tab T (testing only).
    float mapTest = -1;      // --open-map Z: the world map open at zoom Z (testing only).
    bool flightsTest = false; // --open-flights Z: as --open-map, as a flight master opens it (testing only).
    bool shrinesTest = false; // --open-shrines Z: read-only capture of the native restricted map mode.
    bool missionsTest = false; // --known-missions: every place's mission counts as known on the map (testing only).
    float deathTest = -1;      // --death-marker DX: a tombstone DX blocks east of the start, as after a death (testing only).
    double pointerTestX = -1, pointerTestY = -1; // --pointer X Y: the mouse there, for the map's cursor and the pointer (testing only).
    float clickAfter = -1; // --click-after S: the front screens get a left click at the pointer S seconds in (testing only).
    bool flyTest = false;                         // --fly DX DY: after 5 s, a flight to that far from the start, logged (testing only).
    int32_t flyTestX = 0, flyTestY = 0;
    std::string eventsTest; // --events-test battle|rescue|bounty|chest-sack|chest-boss|landmarks: the mod's /events command (testing only).
    float eventsStatus = -1; // --events-status T: /events status after T seconds (testing only).
    float eventsUse = -1;    // --events-use T: the use key on every event creature, twice over, after T seconds (testing only).
    float vendorTest = -1; // --open-vendor CLASS: trade with the nearest vendor of that class after 2 s (testing only). // --gui-scale S: the interface at this scale instead of options.cfg's (testing only).
    bool identifyTest = false; // --open-identify: the identify window beside the inventory after 2 s, the first unidentified item in its slot (testing only).
    float upgradeTest = -1;    // --open-upgrade H: the weapon upgrade widget on hand H (0 right, 1 left) after 2 s (testing only).
    bool upgradeDemo = false;  // --upgrade-demo: with --open-upgrade, one cube of each of the first three palette entries set through the widget's click (testing only).
    float closeInventoryAt = -1; // --close-inventory T: the inventory closed T s in (testing only: the weapon's cubes seen in the world).
    bool undying = false;        // --undying: the player's health is kept full (testing only).
    bool openSystemMenu = false; // --open-system-menu: the system menu shown over the game from the start (testing only).
    float openFriendsAt = -1;    // --open-multiplayer-menu T: the multiplayer menu opened T s in (testing only).
    bool testDrop = false;       // --test-drop: drops a copy of the right-hand weapon beside the player (testing only).
    bool autoUse = false;        // --auto-use: presses the use key once, 3 s in (testing only).
    bool nextWeapon = false;     // --next-weapon: start with the next measured right-hand weapon (screenshots).
    bool openInventory = false;  // --inventory: start with the inventory screen open (screenshots).
    bool openSelector = false;   // --ring-menu: read-only selector comparisons
    bool noTaa = false, noSmaa = false;  // --no-taa, --no-smaa (comparisons)
    float spin = 0;                      // --spin R: turn the camera R radians per second (testing reprojection)
    float benchmark = 0;                 // --benchmark N: vsync off, run N seconds, log frame rate and GPU times, quit
    float runSeconds = 0;                // --run-seconds N: with --capture, play N seconds before the screenshot.
    float attackMultiplier = 1;          // --attack-multiplier X: testing only, scales the player's damage.
    float poseAbility = 0, poseTime = 0; // --pose-ability ID SECONDS: hold a recorded move (screenshots).
    std::string wantedCharacter;         // --character NAME|FOLDER|CLASS: which recorded character to play
    bool listOnly = false;       // --list-characters: log the recorded characters and quit
    bool noSave = false;         // --no-save: neither load nor write this run's progress
    std::string saveFolder;      // --save-folder DIR: another Save folder (testing; Save/ beside the exe by default)
    std::string settingsFolder;  // --settings-folder DIR: where options.cfg and controls.cfg are (testing; beside the exe by default)
    int talkTo = -1;             // --talk N: step up to the Nth nearest townsperson and talk (testing)
    float talkAgain = -1;        // --talk-again S: back to that townsperson after S seconds (testing)
    int hostPort = -1;           // --host [port]: host a multiplayer game
    std::string joinAddress;     // --join address[:port]: join one
    bool hostSteam = false;      // --host-steam: host through Steam (friends join by account)
    bool serverMode = false;     // --server [port|steam]: a dedicated server (no window, no player of its own)
    std::string joinSteam;       // --join-steam friend|steamid: join a Steam host
    std::vector<std::string> invites; // --invite friend|steamid: invite a Steam friend (repeatable)
    bool offline = false;        // --offline: no Steam networking (single player through the loopback only)
    float netTrace = -1;         // --net-trace T: for a second from T s, each frame's mirrored abilities in the log (testing)
    float testCoins = -1;        // --test-coins T: the host lays 5 coins by its player at T s (testing pick-ups)
    float netLoss = 0;           // --net-loss F: drop that fraction of outgoing packets (testing)
    bool hiddenWindow = false;   // --hidden: play in a window that is never shown (multiplayer tests beside the owner's desktop)
    float startHour = -1, startZ = NAN, startX = NAN, startY = NAN, startYaw = NAN, startPitch = NAN, startZoom = NAN;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&](float &out) {
            if (i + 1 < argc)
                out = std::strtof(argv[++i], nullptr);
        };
        if (a == "--pos" && i + 2 < argc) {
            next(startX);
            next(startY);
        } else if (a == "--hour")
            next(startHour);
        else if (a == "--z")
            next(startZ);
        else if (a == "--yaw")
            next(startYaw);
        else if (a == "--pitch")
            next(startPitch);
        else if (a == "--zoom")
            next(startZoom);
        else if (a == "--capture" && i + 1 < argc)
            capture = argv[++i];
        else if (a == "--capture-feat" && i + 1 < argc)
            captureFeat = argv[++i];
        else if (a == "--chat-line" && i + 1 < argc)
            chatTestLines.push_back(argv[++i]);
        else if (a == "--capture-speech" && i + 1 < argc)
            captureSpeech = argv[++i];
        else if (a == "--capture-death" && i + 1 < argc)
            captureDeath = argv[++i];
        else if (a == "--showcase" && i + 1 < argc)
            showcase = argv[++i];
        else if (a == "--pose-ability" && i + 2 < argc) {
            next(poseAbility);
            next(poseTime);
        }
        else if (a == "--attack-multiplier")
            next(attackMultiplier);
        else if (a == "--run-seconds")
            next(runSeconds);
        else if (a == "--benchmark")
            next(benchmark);
        else if (a == "--spin")
            next(spin);
        else if (a == "--no-taa")
            noTaa = true;
        else if (a == "--no-smaa")
            noSmaa = true;
        else if (a == "--inventory")
            openInventory = true;
        else if (a == "--ring-menu")
            openSelector = true;
        else if (a == "--next-weapon")
            nextWeapon = true;
        else if (a == "--auto-attack")
            autoAttack = true;
        else if (a == "--auto-dodge")
            autoDodge = true;
        else if (a == "--auto-walk")
            autoWalk = true;
        else if (a == "--log-sounds")
            logSounds = true;
        else if (a == "--gui-scale")
            next(guiScaleTest);
        else if (a == "--hover-item")
            next(hoverTest);
        else if (a == "--open-crafting")
            next(craftTabTest);
        else if (a == "--fly" && i + 2 < argc) {
            flyTest = true;
            flyTestX = std::atoi(argv[++i]);
            flyTestY = std::atoi(argv[++i]);
        } else if (a == "--pointer" && i + 2 < argc) {
            pointerTestX = std::atof(argv[++i]);
            pointerTestY = std::atof(argv[++i]);
        } else if (a == "--click-after")
            next(clickAfter);
        else if (a == "--open-map")
            next(mapTest);
        else if (a == "--open-shrines") {
            next(mapTest);
            shrinesTest = true;
        } else if (a == "--known-missions")
            missionsTest = true;
        else if (a == "--death-marker")
            next(deathTest);
        else if (a == "--open-flights") {
            next(mapTest);
            flightsTest = true;
        }
        else if (a == "--events-test" && i + 1 < argc)
            eventsTest = argv[++i];
        else if (a == "--events-status")
            next(eventsStatus);
        else if (a == "--events-use")
            next(eventsUse);
        else if (a == "--open-vendor")
            next(vendorTest);
        else if (a == "--open-identify")
            identifyTest = true;
        else if (a == "--open-upgrade")
            next(upgradeTest);
        else if (a == "--upgrade-demo")
            upgradeDemo = true;
        else if (a == "--close-inventory")
            next(closeInventoryAt);
        else if (a == "--undying")
            undying = true;
        else if (a == "--open-system-menu")
            openSystemMenu = true;
        else if (a == "--open-multiplayer-menu")
            next(openFriendsAt);
        else if (a == "--test-drop")
            testDrop = true;
        else if (a == "--auto-use")
            autoUse = true;
        else if (a == "--turn")
            next(turn);
        else if (a == "--part-origin" && i + 1 < argc)
            partCorner = std::string(argv[++i]) != "centre";
        else if (a == "--check-steam")
            checkSteamOnly = true;
        else if (a == "--character" && i + 1 < argc)
            wantedCharacter = argv[++i];
        else if (a == "--list-characters")
            listOnly = true;
        else if (a == "--no-save")
            noSave = true;
        else if (a == "--save-folder" && i + 1 < argc)
            saveFolder = argv[++i];
        else if (a == "--settings-folder" && i + 1 < argc)
            settingsFolder = argv[++i];
        else if (a == "--talk" && i + 1 < argc)
            talkTo = std::atoi(argv[++i]);
        else if (a == "--talk-again")
            next(talkAgain);
        else if (a == "--host") {
            hostPort = Multiplayer::DEFAULT_PORT;
            if (i + 1 < argc && std::isdigit((unsigned char)argv[i + 1][0]))
                hostPort = std::atoi(argv[++i]);
        } else if (a == "--join" && i + 1 < argc)
            joinAddress = argv[++i];
        else if (a == "--host-steam")
            hostSteam = true;
        else if (a == "--server") {
            serverMode = directPlay = true; // (it keeps Save/world.db; --no-save keeps nothing)
            if (i + 1 < argc && std::string(argv[i + 1]) == "steam")
                hostSteam = true, i++;
            else {
                hostPort = Multiplayer::DEFAULT_PORT;
                if (i + 1 < argc && std::isdigit((unsigned char)argv[i + 1][0]))
                    hostPort = std::atoi(argv[++i]);
            }
        }
        else if (a == "--join-steam" && i + 1 < argc)
            joinSteam = argv[++i];
        else if (a == "--invite" && i + 1 < argc)
            invites.push_back(argv[++i]);
        else if (a == "--offline")
            offline = true;
        else if (a == "--net-trace")
            next(netTrace);
        else if (a == "--test-coins")
            next(testCoins);
        else if (a == "--net-loss")
            next(netLoss);
        else if (a == "--hidden")
            hiddenWindow = true;
        else if (a == "--menu")
            forceMenu = true;
        else if (a == "--menu-time") {
            next(menuTime);
            forceMenu = true;
        }
        else if (a == "--menu-page" && i + 1 < argc) {
            forceMenu = true;
            menuPage = argv[++i];
        }
        else if (a == "--play")
            directPlay = true;
        else if (a == "--set" && i + 1 < argc) {
            const std::string kv = argv[++i];
            const size_t dot = kv.find('.'), eq = kv.find('=');
            if (dot != std::string::npos && eq != std::string::npos && dot < eq)
                settingOverrides.push_back({kv.substr(0, eq), kv.substr(eq + 1)});
        }
    }

    // Settings: the existing Steam mods' INI files are read first (so their
    // tuning carries over), then CubeVulkan.ini overrides them.
    Config config;
    Config bootstrap;
    bootstrap.load(root / "CubeVulkan.ini");
    Assets assets;
    if (!assets.locate(bootstrap.text("Game", "Path"))) {
        fatal("Steam Cube World was not found.\n\nSet [Game] Path=... in CubeVulkan.ini to the folder "
              "that contains cubeworld.exe.");
        return 1;
    }
    logLine("Game assets: " + utf8(assets.gameDir()));
    SteamSession steam;
    std::string steamError;
    if (!steam.start(assets.gameDir(), steamError)) {
        if (checkSteamOnly)
            logLine("Steam: " + steamError);
        else
            fatal(steamError);
        return 1;
    }
    logLine("Steam: online account and Cube World license verified");
    if (checkSteamOnly)
        return 0;

    const bool explicitCharacter = !wantedCharacter.empty();
    // Which recorded character to play: --character, else [Game] Character=, else
    // the most recently recorded one.
    // Progress is kept in the game's own format (docs/saves.md, NativeProgress.h) in CubeVulkan's
    // Save folder beside the engine: characters.db and world.db. [Saves] ImportFrom=<folder>
    // copies the game's two files in (read only) when CubeVulkan has none yet. The owner's real
    // saves are never written.
    nativeprogress::ProgressFile saves;
    const bool savesWritable = !noSave && capture.empty() && captureSpeech.empty() && captureFeat.empty() && benchmark <= 0 && !listOnly;
    saves.open(saveFolder.empty() ? root / "Save" : pathOfUtf8(saveFolder), assets.gameDir() / "Save", pathOfUtf8(bootstrap.text("Saves", "ImportFrom")), savesWritable,
               [](const std::string &line) { logLine(line); });
    nativeprogress::SpriteTable sprites; // (the game's model list, for the looks of saved characters)
    sprites.load(assets.gameDir() / "Extract");
    // Recorded characters (their animations), then the characters of characters.db.
    auto everyCharacter = [&] { return saves.characters(listCharacters(assets.gameDir()), sprites); };
    std::vector<CharacterEntry> characters = everyCharacter();
    for (auto &c : characters)
        logLine("Recorded character: " + c.describe());
    if (listOnly)
        return 0;
    if (wantedCharacter.empty())
        wantedCharacter = bootstrap.text("Game", "Character");
    const CharacterEntry *chosen = pickCharacter(characters, wantedCharacter);
    if (!chosen && !wantedCharacter.empty()) {
        logLine("Character: no recorded character \"" + wantedCharacter + "\"; playing the newest");
        chosen = pickCharacter(characters, "");
    }
    if (bootstrap.flag("Game", "ImportSteamModSettings", true))
        for (auto name : {"SteamShadows", "MoonLight", "TerrainVariance", "WaterLilies", "WorldMotion", "DualHair", "TemporalAA"})
            if (config.load(assets.gameDir() / "Mods" / (std::string(name) + ".ini")))
                logLine(std::string("Imported settings from Mods/") + name + ".ini");
    config.load(root / "CubeVulkan.ini");
    config.load(root / "Menu.ini");
    for (auto &[key, value] : settingOverrides) // (the command line wins over the menu's choices)
        config.set(key.substr(0, key.find('.')), key.substr(key.find('.') + 1), value);
    if (!explicitCharacter) {
        wantedCharacter = config.text("Game", "Character", wantedCharacter);
        chosen = pickCharacter(characters, wantedCharacter);
        if (!chosen) chosen = pickCharacter(characters, "");
    }
    assets.addOverrides(root / "mods");
    for (auto &layer : assets.overrides())
        logLine("Asset override folder: " + utf8(layer));

    // The game's settings files (options.cfg, controls.cfg) in CubeVulkan's folder, completed with
    // the game's defaults as the game does at start-up.
    const fs::path settingsRoot = settingsFolder.empty() ? root : pathOfUtf8(settingsFolder);
    gameoptions::Options gameOptions;
    gameoptions::Controls gameControls = gameoptions::defaultControls();
    if (savesWritable)
        gameoptions::loadAndComplete(settingsRoot, gameOptions, gameControls);
    else
        gameoptions::loadOptions(settingsRoot / "options.cfg", gameOptions), gameoptions::loadControls(settingsRoot / "controls.cfg", gameControls);
    // What the command line sets itself (--set) wins over the options.
    auto setOnCommandLine = [&](const std::string &key) {
        return std::any_of(settingOverrides.begin(), settingOverrides.end(), [&](const auto &kv) {
            std::string k = kv.first;
            std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return k == key;
        });
    };
    // Captures, benchmarks and the dedicated server keep a window of the INI's size: a test run
    // does not take the owner's screen.
    if (!captureFeat.empty() && runSeconds <= 0) runSeconds = 6;
    const bool testRun = !capture.empty() || !captureSpeech.empty() || !captureFeat.empty() || benchmark > 0 || runSeconds > 0 || serverMode || hiddenWindow;

    if (!glfwInit()) {
        fatal("Cannot initialise the window system.");
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (serverMode || hiddenWindow)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // a dedicated server shows nothing; nor does a hidden test run
    // The window (options.cfg's fullscreen, resolutionX/Y and refreshRate, frontend-options.md
    // §1.2): full screen on the main display at the chosen mode, or the desktop's when none is
    // chosen; a window of the chosen size, or of CubeVulkan's [Graphics] Width and Height.
    const int iniWidth = (int)config.number("Graphics", "Width", 1600, 640, 7680);
    const int iniHeight = (int)config.number("Graphics", "Height", 900, 360, 4320);
    struct WindowMode {
        bool fullscreen = false;
        int width = 0, height = 0, refresh = 0;
    };
    if (guiScaleTest > 0)
        gameOptions.guiScale = std::clamp(guiScaleTest, gameoptions::GUI_SCALE_MIN, gameoptions::GUI_SCALE_MAX);
    auto windowModeOf = [&](const gameoptions::Options &o) {
        WindowMode m;
        m.fullscreen = o.fullscreen != 0 && !testRun;
        const GLFWvidmode *desktop = glfwGetPrimaryMonitor() ? glfwGetVideoMode(glfwGetPrimaryMonitor()) : nullptr;
        const bool chosen = o.resolutionX > 0 && o.resolutionY > 0 && !testRun;
        if (m.fullscreen) {
            m.width = chosen ? o.resolutionX : desktop ? desktop->width : iniWidth;
            m.height = chosen ? o.resolutionY : desktop ? desktop->height : iniHeight;
            m.refresh = o.refreshRate > 0 ? o.refreshRate : desktop ? desktop->refreshRate : GLFW_DONT_CARE;
        } else {
            m.width = chosen ? o.resolutionX : iniWidth;
            m.height = chosen ? o.resolutionY : iniHeight;
        }
        return m;
    };
    WindowMode windowMode = windowModeOf(gameOptions);
    if (windowMode.fullscreen)
        glfwWindowHint(GLFW_REFRESH_RATE, windowMode.refresh);
    GLFWwindow *window = glfwCreateWindow(windowMode.width, windowMode.height, "CubeVulkan",
                                          windowMode.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    glfwSetScrollCallback(window, [](GLFWwindow *, double, double y) { input.scroll += y; });
    captureChatInput = false;
    glfwSetCharCallback(window, [](GLFWwindow *, unsigned int c) {
        if (captureChatInput) chatInput.push_back({ChatInputEvent::Character, (int)(c & 255)});
        if (c >= 32 && c < 127)
            input.typed += (char)c; // names are plain text, as the game's
    });
    glfwSetKeyCallback(window, [](GLFWwindow *, int key, int, int action, int modifiers) {
        if (action == GLFW_PRESS) input.pressed.push_back(key);
        if (captureChatInput && (action == GLFW_PRESS || action == GLFW_REPEAT))
            chatInput.push_back({ChatInputEvent::Key, key, (modifiers & GLFW_MOD_SHIFT) != 0, action == GLFW_REPEAT});
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *, int button, int action, int) {
        if (action == GLFW_PRESS) {
            input.buttons.push_back(button);
            if (captureChatInput) chatInput.push_back({ChatInputEvent::MouseDown, button});
        }
        else if (action == GLFW_RELEASE)
            input.released.push_back(button);
    });

    RenderSettings renderSettings;
    renderSettings.read(config);
    logLine(std::string("Render look: ") + (renderSettings.style == RenderStyle::Alpha ? "Alpha" : "Steam"));
    renderSettings.nativeZoneRadius = gameOptions.zonesDrawn();
    // The options' vsync and render distance ([Graphics] VSync and ViewDistance unless the command
    // line sets them): the default render distance (50 %) is CubeVulkan's 400 blocks.
    if (!setOnCommandLine("graphics.vsync"))
        renderSettings.vsync = gameOptions.vsync != 0;
    if (!setOnCommandLine("graphics.viewdistance"))
        renderSettings.fogDistance = gameOptions.viewDistance();
    if (benchmark > 0)
        renderSettings.vsync = false;
    if (noTaa)
        renderSettings.taa = false;
    if (noSmaa)
        renderSettings.smaa = false;
    Renderer renderer;
    std::string error;
    if (serverMode)
        renderer.initHeadless(renderSettings); // no graphics card needed
    else if (!renderer.init(window, renderSettings, error)) {
        fatal("Vulkan initialisation failed: " + error);
        return 1;
    }
    logLine("GPU: " + renderer.deviceName());

    TerrainSettings terrainSettings;
    terrainSettings.read(config);
    logLine(std::string("Terrain: ") + (terrainSettings.alphaStyle ? "Alpha" : "Variance") +
        " style, regions " + std::to_string((int)terrainSettings.regionSize) + " blocks, seed " +
        std::to_string(terrainSettings.seed));
    Terrain terrain(terrainSettings);
    int viewZones = renderSettings.style == RenderStyle::Steam ? gameOptions.zonesDrawn() : (int)std::ceil(renderSettings.fogDistance / ZONE);
    // [World] Source=Extracted (default): the real world blocks
    // saved from the user's game by tools/ZoneExtractor. Generated: CubeVulkan's
    // own generator.
    // Game: the game's own world from CubeVulkan's reimplementation of its generator
    // (blocks, objects and models), with [World] Scale (1 = the game, 1.5 = regions, landmasses and
    // oceans half as wide again).
    ExtractedWorld extractedWorld;
    bool useExtracted = false;
    // The game's two worlds (docs/multiplayer.md): the client world the player sees, plays and
    // hears, a mirror of whichever host it is joined to, and this machine's host world, which
    // decides every creature that is not a player and keeps Save/world.db.
    SpawnedCreatures spawned;   // the client world (with the native player)
    SpawnedCreatures hostWorld; // the host world
    // Multiplayer (Multiplayer.h), set up below once the character is known.
    Multiplayer session;
    session.log = [](const std::string &line) { logLine(line); };
    const enemyrespawn::Settings enemyRespawns = enemyrespawn::Settings::read(config);
    spawned.setEnemyRespawn(enemyRespawns);
    hostWorld.setEnemyRespawn(enemyRespawns);
    bool spawnedReady = false;
    bool nativePlayer = false;         // [Player] Native: the game's controller and update move the player
    nativecontrol::Input controlInput; // this frame's keys for it
    auto spawnedClock = std::chrono::steady_clock::now();
    // OverworldEvents' travel locations change what the generator makes, so they are set
    // before anything is generated. They go by the game's single-player world, which Game
    // (0x7ea80) names "world" with seed 0x7b, as the extractions are of it.
    const std::string gameWorldName = "world";
    const int32_t gameWorldSeed = 0x7b;
    if (config.flag("Events", "Enabled", false) && config.flag("Events", "ReplaceTravelLocations", true))
        OverworldEvents::replaceTravelLocations(gameWorldSeed, gameWorldName);
    const std::string worldSource = config.text("World", "Source", "Extracted");
    if (worldSource == "Game") {
        worldgen::setWorldScale(config.number("World", "Scale", 1, 1, 4));
        { // the world's day (world+8, Save/world.db): some encounters go by it, and zones are
          // refreshed for it when loaded (0x31c540: shoppers, travellers' camps)
            worldgen::setWorldDay(saves.world().day);
            worldgen::setRefreshDay(saves.world().day);
        }
        // Start on dry land near the world's origin, where the game's starting region is.
        int homeX = 0, homeY = 0;
        for (int ring = 0; ring < 64 && worldgen::groundHeight(homeX, homeY) < 4; ring++)
            for (int i = 0; i < 16 && worldgen::groundHeight(homeX, homeY) < 4; i++) {
                double a = i * 6.283185307179586 / 16;
                homeX = (int)std::lround(std::cos(a) * ring * 64), homeY = (int)std::lround(std::sin(a) * ring * 64);
            }
        extractedWorld.openGenerated(homeX, homeY, assets.gameDir() / "Extract");
        useExtracted = true;
        { // the game's creature tables (tools/ZoneExtractor's oracle) for the spawn points
            std::string why;
            if (worldgen::loadCreatureTables(assets.gameDir() / "Extract" / "oracle", &why))
                logLine("World: creature tables read from the game");
            else
                logLine("World: no creature tables (" + why + "); zones get no spawn points");
        }
        if (worldgen::creatureTablesLoaded()) {
            std::string why;
            spawned.setRoadTravel(config.flag("Creatures", "RoadTravel", false));
            if (spawned.init(assets.gameDir() / "Extract", &why) && hostWorld.init(assets.gameDir() / "Extract", &why)) {
                hostWorld.setRoadTravel(config.flag("Creatures", "RoadTravel", false));
                spawnedReady = true, logLine("World: the game's creatures, posed by its own pose code");
                if (config.flag("Player", "Native", true)) {
                    spawned.setNativePlayer(true), nativePlayer = true;
                    logLine("Player: moved by the game's controller (0x9c050) and creature update");
                }
                if (!spawned.loadSpecials(assets.gameDir() / "Extract" / "oracle" / "items.bin") ||
                    !hostWorld.loadSpecials(assets.gameDir() / "Extract" / "oracle" / "items.bin"))
                    logLine("World: no special-item lists (Extract/oracle/items.bin); those drops have no kind");
            }
            else
                logLine("World: no creatures drawn (" + why + ")");
        }
        logLine("World: the game's generator (reimplemented), scale " +
                std::to_string(worldgen::worldScale()) + ", starting at " + std::to_string(homeX) + ", " +
                std::to_string(homeY));
    } else if (worldSource != "Generated") {
        std::string path = config.text("World", "ExtractedPath");
        fs::path dir = path.empty() ? ExtractedWorld::find(assets.gameDir()) : pathOfUtf8(path);
        std::string why;
        useExtracted = !dir.empty() && extractedWorld.open(dir, why);
        logLine(useExtracted ? "World: extracted from the game, " + std::to_string(extractedWorld.zoneCount()) +
                                   " zones (seed " + std::to_string(extractedWorld.seed()) + ") in " + utf8(dir)
                             : "World: extracted data unavailable (" + (dir.empty() ? std::string("no extracted world") : why) + ")");
        if (!useExtracted) {
            fatal("No usable extracted Cube World data was found. Run the extractor from CubeVulkanSetup first. "
                  "The experimental generated terrain requires an explicit [World] Source=Generated setting.");
            renderer.shutdown();
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
        // Its creatures are the game's own ([Creatures] Native, on by default): made from the
        // zones' spawn points (the generator on the world's seed), with the game's AI,
        // abilities, hits and pose, as in [World] Source=Game. Off: the old recorded actors.
        if (config.flag("Creatures", "Native", true)) {
            worldgen::setWorldSeed(extractedWorld.seed());
            worldgen::setWorldDay(saves.world().day); // (Save/world.db)
            worldgen::setRefreshDay(saves.world().day);
            std::string why;
            if (!worldgen::loadCreatureTables(assets.gameDir() / "Extract" / "oracle", &why))
                logLine("World: no creature tables (" + why + "); the recorded actors stand in");
            else if (spawned.init(assets.gameDir() / "Extract", &why) && hostWorld.init(assets.gameDir() / "Extract", &why)) {
                spawned.setRoadTravel(config.flag("Creatures", "RoadTravel", false));
                hostWorld.setRoadTravel(config.flag("Creatures", "RoadTravel", false));
                spawnedReady = true, logLine("World: the game's creatures, posed by its own pose code");
                if (config.flag("Player", "Native", true)) {
                    spawned.setNativePlayer(true), nativePlayer = true;
                    logLine("Player: moved by the game's controller (0x9c050) and creature update");
                }
                if (!spawned.loadSpecials(assets.gameDir() / "Extract" / "oracle" / "items.bin") ||
                    !hostWorld.loadSpecials(assets.gameDir() / "Extract" / "oracle" / "items.bin"))
                    logLine("World: no special-item lists (Extract/oracle/items.bin); those drops have no kind");
            } else
                logLine("World: no native creatures (" + why + "); the recorded actors stand in");
        }
    }
    World world(terrain, assets, renderer, viewZones, useExtracted ? &extractedWorld : nullptr);
    // The zones' stored states in Save/world.db go on the zones as they are made, and what changed
    // goes back when they leave memory (ZoneStates.h).
    // (The title shows the client world with the stored states; once play starts they are the host
    // world's, below.)
    if (spawnedReady)
        spawned.setStoredZones(extractedWorld, &saves.world(), false);

    DayNightSettings dayNight;
    dayNight.dayStrength = (float)config.number("Shadows", "StrengthPercent", 90, 0, 100) / 100;
    dayNight.nightStrength = (float)config.number("Moon", "ShadowStrengthPercent", 60, 0, 100) / 100;
    dayNight.moonBrightness = (float)config.number("Moon", "BrightnessPercent", 4, 0, 100) / 100;
    float hour = startHour >= 0 ? startHour : (float)config.number("Time", "StartHour", 10, 0, 24);
    float minutesPerSecond = (float)config.number("Time", "MinutesPerSecond", 1, 0, 600);

    // The game's options soundVolume, musicVolume (percent) and musicLoop (place music never
    // fades after its five minutes).
    Audio audio;
    audio.setVolumes(gameOptions.musicVolume, gameOptions.soundVolume);
    audio.setMusicLoop(gameOptions.musicLoop != 0);
    if (logSounds || config.flag("Audio", "LogSounds", false))
        audio.setLog([](const std::string &line) { logLine(line); });
    if (!capture.empty() || !captureSpeech.empty() || !captureFeat.empty() || serverMode || !audio.init(assets))
        logLine("Audio disabled");
    else if (!audio.soundsKnown())
        logLine("Audio: no sound list (Extract/sounds.txt, written by tools/ZoneExtractor in the game); only music plays");
    else
        logLine("Audio: " + std::to_string(audio.soundsKnown()) + " sounds listed");

    // Putting options into effect, at start and whenever the options screen applies them
    // (frontend-options.md §1.2): the window's mode, vsync and the view distance here; the
    // volumes, the frame limit, CPU usage and the camera's speed, smoothness and invert-Y are
    // read where they are used.
    auto applyGameOptions = [&](const gameoptions::Options &o) {
        const WindowMode wanted = windowModeOf(o);
        if (!serverMode && (wanted.fullscreen != windowMode.fullscreen || wanted.width != windowMode.width ||
                            wanted.height != windowMode.height || (wanted.fullscreen && wanted.refresh != windowMode.refresh))) {
            if (wanted.fullscreen)
                glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, wanted.width, wanted.height, wanted.refresh);
            else {
                int x = 0, y = 0;
                if (const GLFWvidmode *desktop = glfwGetPrimaryMonitor() ? glfwGetVideoMode(glfwGetPrimaryMonitor()) : nullptr)
                    x = std::max(0, (desktop->width - wanted.width) / 2), y = std::max(0, (desktop->height - wanted.height) / 2);
                glfwSetWindowMonitor(window, nullptr, x, y, wanted.width, wanted.height, GLFW_DONT_CARE);
            }
            windowMode = wanted;
            logLine("Options: " + std::string(wanted.fullscreen ? "full screen " : "window ") + std::to_string(wanted.width) + " x " +
                    std::to_string(wanted.height));
        }
        if (!setOnCommandLine("graphics.vsync") && benchmark <= 0)
            renderer.settings.vsync = renderSettings.vsync = o.vsync != 0; // (taken up when the swapchain is next made)
        renderer.settings.nativeZoneRadius = renderSettings.nativeZoneRadius = o.zonesDrawn();
        if (!setOnCommandLine("graphics.viewdistance")) {
            renderer.settings.fogDistance = renderSettings.fogDistance = o.viewDistance();
            world.setViewZones(renderSettings.style == RenderStyle::Steam ? o.zonesDrawn() : (int)std::ceil(renderSettings.fogDistance / ZONE));
        }
    };
    const nativeprogress::Frame saveFrame{useExtracted ? extractedWorld.originDotsX() : 0, useExtracted ? extractedWorld.originDotsY() : 0};
    const std::string savedWorldName = useExtracted ? extractedWorld.directory().filename().string() : "generated";
    if (savesWritable && saves.isOpen()) { // CubeVulkan's old text saves move into characters.db once
        std::string bundle;
        steam.cloudRead("cubevulkan.sav", bundle);
        std::set<int64_t> oldKills;
        if (nativeprogress::migrateOldSaves(saves.store(), root / "Saves", bundle, listCharacters(assets.gameDir()), savedWorldName,
                                            saveFrame, [](const std::string &line) { logLine(line); }, &oldKills) > 0) {
            saves.world().dead.insert(oldKills.begin(), oldKills.end());
            nativesaves::writeWorldFile(saves.folder() / "world.db", saves.world());
            characters = everyCharacter();
            chosen = pickCharacter(characters, wantedCharacter);
            if (!chosen)
                chosen = pickCharacter(characters, "");
        }
    }

    // The front screens (Frontend.h): the title over the running world, character select and
    // creation, the options.
    const bool launchMenu = forceMenu || (!directPlay && capture.empty() && captureSpeech.empty() && captureFeat.empty() && benchmark <= 0 &&
        std::isnan(startX) && runSeconds <= 0 && poseAbility <= 0 && !autoAttack && !openInventory && showcase.empty());
    frontend::Services front;
    front.window = window, front.input = &input, front.assets = &assets, front.renderer = &renderer, front.world = &world;
    front.extracted = useExtracted ? &extractedWorld : nullptr;
    front.spawned = spawnedReady && nativePlayer ? &spawned : nullptr;
    front.steam = &steam, front.audio = &audio, front.root = settingsRoot, front.store = &saves.store();
    front.applyOptions = applyGameOptions;
    // The start spot's world (frontend-characters.md §4.2): the generator's cells, places, regions
    // and ground. A world kept as extracted zones has ground only around its home, so there the
    // search starts at the home itself instead of a far-away radius, and takes only extracted cells.
    if (useExtracted) {
        frontend::StartSpotWorld &w = front.startSpot;
        w.areaCell = [](int32_t qx, int32_t qy) {
            const worldgen::Int2 c = worldgen::areaLandmark(qx, qy);
            return nativesaves::Cell{c.x, c.y};
        };
        w.cellObject = [](int32_t cx, int32_t cy) { return worldgen::cellObject(cx, cy); };
        w.placeKind = [](int32_t cx, int32_t cy) { return worldgen::cellPlace(cx, cy).kind; };
        w.biome = [](int32_t x, int32_t y) { return worldgen::biomeAt(x, y); };
        w.ground = [](int32_t x, int32_t y) { return (float)worldgen::groundHeight(x, y); };
        if (!extractedWorld.generated()) {
            w.playable = [&extractedWorld](nativesaves::Cell cell) {
                const std::array<int32_t, 2> b = frontend::startBlock(cell);
                return extractedWorld.has((int)std::floor(b[0] / 64.0) - extractedWorld.originZoneX(),
                                          (int)std::floor(b[1] / 64.0) - extractedWorld.originZoneY());
            };
            const glm::vec3 home = extractedWorld.home();
            front.startOrigin = {(int32_t)std::floor(((double)home.x + (double)(extractedWorld.originDotsX() >> 16)) / 16.0),
                                 (int32_t)std::floor(((double)home.y + (double)(extractedWorld.originDotsY() >> 16)) / 16.0)};
            front.farStart = false;
        }
    }
    front.sprites = &sprites, front.options = &gameOptions, front.controls = &gameControls;
    front.dayNight = dayNight, front.minutesPerSecond = minutesPerSecond;
    front.startHour = saves.world().timeMs / 3600000.0f; // (Save/world.db, or a new world's 10:00)
    front.log = [](const std::string &line) { logLine(line); };
    std::vector<CharacterEntry> recordedOnly = listCharacters(assets.gameDir());
    front.recorded = &recordedOnly;
    std::optional<glm::vec3> enteredAt; // where the front screens put the player (frontend-title.md §6.1)
    if (launchMenu) {
        frontend::Request request;
        request.page = menuPage;
        request.capture = capture;
        request.captureAfter = runSeconds;
        request.pointer = {pointerTestX, pointerTestY};
        request.clickAfter = clickAfter;
        request.readOnly = !savesWritable;
        logLine("Menu: the title over the world, character select and creation, options");
        const frontend::Result result = frontend::run(front, request);
        if (!result.error.empty()) logLine("Menu: " + result.error);
        if (result.action != frontend::Result::Play || !result.error.empty()) {
            renderer.shutdown();
            glfwDestroyWindow(window);
            glfwTerminate();
            return result.error.empty() ? 0 : 1;
        }
        if (result.placed)
            enteredAt = result.feet;
        characters = everyCharacter(); // (creation may have made one)
        chosen = nullptr;
        for (const CharacterEntry &c : characters)
            if (c.native && c.slot == result.slot)
                chosen = &c;
        renderSettings = renderer.settings;
    }

    // The character's progress (Save/characters.db, the world's in Save/world.db).
    SaveGame saveGame;
    const bool saving = chosen && saves.isOpen() && savesWritable && std::isnan(startX) && !serverMode;
    // A capture with --save-folder loads that folder's character too (read only: it writes
    // nothing back), so a test save's progress shows in the screenshot.
    const bool loading = saving || (chosen && saves.isOpen() && !saveFolder.empty() && std::isnan(startX) && !serverMode);
    if (loading) {
        saves.play(*chosen);
        const bool had = saves.load(saveGame, saveFrame, savedWorldName, [&](const nativesaves::Item &it) {
            return spawnedReady ? spawned.itemSprite(it.data()) : std::string();
        });
        logLine(std::string("Save: ") + (had ? "loaded character " + std::to_string(saves.slot()) : "new character") + " of " +
                utf8(saves.folder() / "characters.db"));
    }
    Player player;
    if (!std::isnan(startX)) {
        player.position.x = startX;
        player.position.y = startY;
    }
    if (!std::isnan(startYaw))
        player.yaw = startYaw;
    if (!std::isnan(startPitch))
        player.pitch = startPitch;
    if (!std::isnan(startZoom))
        player.zoom = startZoom;
    auto groundAt = [&](float x, float y) {
        if (useExtracted) {
            float h;
            return extractedWorld.surface(x, y, h) ? h : player.position.z;
        }
        auto c = terrain.column(x, y);
        return std::max((float)c.ground, c.water ? (float)c.water - .4f : 0.f);
    };
    // Region at the player, for sky, light and music.
    auto columnAt = [&](float x, float y) {
        if (!useExtracted)
            return terrain.column(x, y);
        ColumnInfo c{};
        c.biome = (uint8_t)extractedWorld.biomeAt(x, y);
        c.weights[c.biome] = 1;
        c.ground = (int)groundAt(x, y);
        c.water = c.ground <= ExtractedWorld::SEA ? ExtractedWorld::SEA : 0;
        c.freshwater = c.biome != OCEAN;
        return c;
    };
    if (useExtracted && std::isnan(startX)) {
        player.position = extractedWorld.home(); // Where the extraction started (the save's position).
    } else if (std::isnan(startX)) {
        // Spawn on dry, gentle grassland near the origin, like Cube World's starts.
        for (int ring = 0; ring < 160; ring++) {
            bool found = false;
            for (int i = 0; i < 8 * std::max(ring, 1) && !found; i++) {
                float angle = i * 6.2831853f / (8 * std::max(ring, 1)), r = ring * 16.f;
                float x = 32 + std::cos(angle) * r, y = 32 + std::sin(angle) * r;
                auto c = terrain.column(x, y);
                int slope = std::abs(terrain.column(x + 3, y).ground - c.ground) +
                            std::abs(terrain.column(x, y + 3).ground - c.ground);
                if (c.biome == PLAINS && c.weights[PLAINS] > .9f && !c.water &&
                    c.ground > terrainSettings.seaLevel + 3 && slope <= 2) {
                    player.position.x = x;
                    player.position.y = y;
                    found = true;
                }
            }
            if (found)
                break;
        }
    }
    // Extracted world: the walkable floor under a point (not the topmost block,
    // so roofs over the shrine and houses are not mistaken for the ground).
    constexpr float STEP = 1.1f;        // Climbable step height.
    constexpr int HEADROOM = 2;         // Open blocks needed above a floor.
    // The player's body: the game's square footprint (Body.h), sized from the
    // recorded character once it is loaded below.
    Body body;
    auto floorUnder = [&](float x, float y, float z) {
        return body.floorUnder([&](float px, float py, float pz, float &h) { return extractedWorld.floorAt(px, py, pz, HEADROOM, h); },
                               x, y, z, z);
    };
    // The player's body (above step height) would be inside a block there.
    auto blocked = [&](float x, float y, float z) {
        return body.blocked([&](float px, float py, float pz) { return extractedWorld.solidAt(px, py, pz); }, x, y, z);
    };
    if (useExtracted && !std::isnan(startZ))
        player.position.z = startZ;
    else if (useExtracted && !std::isnan(startX))
        player.position.z = groundAt(player.position.x, player.position.y); // No height given: top surface.
    if (useExtracted)
        player.position.z = floorUnder(player.position.x, player.position.y, player.position.z + STEP);
    else
        player.position.z = groundAt(player.position.x, player.position.y);
    // Back where the character was last saved, if it was in this world.
    const std::string worldName = useExtracted ? extractedWorld.directory().filename().string() : "generated";
    const bool resumed = (saveGame.hasPosition && saveGame.world == worldName) || enteredAt;
    if (resumed) {
        // Exactly where the save has the character, facing yaw 0; no ground search.
        player.position = enteredAt ? *enteredAt : saveGame.position;
        player.yaw = saveGame.yaw;
        logLine("Save: resuming at " + std::to_string(player.position.x) + ", " + std::to_string(player.position.y));
    }
    // The world's clock carries on from the save, as the game keeps day and time in
    // its world state (unless --hour was given).
    int day = std::max(0, saveGame.day);
    if (saveGame.hour >= 0 && startHour < 0)
        hour = std::fmod(saveGame.hour, 24.f);

    ModHost mods;
    ModContext modContext;
    modContext.world = &world;
    modContext.config = &config;
    modContext.hour = &hour;
    modContext.focus = &player.position;
    modContext.log = logLine;
    mods.load(root / "mods", modContext);
    if (!showcase.empty()) {
        float x = player.position.x - 12, y = player.position.y - 14;
        size_t start = 0;
        while (start < showcase.size()) {
            size_t end = showcase.find(',', start);
            std::string item = showcase.substr(start, end == std::string::npos ? std::string::npos : end - start);
            start = end == std::string::npos ? showcase.size() : end + 1;
            size_t colon = item.find(':');
            float scale = colon == std::string::npos ? 1.f / 16 : 1.f / std::strtof(item.c_str() + colon + 1, nullptr);
            int model = world.loadModel(item.substr(0, colon));
            if (model >= 0)
                world.spawn(model, {x, y, groundAt(x, y)}, 0, scale);
            else
                logLine("Showcase: no sprite " + item);
            x += 6;
        }
    }

    // The player character, posed by the game's own recorded part matrices.
    CharacterRecording character;
    std::vector<int> characterModels;
    {
        std::string why;
        if (!chosen)
            why = "no recorded character in Extract/characters (press F5 in game with ZoneExtractor)";
        else if (character.load(chosen->dir, why)) {
            body = Body(character.hitboxScale);
            if (chosen->created) { // made in CubeVulkan: its own look and name on its class's recording
                character.restyle(chosen->head, chosen->hair, chosen->hand, chosen->foot, chosen->hairColor);
                character.name = chosen->name;
            }
            // Built-in DualHair: [Hair] SecondColor=R,G,B[,START,BLEND] in CubeVulkan.ini, or
            // the character's entry in an imported Mods/DualHair.ini ([Characters] Name=...).
            World::HairLook look;
            std::string spec = config.text("Hair", "SecondColor");
            if (spec.empty() && !character.name.empty())
                spec = config.text("Characters", character.name);
            look.start = (float)config.number("DualHair", "StartPercent", 40, 0, 100) / 100;
            look.blend = (float)config.number("DualHair", "BlendPercent", 35, 0, 100) / 100;
            if (config.flag("DualHair", "Enabled", true) && !spec.empty()) {
                int r, g, b, start = -1, blend = -1;
                unsigned hex;
                if (std::sscanf(spec.c_str(), "%d , %d , %d , %d , %d", &r, &g, &b, &start, &blend) >= 3) {
                    look.dual = true;
                    look.second = glm::u8vec3(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
                } else if (std::sscanf(spec.c_str(), " #%6x , %d , %d", &hex, &start, &blend) >= 1) {
                    look.dual = true;
                    look.second = glm::u8vec3(hex >> 16, (hex >> 8) & 255, hex & 255);
                }
                if (start >= 0 && blend >= 0) {
                    look.start = std::min(start, 100) / 100.f;
                    look.blend = std::min(blend, 100) / 100.f;
                }
            }
            if (look.dual)
                logLine("Hair: second colour " + spec);
            for (auto &p : character.parts())
                characterModels.push_back(world.loadModel(p.sprite, p.tint, p.hair, look.dual ? &look : nullptr));
            logLine("Character: " + character.summary());
            for (auto &[id, seconds] : character.abilities())
                logLine("  ability " + std::to_string(id) + ": " + std::to_string(seconds) + " s, recorded " +
                        std::to_string(character.abilityStart(id)) + " s into its clip");
        } else
            logLine("Character: none (" + why + ")");
    }
    // Creatures and NPCs recorded from the game (positions and poses replayed).
    CreatureRecordings creatures;
    std::vector<std::vector<int>> creatureModels;
    if (useExtracted) {
        size_t n = creatures.load(extractedWorld.directory() / "creatures",
                                  [&](const int64_t *dots) { return extractedWorld.toLocal(dots); });
        for (auto &c : creatures.all()) {
            creatureModels.emplace_back();
            for (auto &p : c.parts)
                creatureModels.back().push_back(world.loadModel(p.sprite, p.tint, p.hair));
        }
        auto &stats = creatures.stats();
        logLine("Creatures: " + std::to_string(n) + " replayable recordings from " + std::to_string(stats.listed) +
                " captures (missing appearance " + std::to_string(stats.missingAppearance) +
                ", frames " + std::to_string(stats.missingFrames) + ", pose " + std::to_string(stats.missingPose) + ")");
    }
    // The recordings brought to life: idle, wander and react to the player.
    Actors actors;
    // With the game's own creatures (people included, who say the game's own scripts),
    // the recordings stand in for none of them.
    actors.build(creatures, spawnedReady);
    actors.setEnemyRespawn(enemyRespawns);
    // Other modes recover old enemy deaths as their spawn zones load, while
    // preserving unrelated world state. Permanent mode retains native death history.
    {
        std::set<int64_t> dead(saves.world().dead.begin(), saves.world().dead.end()), kills;
        const size_t fromWorld = dead.size();
        if (saves.world().day > day)
            day = saves.world().day; // (the world's day never runs behind world.db's)
        if (saveGame.world == worldName)
            kills.insert(saveGame.killed.begin(), saveGame.killed.end());
        dead.insert(kills.begin(), kills.end());
        int marked = 0;
        for (size_t k = 0; k < actors.count(); k++)
            if (enemyRespawns.mode == enemyrespawn::Mode::Permanent && dead.count(actors.creatureId(k)))
                actors.alreadyDead(k), marked++;
        saveGame.killed.assign(kills.begin(), kills.end());
        spawned.setWorldDead(dead);
        hostWorld.setWorldDead(dead);
        logLine("Dead creatures: " + std::to_string(fromWorld) + " in Save/world.db, " + std::to_string(kills.size()) +
                " known to this character; " + std::to_string(marked) + " of the recorded creatures stay dead");
        const char *deathMode = enemyRespawns.mode == enemyrespawn::Mode::Timed ? "Timed" :
                                enemyRespawns.mode == enemyrespawn::Mode::Instance ? "Instance" : "Permanent";
        logLine(std::string("Enemy deaths: ") + deathMode + "; respawn delay " + std::to_string(enemyRespawns.seconds) + " seconds");
    }
    if (actors.count())
        logLine("Creatures: " + actors.summary());
    std::vector<glm::mat4> actorPose;
    // The game's interface (bars, plates over monsters, damage numbers).
    Hud hud;
    bool hudReady = false;
    {
        std::string why;
        hudReady = hud.load(assets.gameDir(), why);
        if (hudReady)
            hud.setGuiScale(guiScaleTest > 0 ? guiScaleTest : gameOptions.guiScale);
        if (hudReady)
            hud.layout(renderer, renderer.screenSize());
        logLine(hudReady ? "Interface: the game's HUD art and fonts" : "Interface: none (" + why + ")");
    }
    VkExtent2D hudSize = renderer.screenSize();
    // Monster names from the game's dictionary, via their head or body sprite.
    std::vector<std::string> creatureNames;
    for (auto &c : creatures.all()) {
        std::string name;
        for (auto &p : c.parts)
            if (name.empty())
                name = hud.nameForSprite(p.sprite);
        creatureNames.push_back(name);
    }
    const bool showLevelsSetting = config.flag("Interface", "ShowLevels", false); // Steam shows none; PyroProgression adds them.
    std::vector<float> trails(actors.count(), 1.f);
    struct Floating {
        glm::vec3 at;
        int value;
        double age;
        bool taken;
    };
    std::vector<Floating> floating;
    std::vector<OverlayVertex> hudVertices;
    // The player's combat numbers, from the game (snapshot "stats"), or a level-1
    // rogue's if the snapshot predates stat recording.
    float playerMaxHp = character.maxHp > 0 ? character.maxHp : 145.6f;
    float playerAttack = character.attack > 0 ? character.attack : 10.f;
    float playerArmor = character.armor > 0 ? character.armor : .82f;
    float playerCritical = character.critical > 0 ? character.critical : .02f;
    // Spell power and resistance as recorded, relative to attack and armor (gear only
    // measures those two), and the combo: landed hits, reset 4 s after the last.
    const float spellPerAttack = character.attack > 0 && character.spell > 0 ? character.spell / character.attack : 1;
    const float resistancePerArmor = character.armor > 0 && character.resistance > 0 ? character.resistance / character.armor : 1;
    int playerCombo = 0;
    double sinceLanded = 99;
    // Effects (skills, War Frenzy), the charge of the attack under way, and stun.
    combat::Effects playerEffects;
    float playerCharge = 0;
    double playerStunned = 0, sinceStunned = 99;
    auto playerDefense = [&]() { return combat::Defender{playerArmor, playerArmor * resistancePerArmor, &playerEffects}; };
    float playerHp = playerMaxHp;
    // The game's items: class rules, models seen on creatures, weapon grips.
    ItemCatalog catalog;
    {
        std::string why;
        // The class rules are the game's, the same for every character.
        fs::path rules = assets.gameDir() / "Extract" / "class-rules.txt";
        if (!fs::exists(rules))
            rules = assets.gameDir() / "Extract" / "character" / "class-rules.txt";
        if (!catalog.loadClassRules(rules, why))
            logLine("Items: no class rules (" + why + ")");
        if (useExtracted)
            catalog.loadModels(extractedWorld.directory() / "creatures" / "creatures.txt");
        catalog.learnGrips(creatures);
        catalog.addInferredRangerArmor();
        logLine("Items: " + catalog.summary());
        for (auto &[key, m] : catalog.models()) {
            auto [c, i, mat] = key;
            logLine("  item category " + std::to_string(c) + " id " + std::to_string(i) + " material " +
                    std::to_string(mat) + ": " + m.material + " " + m.type + " (" + *m.sprites.begin() + ")");
        }
    }
    // Gear as the game measured it: stats follow what is equipped.
    Gear gear;
    bool haveGear = false;
    {
        std::string why;
        haveGear = chosen && gear.load(chosen->dir, why);
        if (!chosen)
            why = "no recorded character";
        if (haveGear) {
            for (int s = 0; s < Gear::SLOTS; s++)
                if (gear.equipped((Gear::Slot)s).measured)
                    logLine("Gear: " + gear.describe(gear.equipped((Gear::Slot)s)));
        } else
            logLine("Gear: none (" + why + ")");
    }
    // Character progression (Progression.h), switchable: [Progression] Mode=pyro|vanilla
    // in CubeVulkan.ini or the Options menu. Without a setting it follows the
    // owner's game: pyro when the PyroProgression mod is installed there.
    bool pyroInstalled = false;
    {
        std::error_code ec;
        for (auto &f : fs::directory_iterator(assets.gameDir() / "Mods", ec))
            if (f.path().filename().string().rfind("PyroProgression", 0) == 0 && f.path().extension() == ".dll")
                pyroInstalled = true;
    }
    auto readMode = [&] {
        std::string m = config.text("Progression", "Mode");
        for (auto &c : m)
            c = (char)std::tolower((unsigned char)c);
        if (m == "pyro" || m == "vanilla")
            return m == "pyro" ? progression::Mode::Pyro : progression::Mode::Vanilla;
        return pyroInstalled ? progression::Mode::Pyro : progression::Mode::Vanilla;
    };
    progression::Mode mode = readMode();
    const int recordedLevel = std::max(1, character.level);
    int playerLevel = saveGame.level > 0 ? saveGame.level : recordedLevel;
    auto setSpecializationMetadata = [&](int specialization) {
        character.specialization = specialization;
        saveGame.specialization = specialization;
        for (auto &entry : characters)
            if (&entry == chosen) { entry.specialization = specialization; break; }
    };
    if (saveGame.specialization >= 0) setSpecializationMetadata(saveGame.specialization);
    int playerXp = saveGame.xp, legacyGold = saveGame.gold;
    auto *nativeInventory = saveGame.nativeInventory ? &*saveGame.nativeInventory : nullptr;
    saveGame.inventoryAuthoritative = nativeInventory != nullptr;
    int &playerGold = nativeInventory ? nativeInventory->inventory.gold : legacyGold;
    if (nativeInventory)
        logLine("Inventory: canonical " + std::to_string(nativeInventory->inventory.tabs.size()) + " tabs loaded; saves retain live slots");
    logLine(std::string("Progression: ") + progression::modeName(mode) + (pyroInstalled ? " (PyroProgression is installed in the game)" : "") +
            ", level " + std::to_string(playerLevel));
    // Pyro mode on the game's own creatures (PyroCreatures.h): their numeric levels, and their
    // stats by them through the mod loader's callbacks, as PyroProgression hooks them. The
    // regional baseline is set by the first occupant, shared by the host, and cleared
    // when the last player leaves. Permanent discovery difficulty locks are not used.
    pyrocreatures::RelativeLevels pyroRegions;
    {
        const fs::path gameState = assets.gameDir() / "Mods" / "PyroRelativeLevels.state";
        // (A test's --save-folder keeps its own copy; a dedicated server has no character's to keep.)
        const fs::path ownState = (saveFolder.empty() ? root / "Saves" : pathOfUtf8(saveFolder)) / "PyroRelativeLevels.state";
        pyroRegions.open(gameWorldSeed, gameWorldName, character.name, false, gameState, ownState, !noSave && !serverMode);
        if (mode == progression::Mode::Pyro)
            logLine("Progression: region difficulty refreshes after its last player leaves");
    }
    const bool firstEntrantScaling = config.text("Progression", "RegionScaling", "first-player") != "relative";
    auto regionBasis = [&](worldgen::Int2 region) { return session.regionLevel(region, playerLevel); };
    auto creatureLevel = [&](const uint8_t *creature) { return pyrocreatures::level(creature, gameWorldSeed, regionBasis); };
    std::optional<pyrocreatures::StatHooks> pyroStats;
    auto applyPyroStats = [&] {
        session.setRegionScaling(mode == progression::Mode::Pyro && firstEntrantScaling);
        if (mode == progression::Mode::Pyro && !pyroStats)
            pyroStats.emplace(gameWorldSeed, regionBasis);
        else if (mode != progression::Mode::Pyro)
            pyroStats.reset();
    };
    applyPyroStats();
    // An item's level for pyro progression: gear's kept level (RelativeItemLevel); anything
    // else the mod's old level by its modifier.
    auto itemLevel = [&](const Gear::Item &it) {
        if (mode == progression::Mode::Pyro && it.category >= 3 && it.category <= 9)
            return pyroRegions.itemLevel(it.category, it.id, it.modifier, it.material, playerLevel);
        return it.level > 0 ? it.level : progression::ownedItemLevel(0, it.modifier);
    };
    // A dropped item's level (the mod's hook on the game's drop, 0x2a5b70): gear gets one the
    // first time it is seen; 1 in vanilla.
    auto dropLevel = [&](const Gear::Item &it, int enemyLevel) {
        if (mode != progression::Mode::Pyro)
            return 1;
        if (it.category >= 3 && it.category <= 9)
            return pyroRegions.dropped(it.category, it.id, it.modifier, it.material, playerLevel, enemyLevel);
        return progression::lootLevel(playerLevel, enemyLevel, it.modifier);
    };
    // Pyro forbids gear above the player's level.
    auto levelAllows = [&](const Gear::Item &it) { return mode != progression::Mode::Pyro || itemLevel(it) <= playerLevel; };
    // Stats for an item found in CubeVulkan: the surveyed item for the same slot,
    // scaled by the mode's gear strength (level and rarity, or rarity alone).
    // The region the player stands in (0x2ada20 of its block), for the game's rules: an
    // item counts fully only in its own region. {0, 0} without a game world.
    worldgen::Int2 playerRegion{0, 0};
    auto regionHere = [&]() -> worldgen::Int2 {
        if (!useExtracted)
            return {0, 0};
        const int64_t x = (int64_t)std::llround((double)player.position.x * 65536) + extractedWorld.originDotsX();
        const int64_t y = (int64_t)std::llround((double)player.position.y * 65536) + extractedWorld.originDotsY();
        return worldgen::regionAt((int32_t)(x >> 16), (int32_t)(y >> 16));
    };
    // The player's numbers by the game's own rules (NativePlayer), with `slot` holding
    // `in` (none: as worn).
    auto nativeRecord = [&](int slot, const Gear::Item *in) {
        nativeplayer::Who who;
        who.race = chosen && chosen->native ? chosen->race : character.race, who.cls = gear.playerClass();
        who.spec = chosen && chosen->native ? chosen->specialization : character.specialization;
        who.level = std::max(1, character.level), who.flags2 = character.flags2;
        playerRegion = regionHere();
        who.region[0] = playerRegion.x, who.region[1] = playerRegion.y;
        std::array<nativeplayer::Item, 11> worn{};
        for (int k = 0; k < (int)Gear::SLOTS; k++) {
            const Gear::Item &it = k == slot && in ? *in : gear.equipped((Gear::Slot)k);
            if (nativeInventory && k != slot)
                worn[(size_t)k] = nativeInventory->equipment[(size_t)k];
            else if (it.category && !(k == slot && !in))
                worn[(size_t)k] = Gear::toRecord(it);
        }
        worldgen::CreatureRecord r = nativeplayer::record(who, worn);
        if (chosen && chosen->hasAppearance) // a character of characters.db: its own looks
            std::memcpy(&r[worldgen::creature::APPEARANCE], chosen->appearance.data(), chosen->appearance.size());
        else if (chosen)
            nativeplayer::appearance(r, chosen->dir / "snapshot.txt");
        return r;
    };
    auto nativeStats = [&](int slot, const Gear::Item *in) { return nativeplayer::stats(nativeRecord(slot, in)); };
    auto foundStats = [&](Gear::Item &it, Gear::Slot slot) {
        if (mode != progression::Mode::Pyro) { // what it adds where the player stands (the game's rules)
            Gear::Item empty;
            const nativeplayer::Stats with = nativeStats(slot, &it), without = nativeStats(slot, &empty);
            it.adds = {with.maxHp - without.maxHp, with.attack - without.attack, with.armor - without.armor,
                       with.critical - without.critical};
            it.measured = it.found = true;
            return true;
        }
        const Gear::Item *ref = gear.reference(slot, it.category);
        if (!ref)
            return false;
        float factor = progression::gearStrength(mode, itemLevel(it), it.rarity, it.modifier) /
                       std::max(.001f, progression::gearStrength(mode, itemLevel(*ref), ref->rarity, ref->modifier));
        it.adds = {ref->adds.maxHp * factor, ref->adds.attack * factor, ref->adds.armor * factor, ref->adds.critical * factor};
        it.measured = it.found = true;
        return true;
    };
    if (!nativeInventory)
        for (auto &f : saveGame.found) {
            Gear::Item it = f.storedRecord ? Gear::fromRecord(*f.storedRecord) : Gear::Item{};
            it.category = f.category, it.id = f.id, it.material = f.material, it.rarity = f.rarity;
            it.modifier = f.modifier, it.level = f.level, it.sprite = f.sprite == "-" ? "" : f.sprite;
            it.region[0] = f.region[0], it.region[1] = f.region[1];
            it.formula = f.formula;
            it.cubes = f.cubes, it.cubeCount = f.cubeCount;
            if (haveGear && f.slot >= 0 && f.slot < Gear::SLOTS && foundStats(it, (Gear::Slot)f.slot))
                gear.add(it, (Gear::Slot)f.slot);
        }
    auto applyGear = [&](bool inventoryAction = false) {
        if (!haveGear)
            return;
        Gear::Stats s = gear.stats();
        if (mode == progression::Mode::Pyro) { // Each level past the recorded one adds the mod's bonus.
            auto bonus = progression::levelBonus(playerLevel - recordedLevel);
            s = s + Gear::Stats{bonus.hp, bonus.attack, bonus.armor, bonus.critical};
        } else { // the game's own rules on the player as it stands (NativePlayer)
            const nativeplayer::Stats n = nativeStats(-1, nullptr);
            s = Gear::Stats{n.maxHp, n.attack, n.armor, n.critical};
            logLine("Gear (the game's rules, region " + std::to_string(playerRegion.x) + " " + std::to_string(playerRegion.y) +
                    "): max health " + std::to_string(n.maxHp) + ", attack " + std::to_string(n.attack) + ", armor " +
                    std::to_string(n.armor) + ", resistance " + std::to_string(n.resistance) + ", critical " +
                    std::to_string(n.critical));
        }
        if (nativePlayer) // the native player's own record wears it too
            spawned.setPlayerGear(nativeRecord(-1, nullptr), !inventoryAction);
        float fraction = playerMaxHp > 0 ? playerHp / playerMaxHp : 1;
        playerMaxHp = s.maxHp;
        playerAttack = s.attack;
        playerArmor = s.armor;
        playerCritical = s.critical;
        if (!inventoryAction)
            playerHp = fraction * playerMaxHp;
        // The sprites the game draws for what is equipped (recorded matrix per part).
        for (size_t i = 0; i < character.parts().size() && i < characterModels.size(); i++) {
            int m = character.parts()[i].matrix;
            Gear::Slot slot = m == 2 ? Gear::Chest : m == 7 ? Gear::WeaponRight : m == 8 ? Gear::WeaponLeft : Gear::SLOTS;
            if (slot == Gear::SLOTS)
                continue;
            if (!gear.sprite(slot).empty())
                characterModels[i] = world.loadModel(gear.sprite(slot));
            else if (!gear.wornMeasured(slot) && gear.equipped(slot).category == 0 && slot != Gear::Chest)
                characterModels[i] = -1; // Taken off: nothing in that hand.
        }
    };
    // Inventory screen (B, as the game): the game's art, item icons drawn from the
    // items' own sprites, names from the game's dictionary, the game's class rules.
    bool inventoryOpen = openInventory;
    bool inventoryRegionActive = false;
    // The crafting widget (C, the game's the crafting button) beside the bag, or alone, and the
    // bag itself (B): the screen is open while either is.
    bool craftingOpen = false, bagShown = true;
    Hud::Inventory inventoryView;
    // The world map (M, the game's the map button): open, or the minimap in the corner. Its
    // position follows the player until the map is dragged (Game's the zoom change); the
    // overlay moves and grows between the corner and the middle (the overlay position, Scale).
    worldmap::Map worldMap;
    mapmarks::Selection mapSelection;
    bool &mapOpen = mapSelection.open;
    bool mapMoved = false;
    int mapZoom = 0;
    glm::dvec3 mapPosition(0), targetMapPosition(0);
    bool mapPlaced = false;
    // The map's cursor (WorldMap's the cursor position): where the pointer meets the open map's
    // models, in game blocks; unknown when it meets none or the map is closed. (Found as the
    // map is drawn: a frame late for what uses it, as in the game.)
    glm::dvec3 mapCursor(0);
    bool mapCursorKnown = false;
    // Flight (WorldMap's the flight master mode, the selected flight point): the map opened by a flight
    // master shows the flight points, the one under the pointer is selected, and a click buys
    // it (the flight price, 0x2adce0) or, once known, takes off on the eagle.
    bool flightmasterMode = false, flightSelected = false;
    // the distance text (and PreviewWidget's the distance format, the same): metres to two
    // figures below 1000, kilometres with a tenth below 10 km, else whole kilometres.
    auto flatDistance = [](int metres) {
        if (metres < 10)
            return std::to_string(metres) + " m";
        if (metres < 500)
            return std::to_string(metres / 10 * 10) + " m";
        if (metres < 1000)
            return std::to_string(metres / 100 * 100) + " m";
        const int km = metres / 1000, tenths = metres / 100 - km * 10;
        return metres < 10000 && tenths > 0 ? std::to_string(km) + "." + std::to_string(tenths) + " km" : std::to_string(km) + " km";
    };
    std::array<int32_t, 2> selectedFlightPoint{0, 0};
    std::set<std::array<int32_t, 2>> boughtFlights(saveGame.flightPoints.begin(), saveGame.flightPoints.end());
    auto flightPrice = [&](std::array<int32_t, 2> point) { // 0x2adce0
        const double px = (double)player.position.x + (double)(extractedWorld.originDotsX() >> 16) - (double)point[0];
        const double py = (double)player.position.y + (double)(extractedWorld.originDotsY() >> 16) - (double)point[1];
        double best = std::sqrt(px * px + py * py);
        for (const auto &known : boughtFlights) { // half a leg from a known one
            const double dx = (double)(known[0] - point[0]), dy = (double)(known[1] - point[1]);
            const double leg = std::sqrt(dx * dx + dy * dy);
            if (best < 0.0 || leg * 0.5 < best)
                best = leg * 0.5;
        }
        return std::max(100, (int)std::max(0.0, best) / 100);
    };
    // The game's ground height at a block (the ground height), for the map's position.
    auto groundOf = [](double x, double y) { return (double)worldgen::groundHeight((int32_t)std::floor(x), (int32_t)std::floor(y)); };
    const float &minimapScale = gameOptions.mapScale; // options.cfg "mapScale" (the game's default 1; F5 / F6)
    glm::vec2 mapViewCentre(1.0f, 1.0f);
    const int &showRarity = gameOptions.showRarity; // options.cfg "showRarity" (the game's default: on)
    std::vector<Hud::State::MapMarker> mapMarkerViews; // the markers drawn over the map (a frame late)
    std::optional<lore::Cell> hoveredLoreCell;         // the lore building under the pointer: its lore panel
    worldgen::Item hoveredArtifact;                 // the artifact of the place under the pointer
    if (hudReady && !hud.loadQuestIcons(root / "assets" / "OverworldEvents" / "quest-icons.png"))
        logLine("Quest sprites: assets/OverworldEvents/quest-icons.png not found");
    bool mapPlayerSet = false;
    int mapPlayerLooks = -1; // the player's marker models on the map (WorldMap::looks)
    float mapLabelScale = minimapScale;
    if (useExtracted)
        worldgen::setWorldSeed(extractedWorld.seed());
    // The shopkeeper being traded with (controller +0x1e98, +0x1ea0), what the player bought from
    // each shop (player data +0x138, saved) and the last ten items sold (the buy-back list).
    struct VendorTrade {
        bool open = false;
        gameshop::Shopkeeper shop;
    } vendorTrade;
    // The identify window (talking to a class-0x83 townsperson), the weapon upgrade widget
    // (right-clicking a worn weapon in the inventory) and the chat's input line (Enter), drawn
    // through the interface's screens (lootWindowItems, a frame's items).
    identifywindow::Window identify;
    identifywindow::Shown identificationView;
    upgradewidget::Widget upgrade;
    upgradewidget::View upgradeView;
    CubModel upgradeModel;                       // the hand's weapon model
    std::string upgradeModelSprite, upgradePictureKey;
    ChatLine chat;
    feattext::Store featTexts;
    size_t pendingFeatDisplays = 0;
    chatInput.clear(); // discard frontend events before gameplay starts
    captureChatInput = true;
    for (const auto &text : chatTestLines)
        chat.receive(std::u16string(character.name.begin(), character.name.end()), std::u16string(text.begin(), text.end()));
    Hud::Screen lootWindowItems;
    // A shopkeeper as its record has it: its id, class and home.
    auto shopkeeperOf = [&](int64_t id) {
        gameshop::Shopkeeper k;
        k.id = id;
        if (const uint8_t *c = spawned.record(id)) {
            k.cls = c[worldgen::creature::CLASS];
            std::memcpy(k.home, c + worldgen::creature::HOME, sizeof k.home);
        }
        return k;
    };
    std::map<int64_t, std::vector<gameshop::Item>> &boughtFromVendor = saveGame.purchases;
    gameshop::BuyBack soldItems;
    std::vector<size_t> listed; // gear item index of each list row
    // Startup adds missing defaults to the saved list; regional gear stays transient.
    // Preserve every original raw record, including recipes absent from the crafting tabs.
    nativesaves::Character recipeSource;
    recipeSource.classType = (uint8_t)gear.playerClass();
    if (saveGame.recipes) recipeSource.recipes = *saveGame.recipes;
    frontend::addDefaultRecipes(recipeSource, 0);
    saveGame.recipes = std::move(recipeSource.recipes);
    std::vector<gamecraft::Thing> knownFormulas;
    for (const auto &record : *saveGame.recipes) knownFormulas.push_back(gamecraft::fromRecord(record));
    std::vector<gamecraft::Formula> craftRows; // the formula of each crafting row
    gamecraft::Formula craftingFormula;
    gamecraft::CraftButton craftButton;
    auto spriteForPartMatrix = [&](int matrix) {
        for (auto &p : character.parts())
            if (p.matrix == matrix)
                return p.sprite;
        return std::string();
    };
    if (hudReady && hud.hasInventoryArt()) {
        std::set<std::string> sprites{"cube"};
        for (size_t i = 0; i < gear.itemCount(); i++)
            if (!gear.item(i).sprite.empty())
                sprites.insert(gear.item(i).sprite);
        for (int m : {2, 7, 8})
            if (!spriteForPartMatrix(m).empty())
                sprites.insert(spriteForPartMatrix(m));
        for (auto &name : sprites)
            if (auto path = assets.resolve(fs::path("sprites") / (name + ".cub"))) {
                CubModel cub;
                if (cub.load(*path))
                    hud.addItemIcon(name, cub);
            }
        hud.layout(renderer, renderer.screenSize());
    }
    if (hudReady) // The money counter's coin: the game's own gold coin sprite.
        if (auto path = assets.resolve(fs::path("sprites") / "gold-coin.cub")) {
            CubModel cub;
            if (cub.load(*path)) {
                hud.addItemIcon("gold-coin", cub);
                hud.layout(renderer, renderer.screenSize());
            }
        }
    // "fist" -> "FIST", with the material in front ("IRON FIST"), as the game lists items.
    // The game's dictionary and its generated names (loaded with the world below), for the
    // game's item names (the item naming).
    Speech speech;
    GameNames itemNameParts;
    auto itemName = [&](const Gear::Item &it) {
        if (speech.count() && itemNameParts.loaded()) {
            const nativeplayer::Item raw = nativeplayer::item(it.category, it.id, it.material, it.rarity, it.modifier, it.region[0], it.region[1]);
            const std::string named = itemnames::name(speech, itemNameParts, raw.data());
            if (!named.empty() && named != "unknown")
                return named;
        }
        const ItemCatalog::Model *m = catalog.model(it.category, it.id, it.material);
        std::string type = m ? m->type : catalog.typeOfSprite(it.sprite), material = m ? m->material : "";
        if (material.empty() && it.sprite.find('-') != std::string::npos)
            material = it.sprite.substr(0, it.sprite.find('-'));
        static const std::map<std::string, std::string> armor = {
            {"hand", "gloves"}, {"foot", "boots"}, {"shoulder", "shoulder armor"}, {"chest", "chest armor"}};
        if (auto a = armor.find(type); a != armor.end())
            type = a->second;
        std::string name = material.empty() ? type : material + " " + type;
        for (auto &c : name)
            c = (char)std::toupper((unsigned char)c);
        return name;
    };
    auto equipSlotOf = [](int screenSlot) {
        static const Gear::Slot slots[10] = {Gear::WeaponLeft, Gear::WeaponRight, Gear::RingLeft, Gear::RingRight,
                                             Gear::Neck,       Gear::Shoulder,    Gear::Chest,    Gear::Hands,
                                             Gear::Feet,       Gear::Pet};
        return slots[screenSlot];
    };
    if (!nativeInventory && nextWeapon && haveGear && gear.cycle(Gear::WeaponRight, [&](const Gear::Item &it) {
            return (catalog.mayUse(it.category, it.id, it.material, gear.playerClass()) && levelAllows(it));
        })) {
        applyGear();
        logLine("Right hand: " + gear.describe(gear.equipped(Gear::WeaponRight)));
    }
    // What the character wore and how hurt they were when last saved.
    if (!nativeInventory && haveGear && saveGame.hasGear) {
        saveGame.applyGear(gear);
        applyGear();
    }
    if (resumed)
        playerHp = saveGame.hpFraction * playerMaxHp;

    // Enemy levels (pyro): each creature's level follows the player's by the game's
    // region type it lives in. Its recorded stats were taken beside a level-1
    // player, so they scale by the mod's factor between the two levels. Vanilla
    // keeps the recorded numbers.
    std::vector<int> enemyLevels(actors.count(), 0);
    const int worldSeed = useExtracted ? extractedWorld.seed() : 0;
    auto levelEnemies = [&] {
        for (size_t k = 0; k < actors.count(); k++) {
            size_t r = actors.recordingOf(k);
            if (r >= creatures.all().size() || creatures.all()[r].positions.empty())
                continue;
            const auto &c = creatures.all()[r];
            int cellX = 0, cellY = 0, region = 0;
            if (useExtracted)
                region = std::max(0, extractedWorld.regionTypeAt(c.positions[0].x, c.positions[0].y, cellX, cellY));
            uint64_t roll = progression::regionRoll(worldSeed, cellX, cellY);
            int recorded = progression::enemyLevel(1, region, roll, c.id);
            if (mode == progression::Mode::Pyro) {
                enemyLevels[k] = progression::enemyLevel(regionBasis({cellX, cellY}), region, roll, c.id);
                actors.strength(k, progression::creatureScale(enemyLevels[k]) / progression::creatureScale(recorded));
            } else {
                enemyLevels[k] = 0;
                actors.strength(k, 1);
            }
        }
    };
    session.regionLevelsChanged = levelEnemies;
    levelEnemies();
    applyGear();
    // Floating purple text ("+5 XP", "LEVEL UP!") and the loot pouches monsters drop.
    struct FloatingText {
        glm::vec3 at;
        std::string text;
        glm::u8vec4 color;
        double age = 0;
        bool big = false;
    };
    std::vector<FloatingText> texts;
    const glm::u8vec4 PURPLE{166, 102, 255, 255}; // PyroProgression's colour (0.65, 0.40, 1.0)
    struct Loot {
        double asked = -10; // when this player last asked its host for it (a pick-up interaction)
        glm::vec3 at;
        double age = 0;
        int gold = 0;
        bool hasItem = false;
        Gear::Item item;
        Gear::Slot slot = Gear::SLOTS;
        bool toBag = false; // an item that is no gear: carried in the bag
        // The game's ground item (a zone's item, 0x2a5b70; loot-pickup.md): lies where it fell as
        // its own model, turned by `rotation` (degrees), hopping in place while its pick-up delay
        // `dropMs` (+0xc8) runs; coins are then taken within 4 blocks, anything else with the use key.
        bool onGround = false, taken = false;       // (taken: the use key chose it, or it was walked over)
        bool notice = false;                        // picking it up is received as the game's pick-up notice (§4)
        std::array<int, 3> zoneItem{INT_MIN, 0, 0}; // a zone's own item: its zone and index
        std::array<int32_t, 2> zone{INT_MIN, 0};    // a dropped item: the zone whose list holds it
        int model = -1;
        float rotation = 0, dropMs = 0;
        int32_t timerMs = 0;    // +0xcc: not drawn while above 0
        uint8_t flags = 0;      // +0xc4: bit 1, it stays for others (no automatic pick-up)
        int count = 1;          // +0xd4
        int noticeType = 0;     // the item's type before the class adaptation (type 0x1e treasure)
        int64_t droppedBy = 0;  // the creature that dropped it
        bool forPlayer = false; // reserved for this player (the ground item's owner +0xd8)
        std::array<uint8_t, 0xa0> raw{}; // a zone's ground item: its item as the zone keeps it
    };
    std::vector<Loot> loot;
    // Where a dropped item is drawn, as the game draws a zone's items: scaled (1/14; coins
    // 0.04), lying or standing by category, arcing five blocks high while it drops and
    // tumbling. The model (of `size`) has its corner at the matrix's origin.
    auto dropPlacement = [](const Loot &l, glm::vec3 size, double time) {
        const int cat = l.item.category;
        const float scale = cat == 12 ? 0.04f : cat == 24 && (l.item.id == 1 || l.item.id == 2) ? 0.15f : 1.0f / 14.0f;
        const float clock = (float)(time * 1000.0);
        auto deg = [](float d) { return glm::radians(d); };
        glm::vec3 at = l.at;
        at.z += grounditems::hopHeight(l.dropMs); // the hop in place (loot-pickup.md §2)
        glm::mat4 m = glm::scale(glm::translate(glm::mat4(1), at), glm::vec3(scale));
        switch (cat) {
        case 3: case 4: // weapons and chest armour lie on their side
            m = glm::rotate(m, deg(l.rotation), glm::vec3(0, 0, 1));
            m = glm::translate(m, glm::vec3(0, 0, size.x * .5f));
            m = glm::rotate(m, deg(90), glm::vec3(0, 1, 0));
            m = glm::translate(m, -size * .5f);
            break;
        case 23: case 24: case 29: case 30: // artifacts, specials, glyphs: turning above the ground
            m = glm::rotate(m, deg(clock * .1f + l.rotation), glm::vec3(0, 0, 1));
            m = glm::translate(m, glm::vec3(0, 0, size.x * .85f));
            m = glm::translate(m, -size * .5f);
            break;
        default:
            m = glm::rotate(m, deg(l.rotation), glm::vec3(0, 0, 1));
            m = glm::translate(m, glm::vec3(-size.x * .5f, -size.y * .5f, 0));
        }
        if (l.dropMs > 0) { // tumbling about its centre while it hops (the axis is not specified)
            m = glm::translate(m, size * .5f);
            m = glm::rotate(m, deg(grounditems::tumble(l.dropMs)), glm::vec3(1, 0, 0));
            m = glm::translate(m, -size * .5f);
        }
        return m;
    };
    std::set<std::pair<int32_t, int32_t>> zoneItemsLoaded; // zones whose lying items are in `loot`
    std::set<std::array<int, 3>> takenZoneItems;           // zone items taken (zone x, y, index)
    for (const auto &t : saveGame.takenItems)
        takenZoneItems.insert({t[0], t[1], t[2]});
    // The reward chests of the loaded zones (type 22 with their items), and the cells of
    // those this character opened (the game's discovered markers).
    std::vector<worldgen::ZoneObject> rewardChests;
    // The workstations of the loaded zones (their static entity type and where).
    std::vector<std::pair<int, glm::vec3>> workstations;
    // The cells the character used (reward chests opened, lore read), and its lore by realm,
    // rebuilt from them (Lore.h).
    std::set<std::array<int32_t, 2>> usedCells(saveGame.usedCells.begin(), saveGame.usedCells.end());
    lore::Book loreBook = lore::rebuild(usedCells);
    spawned.setUsedCells(&usedCells);
    world.setUsedCellProvider([&usedCells](int32_t x, int32_t y) { return usedCells.count({x, y}) != 0; });
    steampoints::TransientLights transientLights;
    std::vector<cwnet::DamageRecord> hitVisuals;
    spawned.setHitVisualObserver([&hitVisuals](const cwnet::DamageRecord &number) { hitVisuals.push_back(number); });
    world.setTransientLightProvider([&transientLights] { return transientLights.values(); });
    world.setEffectLightProvider([&spawned] { return spawned.effectLights(); });
    world.setCreatureLightCollector([&spawned](const auto &frame, auto &out) { spawned.collectLights(frame, out); });
    world.setScenePlayerProvider([&spawned]() -> std::optional<scenerange::Player> {
        const auto *record = spawned.playerRecord();
        if (!record) return {};
        scenerange::Player player;
        std::memcpy(player.physical.data(),record+0x10,24);
        player.ability = record[0x68];
        std::memcpy(&player.abilityMs,record+0x6c,4);
        float strength, remaining;
        player.effect13 = spawned.playerBuff(0x13,strength,remaining);
        return player;
    });
    world.setGroundLightProvider([&](steampoints::ZoneKey key) {
        return spawnedReady ? spawned.groundLights(extractedWorld, key.first, key.second) : steampoints::GroundSources{};
    });
    world.setObjectProvider([&spawned](int zx, int zy, const objectvisual::Key &key) { return spawned.objectVisual(zx, zy, key); });
    // The places and lands discovered (the map names them), and the places townspeople marked.
    std::set<std::array<int32_t, 2>> discoveredPlaces(saveGame.discoveredPlaces.begin(), saveGame.discoveredPlaces.end());
    std::set<std::array<int32_t, 2>> discoveredAreas(saveGame.discoveredAreas.begin(), saveGame.discoveredAreas.end());
    for (const SaveGame::Marked &m : saveGame.marked)
        discoveredPlaces.insert({m.x, m.y});
    // Whether a place's mission is known (0x5f4c0): marked on the map, or solved.
    auto missionKnown = [&](std::array<int32_t, 2> place) {
        if (saveGame.solvedMissions.count(place))
            return true;
        return std::any_of(saveGame.marked.begin(), saveGame.marked.end(),
                           [&](const SaveGame::Marked &m) { return m.x == place[0] && m.y == place[1]; });
    };
    // The player's position in the game's blocks.
    auto playerInBlocks = [&]() {
        return glm::dvec3((double)player.position.x + (double)(extractedWorld.originDotsX() >> 16),
                          (double)player.position.y + (double)(extractedWorld.originDotsY() >> 16),
                          (double)player.position.z - ExtractedWorld::Z_OFFSET);
    };
    // The tombstone where the player died (revived there; `east` blocks east of it for testing),
    // on the map for five minutes (MapMarks.h).
    auto placeTombstone = [&](float east) {
        const glm::dvec3 at = playerInBlocks();
        saveGame.deathPosition = {(int64_t)std::floor(at.x + east), (int64_t)std::floor(at.y), (int64_t)std::floor(at.z)};
        saveGame.tombstoneMs = mapmarks::TOMBSTONE_MS;
    };
    // The use key (UsePrompt.h): its state between frames, this frame's prompt and the icon
    // shown with it, the view its crosshair is in (last frame's camera), a chest it opened
    // (given once the frame's drops are made) and whether it revives the dead player.
    useprompt::KeyState useKey;
    itemselector::Selector itemSelector;
    itemSelector.shown = openSelector;
    itemselector::SoundEdges interfaceSoundEdges;
    bool quickItemWasDown = false;
    useprompt::Prompt usePrompt;
    std::string useIcon;
    useprompt::Camera useCamera;
    std::optional<worldgen::ZoneObject> chestToOpen;
    bool reviveNow = false;
    bool autoUsePress = false; // --auto-use: the key pressed once
    // What is carried besides gear (the game's items: resources, food, pets, specials),
    // stacked by kind; kept with the character.
    struct Carried {
        Gear::Item item;
        int count = 0;
    };
    std::vector<Carried> bag;
    std::function<void()> rebuildInventoryViews;
    std::set<std::string> inventoryIconAttempts; // this session owns its Hud and GPU resources
    auto loadInventorySprite = [&](const std::string &sprite) {
        if (sprite.empty() || !inventoryIconAttempts.insert(sprite).second)
            return;
        if (const auto path = assets.resolve(fs::path("sprites") / (sprite + ".cub"))) {
            CubModel cub;
            if (cub.load(*path)) {
                hud.addItemIcon(sprite, cub);
                hud.layout(renderer, renderer.screenSize());
            }
        }
    };
    auto resetDailyProgress = [&] {
        nativeprogress::resetDay(saveGame);
        if (nativeInventory && rebuildInventoryViews) {
            rebuildInventoryViews();
            return;
        }
        bag.erase(std::remove_if(bag.begin(), bag.end(), [](const Carried &item) { return item.item.category == 0x16; }), bag.end());
    };
    // The artifacts carried (Lore.h, spec 5.2): each stack of the artifact tab adds a level and a
    // point to the skill its modifier names.
    auto carriedArtifacts = [&]() {
        std::vector<worldgen::Item> stacks;
        for (const Carried &c : bag)
            if (c.item.category == lore::ARTIFACT_CATEGORY) {
                worldgen::Item stack;
                stack.category = (uint8_t)c.item.category, stack.id = c.item.id, stack.modifier = c.item.modifier;
                stacks.push_back(stack);
            }
        return lore::artifactProgress(stacks);
    };
    std::set<std::array<int32_t, 3>> usedObjects(saveGame.usedObjects.begin(), saveGame.usedObjects.end());
    // A lore object examined with the use key (Lore.h): the tale of its cell, told from where
    // the object stands.
    struct Examined {
        lore::Reading reading; // an empty tale: nothing to read
        glm::vec3 at{0};       // where the object stands
    };
    if (saveGame.bag.empty() && haveGear && !(chosen && chosen->native)) // a new save: what the recorded character carried
        for (auto &[item, count] : gear.carriedExtras())
            bag.push_back({item, count});
    if (!bag.empty())
        logLine("Bag: " + std::to_string(bag.size()) + " kinds carried from the recorded character");
    for (const SaveGame::Bagged &b : saveGame.bag) {
        Carried c;
        if (b.storedRecord)
            c.item = Gear::fromRecord(*b.storedRecord);
        c.item.category = b.category, c.item.id = b.id, c.item.material = b.material, c.item.rarity = b.rarity;
        c.item.modifier = b.modifier, c.item.region[0] = b.region[0], c.item.region[1] = b.region[1];
        c.item.formula = b.formula;
        c.count = b.count;
        bag.push_back(c);
    }
    // PyroProgression's session(): the gear the character owns keeps the level the mod gave it
    // before levels were kept (by its region's distance and its modifier), unless it has one.
    if (mode == progression::Mode::Pyro) {
        const worldgen::Int2 here = regionHere();
        auto own = [&](const Gear::Item &it) {
            if (it.category >= 3 && it.category <= 9)
                pyroRegions.own(it.category, it.id, it.modifier, it.material,
                                pyrocreatures::RelativeLevels::legacyLevel({it.region[0], it.region[1]}, here, it.modifier));
        };
        for (size_t i = 0; i < gear.itemCount(); i++)
            own(gear.item(i));
        for (const Carried &c : bag)
            own(c.item);
    }
    // Eating (the game's item use, 0x2c5337 with abilities 0x54/0x55): over the food's
    // use time (0x108fa0), every 200 ms of it heals 200 / use time of the maximum health.
    double eatMs = -1; // ms into eating; -1 when not
    int eatUse = 0;    // the food's use time
    auto bagName = [&](const Gear::Item &it, const std::string &known) {
        return known.empty() ? "ITEM " + std::to_string(it.category) + ":" + std::to_string(it.id) : known;
    };

    const int pouchModel = world.loadModel("pouch1");
    // Deaths as the game shows them: the body bursts into its voxels (DeathEffects.h). Coins
    // are ground items (layCoins).
    DeathEffects deathFx;
    const int coinModel = world.loadModel("gold-coin");
    std::vector<char> wasDead(actors.count(), 0);
    uint32_t fragmentMeshId = 0;
    Mesh fragmentMesh;
    std::vector<std::pair<double, std::string>> deathShots; // --capture-death
    std::mt19937 lootDice(std::random_device{}());
    {
        size_t legacyFlags = 0, legacyModifiers = 0;
        for (const auto &c : creatures.all()) {
            legacyFlags += !c.flags2.has_value();
            for (const auto &w : c.worn) legacyModifiers += w.category && !w.modifier.has_value();
        }
        logLine("Loot metadata: " + std::to_string(legacyFlags) + " creatures lack appearance flags; " +
                std::to_string(legacyModifiers) + " worn items lack modifiers" +
                (legacyFlags || legacyModifiers ? "; legacy estimates remain (recapture with the updated extractor)" : ""));
    }
    // Equipment slot of an item category (the game's ids).
    auto slotForCategory = [](int category) {
        switch (category) {
        case 3: return Gear::WeaponRight;
        case 4: return Gear::Chest;
        case 5: return Gear::Hands;
        case 6: return Gear::Feet;
        case 7: return Gear::Shoulder;
        case 8: return Gear::Neck;
        case 9: return Gear::RingRight;
        default: return Gear::SLOTS;
        }
    };
    // The bag and Gear are presentation views. Native slots retain holes, exact records and live indices.
    rebuildInventoryViews = [&] {
        if (!nativeInventory)
            return;
        auto viewOf = [&](const nativesaves::Item &raw) {
            auto item = Gear::fromRecord(raw);
            if (item.category == 0)
                return item;
            if (spawnedReady)
                item.sprite = spawned.itemSprite(raw.data());
            if (item.sprite.empty())
                if (const auto *model = catalog.model(item.category, item.id, item.material); model && !model->sprites.empty())
                    item.sprite = *model->sprites.begin();
            loadInventorySprite(item.sprite);
            return item;
        };
        std::array<Gear::Item, Gear::SLOTS> equipment;
        for (int slot = 0; slot < Gear::SLOTS; ++slot) {
            equipment[(size_t)slot] = viewOf(nativeInventory->equipment[(size_t)slot]);
            if (equipment[(size_t)slot].category)
                foundStats(equipment[(size_t)slot], (Gear::Slot)slot);
        }
        gear.setEquipmentView(equipment);
        bag.clear();
        const auto &tabs = nativeInventory->inventory.tabs;
        for (size_t tab = 0; tab < tabs.size(); ++tab)
            for (size_t slot = 0; slot < tabs[tab].size(); ++slot) {
                const auto &stack = tabs[tab][slot];
                if (stack.count == 0)
                    continue;
                auto item = viewOf(stack.item);
                item.tab = (int)tab, item.index = (int)slot;
                const auto equipmentSlot = slotForCategory(item.category);
                if (equipmentSlot != Gear::SLOTS)
                    foundStats(item, equipmentSlot);
                bag.push_back({std::move(item), stack.count});
            }
    };
    auto refreshInventory = [&](bool pickup = false) {
        if (!nativeInventory)
            return;
        const auto fallback = nativeRecord(-1, nullptr);
        const uint8_t *actor = spawned.playerRecord();
        if (!actor)
            actor = fallback.data();
        inventoryslots::SortContext context;
        context.selectedRegion = inventoryView.activeRegion;
        context.usable = [actor](const auto &item) { return nativerules::mayUse(actor, item.data()); };
        context.power = [actor](const auto &item) { return nativerules::itemPower(item.data(), actor); };
        nativeInventory->inventory.refresh(context);
        if (pickup) // local pickup performs an explicit refresh, then the bound widget refresh
            nativeInventory->inventory.refresh(context);
        rebuildInventoryViews();
    };
    auto consumeCarried = [&](size_t row) {
        if (row >= bag.size())
            return;
        if (nativeInventory) {
            const auto &item = bag[row].item;
            nativeInventory->inventory.consumeOne({item.tab, item.index});
            rebuildInventoryViews();
        } else if (--bag[row].count <= 0)
            bag.erase(bag.begin() + (long long)row);
    };
    auto refreshBagOpening = [&] {
        const auto region = regionHere();
        inventoryView.activeRegion = {region.x, region.y};
        inventoryRegionActive = true;
        refreshInventory();
    };
    rebuildInventoryViews();
    if (nativeInventory && haveGear)
        applyGear();
    if (inventoryOpen && bagShown)
        refreshBagOpening();

    // Conversations and quests, in the game's own words: every sentence comes from
    // dict_en.xml through its grammar (Speech.h). Quests use the game's objective
    // and solved lines; the rules around them (who asks, rewards) are CubeVulkan's.
    itemNameParts.load(assets.gameDir() / "Extract" / "names.txt");
    {
        std::string why;
        logLine(speech.load(assets.gameDir() / "dict_en.xml", why) ? "Dialogue: " + std::to_string(speech.count()) + " lines from the game's dictionary"
                                                                   : "Dialogue: " + why);
    }
    // The names of lands (the land naming with the region's generated name) and places
    // (the place naming), for the info texts, the location banner, items and the map.
    auto landNameOf = [&](worldgen::Int2 region) -> std::string {
        static const char *const lands[14] = {"Plains",    "Ocean",      "Hills",     "Woodlands", "Jungle",
                                              "DarkWoods", "SnowForest", "Mountains", "Wetlands",  "Desert",
                                              "Savannah",  "Snowlands",  "Deadlands", "Firelands"};
        const int land = worldgen::regionBiome(region.x, region.y);
        return land >= 0 && land < 14 && itemNameParts.loaded()
                   ? speech.place(lands[land], itemNameParts.name((int)worldgen::random((uint32_t)region.x + (uint32_t)region.y * 57u), -1))
                   : std::string();
    };
    // `px`, `py`: the place's 1024-block cell, a big place's rounded down to even.
    auto placeNameOf = [&](int32_t px, int32_t py, int kind) -> std::string {
        static const char *const placeKeys[48] = {
            nullptr,        "House",        "Farm",      "Inn",        "Crypt",       "Village",     nullptr,      "Tower",
            "Castle",       "Graveyard",    "Mountain",  nullptr,      "Cave",        "Lake",        "LavaLake",   "AncientRuins",
            "CircleOfPower", "ManaPump",    "Crater",    "Volcano",    "Tree",        "Rock",        "OldHut",     "Island",
            "Arena",        "Forest",       "DarkForest", "EnchantedForest", "Canyon", "Rocks",      "Plain",      "Hill",
            "Grove",        "Glade",        "Camp",      "BanditCamp", "Beach",       "Altar",       "Sanctuary",  "Menhir",
            "Catacombs",    "Sewers",       "Mines",     "Shrine",     "Grotto",      "Pyramid",     "Temple",     "Vault"};
        if (kind <= 0 || kind >= 48 || !placeKeys[kind] || !itemNameParts.loaded())
            return "";
        int race = -1; // the city name variant
        const uint32_t pick = worldgen::random((uint32_t)px + 583478u + (uint32_t)py * 57u);
        if (kind == 1 || kind == 2 || kind == 3) {
            static const int races[16] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            race = races[pick & 15];
        } else if (kind == 7)
            race = pick & 1 ? 167 : 43;
        else if (kind == 22)
            race = 45;
        return px == 0 && py == 0 ? "???" : speech.place(placeKeys[kind], itemNameParts.name((int)worldgen::random((uint32_t)px + 380529u + (uint32_t)py * 57u), race));
    };
    // What townspeople say, as the game decides it: the game's townspeople get the script
    // the game would build for them today (TalkGen.h). The scripts ZoneExtractor exported
    // from the game (TalkScripts.h) speak for the people CubeVulkan has no record of, and,
    // with [Talk] UseExports=1, for everyone they cover. People with neither keep
    // CubeVulkan's small talk from the game's lines.
    TalkScripts talkScripts;
    {
        std::string why;
        logLine(talkScripts.load(assets.gameDir() / "Extract" / "talk-scripts.txt", why)
                    ? "Conversations: " + std::to_string(talkScripts.count()) + " townspeople's scripts from the game"
                    : "Conversations: " + why);
    }
    const bool useTalkExports = config.flag("Talk", "UseExports", false);
    talkgen::CRand talkRand; // the game's C rand(), which conversations draw from as the game's thread does
    // The names a generated script needs (the exporter writes them into its scripts).
    TalkNames talkNames;
    talkNames.place = [&](int x, int y, int kind) { return placeNameOf(x, y, kind); };
    talkNames.person = [&](int id, int race) { return itemNameParts.loaded() ? itemNameParts.name(id, race) : std::string(); };
    talkNames.land = [&](int, int seed) { return itemNameParts.loaded() ? itemNameParts.name(seed, -1) : std::string(); };
    talkNames.item = [&](const int(&it)[5], std::string &name, std::string &key) {
        key = itemnames::itemKey(it[0], it[1]);
        if (speech.count() && itemNameParts.loaded()) {
            const nativeplayer::Item raw = nativeplayer::item(it[0], it[1], it[3], it[4], (uint32_t)it[2], 0, 0);
            name = itemnames::name(speech, itemNameParts, raw.data());
            if (name == "unknown")
                name.clear();
        }
    };
    // The player as a conversation sees it (TalkGen's Player): class, gear, pet, gold, the
    // special items and artifacts carried, and the missions.
    auto talkPlayer = [&]() {
        auto gameItem = [](const Gear::Item &it) {
            worldgen::Item g;
            g.category = (uint8_t)it.category, g.id = it.id, g.modifier = it.modifier;
            g.region = {it.region[0], it.region[1]};
            g.rarity = (uint8_t)it.rarity, g.realm = (uint32_t)it.formula, g.material = (uint8_t)it.material;
            return g;
        };
        talkgen::Player p;
        p.cls = chosen ? chosen->classType : 0, p.spec = chosen ? chosen->specialization : 0;
        for (int s = 0; s < 10; s++)
            p.equipment[s] = gameItem(gear.equipped((Gear::Slot)s));
        p.pet = gameItem(gear.equipped(Gear::Pet));
        p.gold = playerGold;
        p.finished = [&](talkgen::Cell c) { return saveGame.finishedMissions.count(c) != 0; };
        p.completed = [&](talkgen::Cell c) { return missions::completed(saveGame.finishedMissions, saveGame.solvedMissions, c); };
        p.known = [&](talkgen::Cell c) { return missionKnown(c); };
        p.hasSpecial = [&](int id) { // (an item 24 of the player's region; 4 always counts)
            const worldgen::Int2 here = regionHere();
            for (const Carried &c : bag)
                if (c.item.category == 24 && c.item.id == id && c.item.region[0] == here.x && c.item.region[1] == here.y)
                    return true;
            return id == 4;
        };
        p.carries = [&](const worldgen::Item &it) {
            for (const Carried &c : bag)
                if (c.item.category == it.category && c.item.id == it.id && c.item.modifier == it.modifier &&
                    c.item.region[0] == it.region.x && c.item.region[1] == it.region.y && (uint32_t)c.item.formula == it.realm)
                    return true;
            return false;
        };
        return p;
    };
    // The "defeat the enemies in a zone" requests are CubeVulkan's own: the game never
    // gives them (its objective_/solved_ lines are unused; docs/dialogue.md). Off
    // unless [Quests] CubeVulkanQuests=1.
    const bool inventedQuests = config.flag("Quests", "CubeVulkanQuests", false);
    std::vector<SaveGame::Quest> quests = saveGame.quests;
    // Proper names. The game makes its own in code (Lalan, Telgor, Ikolon, Ardara);
    // these are CubeVulkan's, in that style, fixed for each person or place.
    auto properName = [](uint64_t seed, int syllables) {
        static const char *starts[] = {"La", "Te", "I", "Ar", "De", "Go", "Ka", "Mo", "Su", "Be", "Ro", "Fa",
                                       "Ne", "Ul", "Ve", "Za", "Ho", "Pi", "Tel", "Des", "Lan", "Or", "Ma", "Ki"};
        static const char *parts[] = {"lan", "gor", "ko", "lon", "da", "ra", "kor", "tar", "tik", "ran", "mel",
                                      "dor", "vin", "sa", "ru", "bel", "nok", "ta", "li", "mar", "go", "ne"};
        uint64_t h = progression::mix(seed);
        std::string name = starts[h % std::size(starts)];
        for (int i = 1; i < syllables; i++) {
            h = progression::mix(h + i);
            name += parts[h % std::size(parts)];
        }
        return name;
    };
    // A place's name and kind ("Plains") from the game's region type there.
    struct Place {
        std::string kind, name;
    };
    auto placeAt = [&](float x, float y) {
        static const char *kinds[] = {"Plains", "Ocean", "Hills", "Woodlands", "Jungle", "DarkWoods", "SnowForest",
                                      "Mountains", "Wetlands", "Desert", "Savannah", "Snowlands", "Deadlands", "Firelands"};
        int cx = 0, cy = 0, type = useExtracted ? extractedWorld.regionTypeAt(x, y, cx, cy) : 0;
        Place p;
        p.kind = type >= 0 && type < (int)std::size(kinds) ? kinds[type] : "Lands";
        uint64_t seed = progression::regionRoll(worldSeed, cx, cy);
        p.name = speech.place(p.kind, properName(seed, 2 + int(seed % 2)));
        return p;
    };
    // The dictionary id of each recording's creature ("RadishCreature"), for quests.
    std::vector<std::string> creatureNameIds;
    for (auto &c : creatures.all()) {
        std::string id;
        for (auto &p : c.parts)
            if (id.empty())
                id = hud.nameIdForSprite(p.sprite);
        creatureNameIds.push_back(id);
    }
    // A kill counts for every quest after that creature.
    auto questKilled = [&](const std::string &nameId) {
        for (auto &q : quests)
            if (q.state == 0 && q.target == nameId && ++q.done >= q.need) {
                q.state = 1;
                Speech::Context context;
                std::string zoneKind = q.zone.substr(0, q.zone.find(':')), zone = q.zone.substr(q.zone.find(':') + 1);
                if (speech.nameTags(zoneKind).find("the") != std::string::npos)
                    context.tags.insert("zone_the");
                context.value = [&](const std::string &fn, const std::string &args) -> std::string {
                    if (fn == "mob") return speech.name(q.target, args.find("plural") != std::string::npos);
                    if (fn == "zone") return zone;
                    return "";
                };
                std::mt19937 fixed((uint32_t)q.giver);
                std::string line = speech.say("solved_defeat_enemies_in_zone", context, fixed);
                texts.push_back({player.position + glm::vec3(0, 0, 3.4f), line, PURPLE, 0, true});
                logLine("Quest done: " + line);
            }
    };
    // A piece of gear of a kind the game's creatures wear, its rarity around `stars`,
    // its stats scaled from the survey (foundStats). False when nothing fits.
    auto rollItem = [&](int stars, int enemy, Gear::Item &it, Gear::Slot &slot) {
        if (!haveGear)
            return false;
        std::vector<std::tuple<int, int, int>> kinds;
        for (auto &[key, m] : catalog.models()) {
            Gear::Slot s = slotForCategory(std::get<0>(key));
            if (s != Gear::SLOTS && gear.reference(s, std::get<0>(key)))
                kinds.push_back(key);
        }
        if (kinds.empty())
            return false;
        auto [category, id, material] = kinds[lootDice() % kinds.size()];
        it = Gear::Item{};
        it.category = category, it.id = id, it.material = material;
        it.rarity = std::clamp(stars + (int)(lootDice() % 3) - 1, 0, 4);
        it.modifier = (unsigned)lootDice();
        it.level = dropLevel(it, enemy);
        if (auto *m = catalog.model(category, id, material))
            it.sprite = *m->sprites.begin();
        slot = slotForCategory(category);
        return foundStats(it, slot);
    };

    // XP (pyro) and quest progress for a kill of this player's own (in multiplayer each player's
    // progress is its own: nothing of it is shared, netcode-roles.md §6).
    // Coins laid on the ground as the game's drop lays them (loot-pickup.md §1–§3): each hops in
    // place for its pick-up delay of 500..799 ms, then is taken if the player is within 4 blocks,
    // or stays on the ground. `value` is each coin's gold (1; Pyro mode's share of the level).
    auto layCoins = [&](glm::vec3 at, int coins, int value, int64_t from) {
        for (int k = 0; k < coins; k++) {
            Loot l;
            l.at = at;
            l.hasItem = true, l.toBag = true, l.onGround = true, l.notice = true;
            l.item.category = grounditems::type::COIN, l.item.material = grounditems::COIN_MATERIAL;
            l.noticeType = grounditems::type::COIN;
            l.gold = value;
            l.model = coinModel;
            l.droppedBy = from;
            l.rotation = (float)(lootDice() % 32768) * 360.0f / 32767.0f;
            l.dropMs = (float)(lootDice() % 32768 % 300 + 500);
            loot.push_back(l);
        }
    };
    // `coinsDropped`: the game's drop laid the coins already (0x2a6860, the generated world).
    auto creditKill = [&](int stars, int enemy, const std::string &nameId, glm::vec3 at, bool coinsDropped = false) {
        if (!nameId.empty())
            questKilled(nameId);
        // The recorded monsters' coins: stars + 1, one gold each (measured). Pyro mode keeps its
        // own amount (the enemy's level), shared between them.
        if (!coinsDropped) {
            const int coins = stars + 1;
            layCoins(at, coins, mode == progression::Mode::Pyro ? std::max(1, enemy / coins) : 1, 0);
        }
        if (mode == progression::Mode::Pyro) {
            int gain = std::max(1, progression::killXp(stars, enemy, playerLevel));
            playerXp += gain;
            texts.push_back({at + glm::vec3(0, 0, 2.8f), "+" + std::to_string(gain) + " XP", PURPLE});
            logLine("Gained " + std::to_string(gain) + " XP (enemy level " + std::to_string(enemy) + ", " + std::to_string(stars) + " stars)");
            bool levelled = false;
            // The creatures' levels follow the player's: they keep their share of their health.
            (spawnedReady ? hostWorld : spawned).keepHealthShares([&] {
                while (playerXp >= progression::xpToNext(playerLevel)) {
                    playerXp -= progression::xpToNext(playerLevel);
                    playerLevel++;
                    levelled = true;
                }
            });
            if (levelled) {
                applyGear();
                playerHp = playerMaxHp; // The mod restores health on a level up.
                levelEnemies();         // Enemies follow the player's level.
                texts.push_back({player.position + glm::vec3(0, 0, 3.2f), "LEVEL UP!", PURPLE, 0, true});
                audio.playInterface(soundrules::soundid::LEVEL_UP); // with the banner (sound.md §5.1)
                logLine("Level up: level " + std::to_string(playerLevel));
            }
        }
    };

    // A defeated recorded monster (the recorded world, single player only): the kill counts,
    // then gold and sometimes an item in a pouch.
    // The world remembers who died (saved with the character, shared by the world).
    auto rememberKill = [&](size_t k) {
        if (enemyRespawns.mode != enemyrespawn::Mode::Permanent)
            return;
        int64_t id = actors.creatureId(k);
        if (id && std::find(saveGame.killed.begin(), saveGame.killed.end(), id) == saveGame.killed.end())
            saveGame.killed.push_back(id);
    };
    auto defeated = [&](size_t k) {
        size_t r = actors.recordingOf(k);
        int stars = r < creatures.all().size() ? std::max(0, creatures.all()[r].level) : 0;
        int enemy = enemyLevels[k];
        std::string nameId = r < creatureNameIds.size() ? creatureNameIds[r] : "";
        creditKill(stars, enemy, nameId, actors.position(k));
        rememberKill(k);
        glm::vec3 at = actors.position(k);
        // Gold comes as coins (creditKill). Items follow the game's drop function
        // (RVA 0x2a6860, docs/progression.md "Item drops"), each landing within a block
        // of the body:
        //   * none from hostility types 0 and 3;
        //   * 1 in 16: one eligible worn item (native slot order and exclusions);
        //   * 1 in 50: what it wears in the pet slot;
        //   * hostility 1 or 7, 1 in 20: a generated item. The game's two generators
        //     (0x2d0680, 0x2d15d0) are not decoded yet: rollItem stands in for them.
        // Recorded appearance flags supply the native suppress/guarantee branches.
        // Legacy captures without them retain the reported clear-flags estimate.
        // Race resources, inventory and a third generator remain unfinished.
        if (r >= creatures.all().size())
            return;
        const auto &creature = creatures.all()[r];
        if (creature.hostility == 0 || creature.hostility == 3 || !haveGear)
            return;
        std::uniform_int_distribution<unsigned> nativeRand(0, 32767);
        auto selection = nativeLoot::select(creature, [&] { return nativeRand(lootDice); });
        auto drop = [&](const Gear::Item &item, Gear::Slot slot) {
            Loot l;
            std::uniform_real_distribution<float> offset(-1, 1);
            l.at = at + glm::vec3(offset(lootDice), offset(lootDice), 0);
            l.hasItem = true, l.item = item, l.slot = slot;
            loot.push_back(l);
            logLine("Dropped " + itemName(item));
        };
        // A worn item as the game keeps it: its kind, rarity and the level rules of the mode.
        auto wornItem = [&](const CreatureRecordings::Creature::Worn &w, Gear::Item &it, Gear::Slot &slot) {
            it = Gear::Item{};
            it.category = w.category, it.id = w.id, it.material = w.material, it.rarity = std::clamp(w.rarity, 0, 4);
            it.modifier = w.modifier.has_value() ? *w.modifier : (unsigned)lootDice();
            it.level = dropLevel(it, enemy);
            if (auto *m = catalog.model(w.category, w.id, w.material); m && !m->sprites.empty())
                it.sprite = *m->sprites.begin();
            slot = slotForCategory(w.category);
            return slot != Gear::SLOTS && foundStats(it, slot);
        };
        Gear::Item item;
        Gear::Slot slot;
        if (selection.wornSlot != SIZE_MAX && wornItem(creature.worn[selection.wornSlot], item, slot))
            drop(item, slot);
        if (selection.pet && wornItem(creature.worn[10], item, slot))
            drop(item, slot);
        if (selection.generated && rollItem(stars, enemy, item, slot))
            drop(item, slot);
    };
    // What a pouch holds, now this player's: gold, and its item into the inventory.
    auto collect = [&](const Loot &l) {
        if (l.hasItem && l.item.category == 24) // a special item obtained: the success track (sound.md §3.3)
            audio.startSuccess();
        if (l.hasItem && l.item.category == grounditems::type::GOLD_BAG) { // its money value (use-prompt.md §3.7.3)
            const int coins = grounditems::moneyValue(nativeplayer::item(l.item.category, l.item.id, l.item.material, l.item.rarity,
                                                                         l.item.modifier, l.item.region[0], l.item.region[1]));
            playerGold += coins;
            texts.push_back({player.position + glm::vec3(0, 0, 2.6f), "+" + std::to_string(coins) + " GOLD", {255, 214, 64, 255}});
            logLine("Picked up a gold bag: " + std::to_string(coins) + " gold");
            return;
        }
        if (l.gold > 0) {
            playerGold += l.gold;
            texts.push_back({player.position + glm::vec3(0, 0, 2.6f), "+" + std::to_string(l.gold) + " GOLD", {255, 214, 64, 255}});
            logLine("Picked up " + std::to_string(l.gold) + " gold");
        }
        if (!l.hasItem)
            return;
        if (nativeInventory) {
            nativeInventory->inventory.add(Gear::toRecord(l.item), 1);
            rebuildInventoryViews();
            texts.push_back({player.position + glm::vec3(0, 0, 3.1f), bagName(l.item, itemName(l.item)), {255, 255, 255, 255}});
            return;
        }
        if (l.toBag) {
            Carried *same = nullptr;
            if (worldgen::Inventory::stackable((uint8_t)l.item.category)) {
                const auto incoming = Gear::toRecord(l.item);
                for (auto &c : bag) {
                    const auto carried = Gear::toRecord(c.item);
                    if (nativeinteract::sameItem(carried.data(), incoming.data())) {
                        same = &c;
                        break;
                    }
                }
            }
            if (same)
                same->count++;
            else
                bag.push_back({l.item, 1});
            const std::string name = bagName(l.item, itemName(l.item));
            texts.push_back({player.position + glm::vec3(0, 0, 3.1f), name, {255, 255, 255, 255}});
            logLine("Picked up " + name + " (carried: " + std::to_string(same ? same->count : 1) + ")");
            return;
        }
        gear.add(l.item, l.slot);
        if (!l.item.sprite.empty())
            if (auto path = assets.resolve(fs::path("sprites") / (l.item.sprite + ".cub"))) {
                CubModel cub;
                if (cub.load(*path)) {
                    hud.addItemIcon(l.item.sprite, cub);
                    hud.layout(renderer, renderer.screenSize());
                }
            }
        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), itemName(l.item), {255, 255, 255, 255}});
        logLine("Picked up " + itemName(l.item) + " (rarity " + std::to_string(l.item.rarity) +
                (mode == progression::Mode::Pyro ? ", level " + std::to_string(l.item.level) : "") + ")");
    };
    // The game-layout item of an inventory item (for prices, the shop and the events).
    auto rawOf = [](const Gear::Item &it) { return Gear::toRecord(it); };
    // An item received (bought, a reward): as a picked-up item goes, gear or the bag.
    auto receiveMany = [&](const gameshop::Item &raw, int quantity) {
        const uint8_t *n = raw.data();
        Gear::Item it = Gear::fromRecord(raw);
        it.level = 1;
        if (auto *m = catalog.model(it.category, it.id, it.material); m && !m->sprites.empty())
            it.sprite = *m->sprites.begin();
        if (it.sprite.empty() && spawnedReady)
            it.sprite = spawned.itemSprite(n);
        const Gear::Slot slot = slotForCategory(it.category);
        Loot l;
        l.hasItem = true, l.item = it;
        l.slot = haveGear && slot != Gear::SLOTS && foundStats(l.item, slot) ? slot : Gear::SLOTS;
        l.toBag = l.slot == Gear::SLOTS;
        if (nativeInventory) {
            nativeInventory->inventory.add(raw, quantity);
            refreshInventory();
            if (it.category == 24)
                audio.startSuccess();
            texts.push_back({player.position + glm::vec3(0, 0, 3.1f), bagName(it, itemName(it)), {255, 255, 255, 255}});
        } else
            for (int count = 0; count < quantity; ++count)
                collect(l);
        return true;
    };
    auto receive = [&](const gameshop::Item &raw) { return receiveMany(raw, 1); };
    // OverworldEvents ([Events], off by default): DieuDeGlace's encounters and village quests,
    // run on the engine's world (OverworldEvents.h). Its progress is kept apart from the
    // Steam game's, in Saves/OverworldEvents.state.
    OverworldEvents events;
    {
        OverworldEvents::Settings s;
        s.enabled = useExtracted && config.flag("Events", "Enabled", false);
        s.slots = (unsigned)config.number("Events", "EventsPerRegion", 3);
        s.cap = (unsigned)config.number("Events", "ActiveEventCap", 2);
        s.cooldown = (unsigned)config.number("Events", "CooldownMinutes", 120);
        s.landmarkCap = (unsigned)config.number("Events", "LandmarkEventCap", 2);
        events.configure(s, root / "Saves" / "OverworldEvents.state", gameWorldSeed, gameWorldName);
    }
    // What the events see of the world now: its creatures and terrain, the player, the bag.
    auto eventWorld = [&] {
        auto same = [](const Gear::Item &a, const gameshop::Item &b) {
            int32_t id, region[2];
            uint32_t modifier;
            std::memcpy(&id, b.data() + 4, 4), std::memcpy(&modifier, b.data() + 8, 4), std::memcpy(region, b.data() + 0xc, 8);
            return a.category == b[0] && a.id == id && a.material == b[0x1c] && a.rarity == (int)(int8_t)b[0x14] && a.modifier == modifier &&
                   a.region[0] == region[0] && a.region[1] == region[1];
        };
        OverworldEvents::World w{spawnedReady ? hostWorld : spawned, extractedWorld}; // (the world that decides)
        w.player = {(int64_t)std::floor(player.position.x) + (extractedWorld.originDotsX() >> 16),
                    (int64_t)std::floor(player.position.y) + (extractedWorld.originDotsY() >> 16)};
        w.playerLevel = playerLevel;
        w.playerClass = haveGear ? gear.playerClass() : 1;
        if (mode == progression::Mode::Pyro) // (PyroProgressionCreatureLevel)
            w.creatureLevel = creatureLevel;
        w.give = receive;
        w.holds = [&, same](const gameshop::Item &item) {
            int n = 0;
            for (const Carried &c : bag)
                if (same(c.item, item))
                    n += c.count;
            return n;
        };
        w.take = [&, same](const gameshop::Item &item) {
            for (size_t k = 0; k < bag.size(); k++)
                if (same(bag[k].item, item)) {
                    consumeCarried(k);
                    return true;
                }
            return false;
        };
        return w;
    };
    // Talking to townspeople (E, the game's "use" key): one line at a time in a
    // bubble over their head. Some have a request: a quest to defeat creatures.
    // Who is talking: a recorded actor, or one of the game's own creatures (by its id).
    struct Speaker {
        bool native = false;
        size_t actor = SIZE_MAX;
        int64_t id = 0;
        int race = 0;
    };
    struct Talk {
        Speaker npc;
        std::vector<std::string> lines;
        size_t line = 0;
        double shown = 0;
        std::vector<std::vector<TalkPlay::Marked>> pageMarks;
        size_t effectsPage = SIZE_MAX;
    };
    std::optional<Talk> talk;
    dialoguechoice::Panel answers;
    dialoguepanel::General generalDialogue;
    npcspeech::Playback npcSpeech;
    std::optional<Speaker> lastTalkedSpeaker;
    size_t talkedTo = SIZE_MAX; // --talk: who was talked to
    std::mt19937 chatter(std::random_device{}());
    auto personOf = [&](const Speaker &s) {
        if (s.native)
            return (const CreatureRecordings::Creature *)nullptr;
        return actors.recordingOf(s.actor) < creatures.all().size() ? &creatures.all()[actors.recordingOf(s.actor)] : nullptr;
    };
    auto speakerAt = [&](const Speaker &s) {
        glm::vec3 at(0);
        if (!s.native)
            return actors.position(s.actor);
        spawned.positionOf(extractedWorld, s.id, at);
        return at;
    };
    auto speakerId = [&](const Speaker &s) -> int64_t {
        if (s.native)
            return s.id;
        const auto *c = personOf(s);
        return c ? (int64_t)c->id : 0;
    };
    auto speakerContext = [&](const Speaker &k, const SaveGame::Quest *q, glm::vec3 target) {
        Speech::Context context;
        const auto *c = personOf(k);
        bool female = false;
        if (c)
            for (auto &p : c->parts)
                female = female || p.sprite.find("-head-f") != std::string::npos;
        if (k.native) // the playable races come in pairs, male then female
            female = k.race < 16 && (k.race & 1);
        context.tags.insert(female ? "mob_f" : "mob_m");
        std::string zoneKind, zone;
        if (q) {
            zoneKind = q->zone.substr(0, q->zone.find(':')), zone = q->zone.substr(q->zone.find(':') + 1);
            // Which way the creatures are, as the game says it ("east of here", or "nearby").
            glm::vec2 d = glm::vec2(target - speakerAt(k));
            if (glm::length(d) > 40) {
                static const char *dirs[] = {"east", "northeast", "north", "northwest", "west", "southwest", "south", "southeast"};
                int octant = (int)std::lround(std::atan2(d.y, d.x) / (3.14159265f / 4));
                context.tags.insert(dirs[(octant + 8) % 8]);
            }
        } else {
            Place home = placeAt(speakerAt(k).x, speakerAt(k).y);
            zoneKind = "Village", zone = speech.place("Village", properName(progression::mix(home.name.size() * 131 + (uint64_t)worldSeed), 2));
        }
        if (speech.nameTags(zoneKind).find("the") != std::string::npos)
            context.tags.insert("zone_the");
        const int64_t who = speakerId(k);
        const TalkScript *native = who ? talkScripts.find(who, day) : nullptr;
        std::string name = native && !native->speaker.empty() ? native->speaker : who ? properName((uint64_t)who, 2) : "";
        std::string target2 = q ? q->target : "";
        context.value = [name, zone, zoneKind, target2, &speech](const std::string &fn, const std::string &args) -> std::string {
            if (fn == "mob_name") return name;
            if (fn == "mob") return speech.name(target2, args.find("plural") != std::string::npos);
            if (fn == "zone") {
                if (args.find("named") != std::string::npos) return zone;
                std::string kind = speech.name(zoneKind);
                for (auto &ch : kind) ch = (char)std::tolower((unsigned char)ch);
                return kind;
            }
            return "";
        };
        return context;
    };
    // The script a townsperson says now: with [Talk] UseExports=1 the exported one for today,
    // if there is one; else the one the game would build from their record (TalkGen); for
    // people CubeVulkan has no record of, the exported one of the nearest day.
    auto conversationFor = [&](const Speaker &k, int64_t who, TalkScript &out) -> bool {
        bool exact = false;
        const TalkScript *exported = talkScripts.find(who, day, &exact);
        if (useTalkExports && exported && exact) {
            out = *exported;
            return true;
        }
        if (const uint8_t *record = k.native && spawnedReady ? spawned.record(who) : nullptr) {
            talkgen::World world;
            world.day = day;
            world.timeOfDayMs = hour * 3600000.0f;
            world.pets = spawned.petList();
            const talkgen::Speaker speaker = talkgen::speakerOf(record);
            out.id = who, out.race = speaker.race, out.day = day;
            out.speaker = itemNameParts.loaded() ? itemNameParts.name((int)who, speaker.race) : "";
            out.tokens = talkgen::script(speaker, talkPlayer(), world, talkRand);
            return true;
        }
        if (!exported)
            return false;
        if (!exact)
            logLine("Conversation: no script exported for " + exported->speaker + " on day " + std::to_string(day) +
                    "; playing day " + std::to_string(exported->day) + "'s");
        out = *exported;
        return true;
    };
    // Recorded actors retain their preview UI; native actors use npcSpeech.
    auto talkerAt = [&](const Talk &t) { return speakerAt(t.npc); };
    // A realm's name ("realmname"), when the dictionary can say it; empty otherwise.
    auto realmName = [&](uint32_t realm) {
        TalkScript script;
        script.tokens = {"define_realm", std::to_string(talkgen::realmKind(realm)), std::to_string((int32_t)realm), "realmname"};
        std::mt19937 fixed(realm);
        const TalkPlay play = playScript(script, speech, fixed, &talkNames);
        return play.unrendered.empty() && !play.lines.empty() ? play.lines[0] : std::string();
    };
    // A lore cell read (Lore.h, spec 4.1): a used cell, and a piece of its realm's lore told in
    // the realm's colours with the quest sound.
    auto creditLore = [&](lore::Cell cell, bool speakerSound) {
        const lore::Credit credit = lore::credit(cell, usedCells, loreBook);
        if (credit.newlyUsed)
            saveGame.usedCells.push_back(cell);
        if (!credit.newPiece)
            return;
        const std::string name = realmName(credit.realm);
        const std::string line = (name.empty() ? "Lore" : name + " lore") + " increased to " + std::to_string(lore::percent(credit.pieces)) + "%.";
        // (spec: open question: the game adds a chat line, the realm's name in (1, 0.8, 0.5) and
        // the rest in (0.6, 1, 1); CubeVulkan has no chat, so it rises over the player.)
        texts.push_back({player.position + glm::vec3(0, 0, 3.4f), line, glm::u8vec4(153, 255, 255, 255), 0, true});
        if (speakerSound) audio.playInterface(soundrules::soundid::QUEST_PROGRESS);
        logLine("Lore: cell " + std::to_string(cell[0]) + " " + std::to_string(cell[1]) + " of realm " + std::to_string(credit.realm) +
                ", now " + std::to_string(lore::percent(credit.pieces)) + "%");
    };
    // A place a talk marks (spec 4.2): unless its mission is known already, it becomes known and
    // discovered, with a "NEW" on the map for three minutes and the quest sound.
    auto markPlace = [&](const TalkPlay::Marked &m) {
        const std::array<int32_t, 2> place{m.x, m.y};
        if (missionKnown(place))
            return;
        saveGame.marked.push_back({m.x, m.y, m.place, m.item});
        if (discoveredPlaces.insert(place).second)
            saveGame.discoveredPlaces.push_back(place);
        saveGame.missionTimers[place] = mapmarks::TALK_NOTE_MS;
        audio.playInterface(soundrules::soundid::QUEST_PROGRESS);
        logLine("Marked on the map: " + m.place + (m.item.empty() ? "" : " (" + m.item + ")") + ", cell " + std::to_string(m.x) + "," +
                std::to_string(m.y));
    };
    // Recorded-actor preview page activation; native effects use the page consumers below.
    auto pageShown = [&]() {
        if (!talk || talk->effectsPage == talk->line)
            return;
        talk->effectsPage = talk->line;
        if (talk->line < talk->pageMarks.size())
            for (const TalkPlay::Marked &m : talk->pageMarks[talk->line])
                markPlace(m);
    };
    // Lore and receipt scripts append to the general panel, retaining its clock and speaker.
    // The lore cell belongs to the final compiled page (steam-dialogue-item-actions 1.1).
    auto startTale = [&](const Examined &examined) {
        if (examined.reading.tale.empty()) return;
        const lore::Cell cell = examined.reading.cell;
        TalkScript script;
        script.tokens = examined.reading.tale;
        std::mt19937 seeded(worldgen::random((uint32_t)cell[0] + 57u * (uint32_t)cell[1]));
        for (const auto &what : generalDialogue.append(script, speech, seeded, &talkNames, cell))
            logLine("Lore tale: cannot say this in full yet: " + what);
    };
    auto showDialog = [&](const std::vector<std::string> &tokens, uint32_t seed) {
        TalkScript script;
        script.tokens = tokens;
        std::mt19937 seeded(seed);
        for (const auto &what : generalDialogue.append(script, speech, seeded, &talkNames))
            logLine("Dialog: cannot say this in full yet: " + what);
    };
    // A ground item this player picked up, received as the game receives its pick-up notice
    // (loot-pickup.md §4): an item that stays for others only once per character, the region
    // counter, money, coins into the gold counter, the used cell of treasure, the receipt dialog
    // and holding it up. Returns how many of it go on into the bag or gear (collect).
    auto receivePickup = [&](const Loot &l) -> int {
        nativeplayer::Item adapted = Gear::toRecord(l.item);
        nativeplayer::Item noticeItem = adapted;
        noticeItem[0] = (uint8_t)l.noticeType;
        const int64_t position[3] = {(int64_t)std::llround((double)l.at.x * 65536) + extractedWorld.originDotsX(),
                                     (int64_t)std::llround((double)l.at.y * 65536) + extractedWorld.originDotsY(),
                                     (int64_t)std::llround(((double)l.at.z - ExtractedWorld::Z_OFFSET) * 65536)};
        int32_t region[2] = {0, 0};
        uint32_t playerFlags = 0;
        if (const uint8_t *who = spawned.playerRecord()) {
            std::memcpy(region, who + worldgen::creature::REGION, 8);
            std::memcpy(&playerFlags, who + 0x5c, 4); // (the record's flags +0x5c)
        }
        grounditems::Keeper keeper{saveGame.perPlayerTaken, saveGame.regionCounters};
        const auto *playerRecord = spawned.playerRecord();
        float abilityMs = 0;
        if (playerRecord) std::memcpy(&abilityMs, playerRecord + 0x6c, sizeof abilityMs);
        const bool abilityFree = !playerRecord || playerRecord[0x68] == 0 || abilityMs > spawned.playerAbilityDuration();
        const grounditems::ReceiptRegion descriptionRegion{
            worldgen::regionBiome(l.item.region[0], l.item.region[1]),
            (int32_t)worldgen::random((uint32_t)l.item.region[0] + 57u * (uint32_t)l.item.region[1])};
        const auto r = grounditems::receive(keeper, noticeItem, adapted, l.count, l.flags, position, region,
                                          playerFlags, abilityFree, descriptionRegion);
        saveGame.perPlayerTaken = std::move(keeper.perPlayerTaken);
        saveGame.regionCounters = std::move(keeper.regionCounters);
        if (!r.announce)
            return 0; // taken before by this character
        if (nativeInventory) {
            if (r.into == grounditems::Into::Gold)
                playerGold += r.amount;
            else if (r.into == grounditems::Into::Inventory) {
                int quantity = r.amount;
                if (adapted[0] == grounditems::type::COIN && adapted[nativesaves::item::MATERIAL] == grounditems::COIN_MATERIAL)
                    quantity *= std::max(l.gold, 1);
                nativeInventory->inventory.add(adapted, quantity);
            }
            refreshInventory(true);
        }
        if (r.markCell && usedCells.insert(r.cell).second)
            saveGame.usedCells.push_back(r.cell);
        if (!r.dialog.empty()) {
            showDialog(r.dialog, chatter() % 10000);
            if (nativeai::Creature *me = spawned.localPlayer(); me && r.holdUpAbility) {
                useprompt::Scene holder;
                holder.player = me->record;
                holder.abilityDuration = spawned.playerAbilityDuration();
                useprompt::holdUp(holder, adapted, r.holdUpAbility);
            }
        }
        if (nativeInventory)
            return 0; // the complete notice amount was applied once, before refresh/receipt presentation
        const glm::u8vec4 gold{255, 214, 64, 255};
        switch (r.into) {
        case grounditems::Into::RegionCounter:
            logLine("Picked up " + std::to_string(r.amount) + " for the region's counter");
            return 0;
        case grounditems::Into::Gold:
            playerGold += r.amount;
            texts.push_back({player.position + glm::vec3(0, 0, 2.6f), "+" + std::to_string(r.amount) + " GOLD", gold});
            logLine("Picked up a gold bag: " + std::to_string(r.amount) + " gold");
            return 0;
        case grounditems::Into::Inventory:
            if (l.item.category == grounditems::type::COIN && l.item.material == grounditems::COIN_MATERIAL) { // the gold counter
                const int coins = r.amount * std::max(l.gold, 1);
                playerGold += coins;
                texts.push_back({player.position + glm::vec3(0, 0, 2.6f), "+" + std::to_string(coins) + " GOLD", gold});
                logLine("Coins: +" + std::to_string(coins) + " gold");
                return 0;
            }
            return r.amount;
        default:
            return 0;
        }
    };
    auto startTalk = [&](const Speaker &k) {
        const auto *c = personOf(k);
        const int64_t who = speakerId(k);
        if (!who)
            return;
        lastTalkedSpeaker = k;
        if (k.native && npcSpeech.repeatUse(who)) return;
        answers.clear(); // a newly addressed conversation replaces its offered answers
        if (k.native) {
            TalkScript script;
            if (!conversationFor(k, who, script)) return;
            auto play = playScript(script, speech, chatter, &talkNames);
            for (const auto &what : play.unrendered)
                logLine("Conversation: " + script.speaker + " cannot say this in full yet: " + what);
            std::vector<dialoguechoice::Row> rows;
            for (size_t index = 0; index < play.choiceIds.size(); ++index)
                rows.push_back({play.choiceIds[index], play.choiceWords[index]});
            answers.offer(who, std::move(rows));
            std::vector<TalkPlay::Page> pages;
            for (auto &page : play.pages) if (page.choiceId.empty()) pages.push_back(std::move(page));
            const auto speaker = npcspeech::actor(spawned.record(who));
            npcSpeech.attach(who, spawned.playerId(), std::move(pages), speaker.drawn);
            for (const auto &line : play.lines) logLine("Talk: " + line);
            return;
        }
        Talk t;
        t.npc = k;
        auto say = [&](const char *id, const Speech::Context &context) {
            std::string line = speech.say(id, context, chatter);
            if (!line.empty())
                t.lines.push_back(line);
        };
        SaveGame::Quest *mine = nullptr;
        for (auto &q : quests)
            if (q.giver == (unsigned long long)who)
                mine = &q;
        // Where this NPC's creatures are now (the nearest living one), for directions.
        auto nearestOf = [&](const std::string &nameId, glm::vec3 from, int &alive) {
            glm::vec3 best(0);
            float bestDistance = 1e9f;
            alive = 0;
            for (size_t i = 0; i < actors.count(); i++) {
                size_t r = actors.recordingOf(i);
                if (!actors.hittable(i) || r >= creatureNameIds.size() || creatureNameIds[r] != nameId)
                    continue;
                float d = glm::length(actors.position(i) - from);
                if (d < 300)
                    alive++;
                if (d < bestDistance)
                    bestDistance = d, best = actors.position(i);
            }
            return best;
        };
        if (mine && mine->state == 1) { // back with the job done: thanks and a reward
            say("good_work", speakerContext(k, mine, {}));
            say("thank_you", speakerContext(k, mine, {}));
            mine->state = 2;
            int gold = 20 * mine->need * std::max(1, mode == progression::Mode::Pyro ? playerLevel : 1);
            Loot reward;
            reward.at = player.position, reward.age = 1, reward.gold = gold;
            reward.hasItem = rollItem(3, playerLevel, reward.item, reward.slot);
            loot.push_back(reward);
            if (mode == progression::Mode::Pyro)
                playerXp += progression::killXp(3, playerLevel, playerLevel) * mine->need;
            logLine("Quest rewarded: " + std::to_string(gold) + " gold" + (reward.hasItem ? " and an item" : ""));
        } else if (mine && mine->state == 0) { // still to do: a reminder of where
            int alive = 0;
            glm::vec3 where = nearestOf(mine->target, speakerAt(k), alive);
            auto context = speakerContext(k, mine, where);
            say("you_must_defeat_the_enemies_in_zone", context);
            say("the_enemies_are_in_zone_direction_of_here", context);
        } else if (TalkScript script; !mine && conversationFor(k, who, script)) {
            // The game's script for this person, for today (it changes daily).
            TalkPlay play = playScript(script, speech, chatter, &talkNames);
            t.lines = play.lines;
            t.pageMarks = play.lineMarks;
            for (const std::string &what : play.unrendered)
                logLine("Conversation: " + script.speaker + " cannot say this in full yet: " + what);
            std::vector<dialoguechoice::Row> rows;
            for (size_t index = 0; index < play.choiceIds.size(); ++index)
                rows.push_back({play.choiceIds[index], play.choiceWords[index]});
            answers.offer(who, std::move(rows));
            if (!t.lines.empty() || !answers.empty()) {
                if (!t.lines.empty()) { talk = std::move(t); pageShown(); }
                else talk.reset();
                return; // answers are independent of the speaker bubble
            }
        } else if (inventedQuests && c && !mine && c->id % 3 == 0 && std::count_if(quests.begin(), quests.end(), [](auto &q) { return q.state < 2; }) < 3) {
            // A request: the nearest kind of monster in reach, and how many to defeat.
            float bestDistance = 1e9f;
            size_t best = SIZE_MAX;
            for (size_t i = 0; i < actors.count(); i++) {
                size_t r = actors.recordingOf(i);
                float d = glm::length(actors.position(i) - speakerAt(k));
                if (actors.hittable(i) && r < creatureNameIds.size() && !creatureNameIds[r].empty() && d < 400 && d < bestDistance)
                    bestDistance = d, best = i;
            }
            if (best != SIZE_MAX) {
                SaveGame::Quest q;
                q.giver = c->id;
                q.target = creatureNameIds[actors.recordingOf(best)];
                int alive = 0;
                glm::vec3 where = nearestOf(q.target, actors.position(best), alive);
                q.need = std::clamp(alive, 1, 5);
                Place p = placeAt(where.x, where.y);
                q.zone = p.kind + ":" + p.name;
                quests.push_back(q);
                auto context = speakerContext(k, &quests.back(), where);
                say("hello", context);
                say("my_name_is", context);
                say("the_location_is_full_of_creatures", context);
                say("you_must_defeat_the_enemies_in_zone", context);
                say("the_enemies_are_in_zone_direction_of_here", context);
                logLine("Quest taken: defeat " + std::to_string(q.need) + " " + speech.name(q.target, true) + " in " + p.name);
            }
        }
        if (t.lines.empty()) { // small talk, from the game's conversation lines
            auto context = speakerContext(k, nullptr, {});
            say("hello", context);
            say("my_name_is", context);
            int cellX = 0, cellY = 0, region = useExtracted ? extractedWorld.regionTypeAt(speakerAt(k).x, speakerAt(k).y, cellX, cellY) : 0;
            const char *weather = region == 6 || region == 11 ? "its_cold" : region == 9 || region == 10 || region == 13 ? "its_hot"
                                : region == 4 || region == 8 ? "its_warm" : "its_pleasant";
            static const char *classes[] = {"", "so_you_are_a_warrior", "so_you_are_a_ranger", "so_you_are_a_mage", "so_you_are_a_rogue"};
            int cls = std::clamp(chosen ? chosen->classType : 0, 0, 4);
            std::vector<std::string> topics = {"welcome_to_location", "how_do_you_like_our_zone", weather};
            if (cls)
                topics.push_back(classes[cls]);
            for (int i = 1; i <= 9; i++)
                topics.push_back("random:" + std::to_string(i));
            // A topic whose line needs something CubeVulkan cannot name yet (a shop, an
            // item) is passed over for another.
            for (int tries = 0; tries < 12; tries++) {
                context.missing = false;
                std::string line = speech.say(topics[chatter() % topics.size()], context, chatter);
                if (!line.empty() && !context.missing) {
                    t.lines.push_back(line);
                    break;
                }
            }
        }
        t.lines.push_back(speech.say("goodbye", speakerContext(k, nullptr, {}), chatter));
        talk = t;
    };
    // Multiplayer (Multiplayer.h, docs/multiplayer.md). Every game runs its own host world, which
    // decides, and the client world the player sees, a mirror of whichever host it is joined to;
    // single player is the client joined to its own host through the loopback. Players reach each
    // other:
    //   * through Steam, the game's way (unless --offline or [Multiplayer] Steam=0): friends invite
    //     and join from the multiplayer menu (J); --invite friend and --join-steam friend|steamid
    //     do the same from the command line;
    //   * by address over UDP, CubeVulkan only: --host [port], --join address[:port] (or
    //     [Multiplayer] Host=1, Join=address, Port=), and the dedicated --server [port].
    std::set<int64_t> creditedKills; // kill credits naming this player, kept until its death is seen
    if (spawnedReady) {
        // The host world takes the stored zone states and the world's clock; the client world is
        // a mirror from now on.
        hostWorld.setStoredZones(extractedWorld, &saves.world(), false);
        hostWorld.setClock(day, hour * 3600000.0f);
        hostWorld.setClockRate(minutesPerSecond * 60.0); // ([Time] MinutesPerSecond: game ms a real ms)
        spawned.setStoredZones(extractedWorld, nullptr, false);
        spawned.setRemoteClient(true);
        spawned.setClock(day, hour * 3600000.0f);
        const int port = (int)config.number("Multiplayer", "Port", Multiplayer::DEFAULT_PORT, 1, 65535);
        std::string join = joinAddress.empty() ? config.text("Multiplayer", "Join") : joinAddress;
        const bool byAddress = serverMode ? !hostSteam : (hostPort > 0 || !join.empty() || config.flag("Multiplayer", "Host", false));
        Multiplayer::Setup setup;
        setup.steam = &steam;
        setup.hostWorld = &hostWorld;
        setup.clientWorld = serverMode ? nullptr : &spawned;
        setup.blocks = &extractedWorld;
        setup.loss = netLoss;
        if (byAddress) {
            setup.way = Multiplayer::Way::Address;
            setup.port = (uint16_t)(hostPort > 0 ? hostPort : join.empty() ? port : 0);
        } else if (!offline && config.flag("Multiplayer", "Steam", true))
            setup.way = Multiplayer::Way::Steam;
        std::string why;
        if (!session.start(setup, why)) {
            logLine("Multiplayer: " + why + "; playing alone");
            setup.way = Multiplayer::Way::Local;
            session.start(setup, why);
        }
        switch (session.way()) {
        case Multiplayer::Way::Steam:
            logLine("Multiplayer: through Steam as " + steam.personaName() + "; friends invite and join with the multiplayer menu");
            break;
        case Multiplayer::Way::Address:
            logLine("Multiplayer: by address, UDP port " + std::to_string(setup.port ? setup.port : 0) + (serverMode ? " (dedicated server)" : ""));
            break;
        default: logLine("Multiplayer: none (single player through the loopback)"); break;
        }
        if (!join.empty() && session.way() == Multiplayer::Way::Address && !serverMode) {
            std::string address = join;
            int joinPort = port;
            if (auto colon = join.rfind(':'); colon != std::string::npos)
                address = join.substr(0, colon), joinPort = std::atoi(join.c_str() + colon + 1);
            if (session.joinAddress(address, (uint16_t)joinPort, why))
                logLine("Multiplayer: joining " + address + ":" + std::to_string(joinPort));
            else
                logLine("Multiplayer: cannot join (" + why + ")");
        }
        if (!joinSteam.empty() && session.way() == Multiplayer::Way::Steam) {
            const bool number = std::all_of(joinSteam.begin(), joinSteam.end(), [](char c) { return std::isdigit((unsigned char)c); });
            uint64_t id = number ? std::strtoull(joinSteam.c_str(), nullptr, 10) : 0;
            for (uint64_t f : steam.friendIds())
                if (!id && steam.friendName(f) == joinSteam)
                    id = f;
            if (id) {
                session.join(id);
                logLine("Multiplayer: joining " + joinSteam + " through Steam");
            } else
                logLine("Multiplayer: no Steam friend " + joinSteam);
        }
        for (const std::string &name : invites)
            if (!session.invite(name))
                logLine("Multiplayer: cannot invite " + name + " (Steam friends only)");
        // What the client world tells the game frame.
        if (MirrorSide *mirror = session.mirrorSide()) {
            mirror->damageShown = [&](const cwnet::DamageRecord &number) { // a hit someone else made
                float damage;
                int64_t target, at[3];
                std::memcpy(&damage, number.data() + 0x10, 4);
                std::memcpy(&target, number.data() + 0x08, 8);
                std::memcpy(at, number.data() + 0x20, sizeof at);
                if (std::lround(damage) != 0)
                    floating.push_back({extractedWorld.toLocal(at) + glm::vec3(0, 0, 2.4f), (int)std::lround(damage), 0,
                                        target == spawned.playerId()});
            };
            mirror->featDone = [&](const cwnet::FeatRecord &) { ++pendingFeatDisplays; };
            mirror->dayChanged = [&](int32_t newDay) {
                resetDailyProgress();
                logLine("World: day " + std::to_string(newDay));
            };
            mirror->startedOver = [&] { // the ground items shown are the new host's from now on
                zoneItemsLoaded.clear();
                loot.erase(std::remove_if(loot.begin(), loot.end(), [](const Loot &l) { return l.zoneItem[0] != INT_MIN; }), loot.end());
            };
        }
        if (HostSide *host = session.hostSide()) {
            host->standIn = player.position;
            host->deaths = [&](const std::vector<SpawnedCreatures::Death> &died) { // the world remembers who died
                for (const SpawnedCreatures::Death &d : died) {
                    events.death(d.id);
                    if (std::find(saveGame.killed.begin(), saveGame.killed.end(), d.id) == saveGame.killed.end())
                        saveGame.killed.push_back(d.id);
                }
            };
            host->reset = [&] {
                resetDailyProgress();
                logLine("World: daily reset");
            };
        }
    } else
        logLine("Multiplayer: none (it needs the game's own creatures)");
    double sinceHurt = 99;
    bool attackLanded = false;
    int struck = -1; // last monster struck, for the title bar
    std::mt19937 dice(std::random_device{}());
    glm::vec3 respawn = player.position; // where this world was entered (the native player: its last shrine)
    float heading = resumed ? saveGame.heading : player.yaw + 3.14159265f + turn; // Facing away from the camera.
    // Writes the character's progress to Save/characters.db and the world's to Save/world.db.
    // Returns where the progress was written ("" when nothing could be).
    auto saveProgress = [&]() -> std::string {
        if (!saving)
            return "";
        saveGame.world = worldName;
        saveGame.hasPosition = true;
        saveGame.position = player.position;
        saveGame.yaw = player.yaw;
        saveGame.heading = heading;
        saveGame.hpFraction = playerMaxHp > 0 ? std::clamp(playerHp / playerMaxHp, .01f, 1.f) : 1;
        if (haveGear)
            saveGame.takeGear(gear);
        saveGame.level = playerLevel;
        saveGame.xp = playerXp;
        saveGame.gold = playerGold;
        saveGame.hour = hour;
        saveGame.day = day;
        saveGame.quests = quests;
        if (spawnedReady && !session.guestElsewhere()) { // the world's dead are its host world's
            const auto persistentDead = hostWorld.persistentDeadSet();
            saveGame.killed.assign(persistentDead.begin(), persistentDead.end());
            saves.world().dead = persistentDead;
        } else if (!spawnedReady && enemyRespawns.mode != enemyrespawn::Mode::Permanent)
            saveGame.killed.clear();
        saveGame.bag.clear();
        for (const Carried &c : bag) {
            SaveGame::Bagged b;
            b.category = c.item.category, b.id = c.item.id, b.material = c.item.material, b.rarity = c.item.rarity;
            b.modifier = c.item.modifier, b.count = c.count, b.region[0] = c.item.region[0], b.region[1] = c.item.region[1];
            b.formula = c.item.formula;
            b.storedRecord = Gear::toRecord(c.item);
            saveGame.bag.push_back(b);
        }
        saveGame.found.clear();
        for (size_t i = 0; i < gear.itemCount(); i++) {
            const Gear::Item &it = gear.item(i);
            if (it.found)
            {
                saveGame.found.push_back({it.category, it.id, it.material, it.rarity, it.level, (int)gear.slotOf(i),
                                          it.modifier, it.sprite, it.adds.maxHp, it.adds.attack, it.adds.armor, it.adds.critical});
                saveGame.found.back().region[0] = it.region[0], saveGame.found.back().region[1] = it.region[1];
                saveGame.found.back().cubes = it.cubes, saveGame.found.back().cubeCount = it.cubeCount;
                saveGame.found.back().formula = it.formula;
                saveGame.found.back().storedRecord = Gear::toRecord(it);
            }
        }
        saveGame.savedAt = (long long)std::time(nullptr);
        // A guest's own world is left alone: only its character is written (netcode-roles.md §7).
        if (!saves.save(saveGame, haveGear ? &gear : nullptr, std::max(0.0f, playerHp), saveFrame, !session.guestElsewhere())) {
            logLine("Save: could not write " + utf8(saves.folder()));
            return "";
        }
        return "character " + std::to_string(saves.slot()) + " of " + utf8(saves.folder() / "characters.db");
    };
    auto lastSave = std::chrono::steady_clock::now();
    double motionTime = 0, walked = 0;
    auto motion = CharacterRecording::Motion::Idle;
    // Combat moves, played with the game's recorded animations: the character's
    // class and specialization pick the game's ability ids (ClassMoves.h). Moves
    // missing from this character's recording are left out. Buttons follow Steam
    // Cube World's default controls.
    ClassMoves moves = chosen ? classMoves(chosen->classType, chosen->specialization) : classMoves(4, 0);
    for (int *id : {&moves.attack[0], &moves.attack[1], &moves.special, &moves.releaseSpecial, &moves.skill, &moves.tap, &moves.ultimate, &moves.dodge})
        if (*id && character.abilityStart(*id) < 0)
            *id = 0;
    if (!moves.attack[1])
        moves.attack[1] = moves.attack[0];
    logLine("Moves: attack " + std::to_string(moves.attack[0]) + "/" + std::to_string(moves.attack[1]) + ", special " +
            std::to_string(moves.special) + ", skill " + std::to_string(moves.skill) + ", P " + std::to_string(moves.tap) +
            ", R " + std::to_string(moves.ultimate) + ", dodge " + std::to_string(moves.dodge) + " (0 = not recorded)");
    int ATTACK_A = moves.attack[0], ATTACK_B = moves.attack[1];
    auto isAttack = [&](int id) { return id && (id == ATTACK_A || id == ATTACK_B); };
    auto dealsDamage = [&](int id) { return isAttack(id) || (id && id == moves.special && !moves.releaseSpecial) ||
                                                 (id && id == moves.releaseSpecial); };
    int ability = 0, lastAttack = ATTACK_B;
    bool attackQueued = false;
    double abilityTime = 0, sinceAttack = 99;
    Walk walk; // the player's walking speed, as the game changes it (Body.h)
    double rightDownAt = -1, rightDownX = 0, rightDownY = 0; // a right-click tap is the special attack
    std::vector<int> movesPressed; // keys pressed this frame, for the class moves
    // Mana and stamina (0..1). Skills that cost more than is left are refused, as
    // the game refuses them.
    float playerMp = 0, playerStamina = 1;
    // Blocking (Combat.h): block power (-1..1) is spent by hits taken while guarding.
    float playerBlock = 1;
    const int playerCls = chosen ? chosen->classType : 4;
    int playerSpec = saveGame.specialization >= 0 ? saveGame.specialization : (chosen ? chosen->specialization : 0);
    auto held = [&](Gear::Slot s) {
        const Gear::Item &it = gear.equipped(s);
        return haveGear ? combat::HeldItem{it.category, it.id, it.rarity, it.modifier} : combat::HeldItem{};
    };
    auto guardedDefense = [&]() {
        combat::Defender d = playerDefense();
        d.block = &playerBlock;
        d.ability = ability;
        d.guarding = combat::guarding(ability, playerCls, playerSpec, playerStunned > 0);
        d.blockFactor = combat::blockFactor(held(Gear::WeaponLeft), held(Gear::WeaponRight), playerCls, playerSpec, 0);
        d.mana = &playerMp;
        d.manaGeneration = character.manaGeneration;
        return d;
    };
    // A generated creature's hit on the player (its damage from the game's ability step,
    // 0x2c5250): the target's side of 0x29eb60 as Combat.h has it for CubeVulkan's player.
    // Blocked, it does not land; a mana shield that takes it all still counts as landed.
    size_t spawnedHits = 0, spawnedBlocked = 0; // for the log
    float spawnedDamage = 0;
    spawned.setPlayerHit([&](const nativeupkeep::Hit &h, const uint8_t *from) {
        combat::Attacker attacker;
        attacker.ability = h.ability;
        attacker.charge = h.charge;
        unsigned flags2 = 0;
        if (from) {
            std::memcpy(&attacker.combo, from + 0x70, 4);
            attacker.maxCombo = nativerules::comboThreshold(from);
            std::memcpy(&flags2, from + 0x80, 4);
        }
        const combat::Hit got = combat::takeHit(h.damage, attacker, guardedDefense());
        if (got.blocked) {
            spawnedBlocked++;
            return false;
        }
        spawnedHits++, spawnedDamage += got.damage;
        if (got.damage > 0) {
            playerHp -= got.damage;
            floating.push_back({player.position + glm::vec3(0, 0, 2.6f), (int)std::round(got.damage), 0, true});
            sinceHurt = 0;
        }
        const float chance = combat::stunChance(h.ability, h.charge, false, flags2, 0);
        if (playerStunned <= 0 && sinceStunned >= 3 && !playerEffects.find(combat::BULWARK) &&
            (float)combat::nativeRand(dice) / 32767.0f < chance)
            playerStunned = combat::stunSeconds(true), ability = 0;
        return true;
    });
    // In the client world a monster's hit on the player comes from the host as a damage number
    // (netcode-roles.md §8 step 11); CubeVulkan's own player (not the native one) takes its
    // damage unless it was blocked.
    spawned.setPlayerNumber([&](const std::array<uint8_t, 0x48> &number) {
        constexpr size_t DAMAGE = 0x10, KIND = 0x45;
        constexpr uint8_t BLOCKED = 1;
        float damage;
        std::memcpy(&damage, number.data() + DAMAGE, 4);
        if (number[KIND] == BLOCKED || !(damage > 0.0f)) {
            spawnedBlocked += number[KIND] == BLOCKED;
            return;
        }
        spawnedHits++, spawnedDamage += damage;
        playerHp -= damage;
        sinceHurt = 0;
    });
    auto pay = [&](float mp, float stamina) {
        if (mp < 0 ? playerMp < .01f : playerMp + 1e-4f < mp)
            return false;
        if (playerStamina + 1e-4f < stamina)
            return false;
        playerMp = mp < 0 ? 0 : playerMp - mp;
        playerStamina -= stamina;
        return true;
    };
    auto startMove = [&](int id) {
        if (nativePlayer && !player.fly)
            return; // the game's own input (NativeControl) starts the native player's moves
        if (!id || (ability && !isAttack(ability)) || playerStunned > 0)
            return; // a move plays to its end before the next; a stunned player does nothing
        float mpBefore = playerMp;
        bool charged = id == moves.special && moves.releaseSpecial; // its release pays
        float mp = id == moves.tap ? moves.tapMp : (id == moves.special && !charged) || id == moves.releaseSpecial ? moves.specialMp : 0;
        float stamina = id == moves.tap ? moves.tapStamina : id == moves.skill ? moves.skillStamina : id == moves.dodge ? moves.dodgeStamina : 0;
        if ((mp || stamina) && !pay(mp, stamina)) {
            logLine(std::string("Not enough ") + (playerStamina + 1e-4f < stamina ? "stamina" : "mana") + " for ability " + std::to_string(id));
            return;
        }
        ability = id;
        abilityTime = 0;
        // A released charge holds the mana it spent (the game's +0xba8).
        playerCharge = id == moves.releaseSpecial ? combat::releasedCharge(id, mpBefore - playerMp) : 0;
        combat::Active effect;
        if (combat::skillEffect(id, playerAttack * spellPerAttack, effect))
            playerEffects.add(effect.type, effect.value, effect.left);
        if (!isAttack(id)) attackQueued = false;
        attackLanded = false;
        if (dealsDamage(id))
            heading = player.yaw + 3.14159265f; // Attacks face where the camera looks.
    };
    std::vector<glm::mat4> pose;

    bool shadowsEnabled = renderSettings.shadows;
    double lastX = 0, lastY = 0;
    const bool mouseLook = capture.empty() && captureSpeech.empty() && captureFeat.empty() && config.number("Input", "MouseLook", 1, 0, 1) != 0;
    glfwGetCursorPos(window, &lastX, &lastY);
    auto pointerPosition = [&](double *x, double *y) {
        if (input.pointerSampled)
            *x = input.pointerX, *y = input.pointerY;
        else
            glfwGetCursorPos(window, x, y);
    };
    auto start = std::chrono::steady_clock::now(), previous = start, titleTime = start;
    // Sounds (sound.md): the world clock the ambient emitters tick by (world +0x440, ms) and
    // each zone's emitters, read once.
    double worldClockMs = 0;
    std::map<std::pair<int32_t, int32_t>, std::vector<soundrules::Emitter>> zoneEmitters;
    // A local point in world units (1/65536 block), where the game's sounds are placed.
    auto toUnits = [&](glm::vec3 local, int64_t out[3]) {
        out[0] = (int64_t)std::llround((double)local.x * 65536) + extractedWorld.originDotsX();
        out[1] = (int64_t)std::llround((double)local.y * 65536) + extractedWorld.originDotsY();
        out[2] = (int64_t)std::llround(((double)local.z - ExtractedWorld::Z_OFFSET) * 65536);
    };
    // The player's position as the world keeps it: the native player's record, else CubeVulkan's.
    auto playerUnits = [&](int64_t out[3]) {
        if (const uint8_t *me = spawned.playerRecord())
            std::memcpy(out, me + worldgen::creature::POSITION, 3 * sizeof(int64_t));
        else
            toUnits(player.position, out);
    };
    auto clientRand = [] { return std::rand(); }; // the C runtime's rand, as the game's client draws
    unsigned framesSinceTitle = 0, settledFrames = 0;
    double nextSteamCheck = 5;
    int exitCode = 0;
    bool returnToTitle = false;
    std::vector<DrawItem> opaque, water;
    // The screens over the game (Frontend.h): the system menu and the options, controls and
    // help screens, drawn with the game's own interface while the world runs on behind them.
    frontend::Services inGameServices = front;
    inGameServices.externalMenuSounds = nativeInventory != nullptr;
    inGameServices.hud = hudReady ? &hud : nullptr;
    inGameServices.spawned = nullptr; // (they leave the player alone)
    // The multiplayer menu (J): the Steam friends, or by address the players on this host.
    frontend::FriendsMenu friendsMenu;
    friendsMenu.list = [&] {
        std::vector<frontend::FriendRow> rows;
        for (const Multiplayer::FriendRow &f : session.friends())
            rows.push_back({f.id, f.name, f.hostOfMine, f.joined, f.invited, f.invitation, f.eligible});
        return rows;
    };
    friendsMenu.hostButton = [&](uint64_t id) { session.hostButton(id); };
    friendsMenu.guestButton = [&](uint64_t id) { session.guestButton(id); };
    friendsMenu.emptyText = session.way() == Multiplayer::Way::Steam ? "No Steam friends to show"
                            : session.way() == Multiplayer::Way::Address ? "No players connected"
                                                                         : "Playing alone (no Steam networking)";
    inGameServices.friends = &friendsMenu;
    frontend::Request inGameRequest;
    inGameRequest.readOnly = !savesWritable;
    frontend::InGameMenus menus(inGameServices, inGameRequest);
    bool menusReady = false;
    if (!serverMode) {
        std::string why;
        menusReady = menus.start(why);
        if (!menusReady)
            logLine("Menu: " + why);
        else if (openSystemMenu)
            menus.toggleSystemMenu();
    }
    // The game's controls (controls.cfg), read each frame.
    GameInput gameInput;
    // The camera as drawn follows the angles and distance the mouse sets, smoothed by the
    // cameraSmoothness option (frontend-title.md §3.3).
    float viewYaw = player.yaw, viewPitch = player.pitch, viewZoom = player.zoom;

    auto submitChat = [&](const std::u16string &line) {
        const auto command = chatcommand::parse(line);
        auto feedback = [&](const std::u16string &text) { hud.appendChat(chat, text + u"\n", {255, 127, 127}); };
        switch (command.kind) {
        case chatcommand::Kind::Check:
            feedback(std::u16string(u"multiplayerWidget->checkGame = ") + (session.toggleGameCheck() ? u"1" : u"0"));
            break;
        case chatcommand::Kind::NamePet:
            if (command.argument.size() >= 16) feedback(u"Name is too long.");
            else if (auto *me = spawned.localPlayer(); me && me->record) {
                auto *item = me->record + 0x8b8;
                if (chatcommand::namePet(item, command.argument) && gear.equipped(Gear::Pet).category == 20) {
                    auto pet = gear.equipped(Gear::Pet);
                    Gear::cubesFrom(item, pet);
                    gear.updateWorn(Gear::Pet, pet);
                    if (nativeInventory)
                        nativeInventory->equipment[Gear::Pet] = Gear::toRecord(pet);
                }
            }
            break;
        case chatcommand::Kind::Motion:
            if (auto *me = spawned.localPlayer())
                chatcommand::startMotion(me->record, spawned.playerAbilityDuration(), command.ability);
            break;
        case chatcommand::Kind::Text:
            if (session.started()) session.say(line);
            else chat.receive(std::u16string(character.name.begin(), character.name.end()), line);
            break;
        }
    };

    while (!glfwWindowShouldClose(window)) {
        input.gamepadSampled = false;
        input.pointerSampled = false;
        glfwPollEvents();
        if (std::chrono::steady_clock::now() - lastSave > std::chrono::seconds(60)) {
            saveProgress();
            lastSave = std::chrono::steady_clock::now();
        }
        auto now = std::chrono::steady_clock::now();
        const double frameMs = std::chrono::duration<double, std::milli>(now - previous).count();
        double dt = std::min(.1, frameMs / 1000.0);
        double time = std::chrono::duration<double>(now - start).count();
        previous = now;
        steam.pump();
        if (time >= nextSteamCheck) {
            if (!steam.check(steamError)) {
                fatal(steamError);
                exitCode = 1;
                break;
            }
            nextSteamCheck = time + 5;
        }

        // The screens over the game take their keys first (the interface keys F1 to F6 always,
        // everything while one is open). Start Menu and Exit Game end the session.
        const bool panelsBeforeInput = inventoryOpen || mapOpen || answers.visible() || (menusReady && menus.open());
        const bool capturingBeforeInput = menusReady && menus.capturingControl();
        input.gamepadModal = panelsBeforeInput || chat.typing();
        input.gamepadMapMode = mapOpen;
        input.gamepadPanels = {};
        if (inventoryOpen && hud.hasPanels()) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            const auto size = renderer.screenSize();
            const glm::vec2 point((float)(x * size.width / std::max(width, 1)),
                                  (float)(y * size.height / std::max(height, 1)));
            input.gamepadPanels = {hud.inventoryPanelEligible(point, inventoryView, 0),
                                   hud.inventoryPanelEligible(point, inventoryView, 2),
                                   hud.inventoryPanelEligible(point, inventoryView, 1)};
        }
        input.sampleGamepad(window, capturingBeforeInput, &gameControls);
        if (menusReady)
            switch (menus.frame(dt * 1000.0)) {
            case frontend::Result::Title:
                returnToTitle = true;
                glfwSetWindowShouldClose(window, 1);
                break;
            case frontend::Result::Quit: glfwSetWindowShouldClose(window, 1); break;
            default: break;
            }
        bool menusOpen = menusReady && menus.open();
        std::vector<int> pressed = input.pressed;
        input.pressed.clear();
        // Preserve callback order: a character before Enter belongs to that send;
        // a mouse-down later in the same frame must not erase retained editor text.
        // Binding capture completes in the later polling phase; its entry wait
        // blocks these queued keys. Ordinary GUI-edit blur has a different order.
        const bool chatEligible = !session.dedicated() && !capturingBeforeInput &&
            (!menusReady || (menus.interfaceShown() && !menus.capturingControl()));
        for (const auto &event : chatInput) {
            if (event.kind == ChatInputEvent::MouseDown) { chat.stopEditing(); continue; }
            // Character insertion has no root-visibility/binding-wait gate.
            // This runtime has no focused gameplay text editor besides chat.
            if (event.kind == ChatInputEvent::Character) {
                chat.type(std::string(1, (char)event.value));
                continue;
            }
            if (!chatEligible) continue;
            const int key = event.value;
            bool consumed = chat.typing() && key != GLFW_KEY_ESCAPE;
            if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                consumed = true;
                if (auto line = chat.enter()) submitChat(*line);
            } else if (chat.typing()) switch (key) {
            case GLFW_KEY_LEFT: chat.key(ChatLine::Key::Left, event.shift); break;
            case GLFW_KEY_RIGHT: chat.key(ChatLine::Key::Right, event.shift); break;
            case GLFW_KEY_HOME: chat.key(ChatLine::Key::Home, event.shift); break;
            case GLFW_KEY_END: chat.key(ChatLine::Key::End, event.shift); break;
            case GLFW_KEY_DELETE: chat.key(ChatLine::Key::Delete); break;
            case GLFW_KEY_BACKSPACE: chat.backspace(); break;
            default: break;
            }
            if (consumed && !event.repeat)
                if (const auto found = std::find(pressed.begin(), pressed.end(), key); found != pressed.end()) pressed.erase(found);
        }
        chatInput.clear();
        input.typed.clear();
        gameInput.update(window, gameControls, pressed, input.buttons, !menusOpen && !chat.typing(),
                         panelsBeforeInput || menusOpen || chat.typing(), mapOpen,
                         capturingBeforeInput || (menusReady && menus.capturingControl()),
                         input.gamepadPanels.any(), &input.gamepad);
        if (!input.buttons.empty())
            identify.visibleAtDown = identify.open;
        if (std::any_of(input.buttons.begin(), input.buttons.end(), [](int button) { return button == 0 || button == 1; }))
            itemSelector.hide();
        if (gameInput.gamepad().dismiss) {
            inventoryOpen = craftingOpen = mapOpen = false;
            bagShown = true;
            if (menusReady)
                menus.dismiss();
            menusOpen = false;
        }
        // Escape and the System Menu binding (default O; Start on a gamepad) close the inventory
        // first, then the map, then show the system menu.
        auto escape = [&] {
            if (mapOpen && !inventoryOpen)
                mapOpen = false;
            else if (inventoryOpen)
                inventoryOpen = craftingOpen = false, bagShown = true;
            else if (menusReady)
                menus.toggleSystemMenu();
        };
        if (gameInput.pressed(gameoptions::SYSTEM_MENU))
            escape();
        if (gameInput.pressed(gameoptions::MULTIPLAYER_MENU) && menusReady) // (J: the friends list)
            menus.toggleMultiplayerMenu();
        if (openFriendsAt >= 0 && time >= openFriendsAt && menusReady) { // (testing)
            menus.toggleMultiplayerMenu();
            openFriendsAt = -1;
        }
        if (gameInput.pressed(gameoptions::INVENTORY)) {
            bagShown = !(inventoryOpen && bagShown);
            inventoryOpen = (bagShown || craftingOpen) && hud.hasInventoryArt() && haveGear;
            if (inventoryOpen && bagShown)
                refreshBagOpening();
            if (!inventoryOpen)
                craftingOpen = false, bagShown = true;
        }
        if (gameInput.pressed(gameoptions::WORLD_MAP)) // the map toggle
            mapOpen = !mapOpen;
        if (gameInput.pressed(gameoptions::CRAFTING_MENU)) { // the crafting toggle
            const bool wasOpen = inventoryOpen;
            craftingOpen = !(inventoryOpen && craftingOpen);
            if (!wasOpen)
                bagShown = false;
            inventoryOpen = (bagShown || craftingOpen) && hud.hasInventoryArt() && haveGear;
            if (nativeInventory && !nativeInventory->inventory.tabs.empty())
                refreshInventory();
            if (craftingOpen)
                refreshInventory();
            if (!inventoryOpen)
                craftingOpen = false, bagShown = true;
        }
        for (int key : pressed) {
            if (mods.key(key))
                continue;
            // CubeVulkan's own keys give way to a binding on the same key.
            if (GameInput::keyIsBound(gameControls, key))
                continue;
            switch (key) {
            case GLFW_KEY_ESCAPE:
                escape();
                break;
            case GLFW_KEY_F7: // Same key as the SteamShadows mod.
                shadowsEnabled = !shadowsEnabled;
                renderer.settings.shadows = shadowsEnabled;
                break;
            case GLFW_KEY_F8: renderer.settings.wind = !renderer.settings.wind; break;
            case GLFW_KEY_F9: // TAA, as the Steam TemporalAA mod's key.
                renderer.settings.taa = !renderer.settings.taa;
                logLine(renderer.settings.taa ? "TAA on" : "TAA off");
                break;
            case GLFW_KEY_F10:
                renderer.settings.smaa = !renderer.settings.smaa;
                logLine(renderer.settings.smaa ? "SMAA on" : "SMAA off");
                break;
            case GLFW_KEY_F11: player.fly = !player.fly; break; // (testing: F is the lamp)
            case GLFW_KEY_1: // Swap the right-hand weapon among those the game measured.
                if (!nativeInventory && haveGear && gear.cycle(Gear::WeaponRight, [&](const Gear::Item &it) {
                        return (catalog.mayUse(it.category, it.id, it.material, gear.playerClass()) && levelAllows(it));
                    })) {
                    applyGear();
                    logLine("Right hand: " + gear.describe(gear.equipped(Gear::WeaponRight)));
                }
                break;
            case GLFW_KEY_PAGE_UP: hour = std::fmod(hour + 1, 24.f); break; // (testing: T calls the pet)
            case GLFW_KEY_PAGE_DOWN: hour = std::fmod(hour + 23, 24.f); break;
            case GLFW_KEY_F12: {
                auto name = root / ("CubeVulkan-" + std::to_string((long long)time) + ".png");
                renderer.capture(utf8(name));
                logLine("Screenshot: " + utf8(name));
                break;
            }
            }
        }
        movesPressed = menusOpen ? std::vector<int>{} : pressed;

        // Native-style mouse look leaves right mouse free to start a special
        // immediately. Menus/inventory release the pointer for interaction.
        // (in the game's menus the game draws its own pointer, cursor.plx)
        int cursorMode = menusOpen ? GLFW_CURSOR_NORMAL
                         : mouseLook && !inventoryOpen && !mapOpen && !answers.visible() ? GLFW_CURSOR_DISABLED
                         : hud.hasPointer() && (inventoryOpen || mapOpen || answers.visible()) ? GLFW_CURSOR_HIDDEN
                                                                          : GLFW_CURSOR_NORMAL;
        if (glfwGetInputMode(window, GLFW_CURSOR) != cursorMode) {
            glfwSetInputMode(window, GLFW_CURSOR, cursorMode);
            pointerPosition(&lastX, &lastY);
        }
        double mx, my;
        pointerPosition(&mx, &my);
        mapSelection.move({mx - lastX, my - lastY},
                          !input.lastUsedPad && (inventoryOpen || mapOpen || menusOpen || chat.typing() || answers.visible()));
        const bool answerDragLook = answers.visible() && !input.lastUsedPad &&
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
            !hud.dialogueChoicesHit({(float)mx, (float)my}) &&
            !(generalDialogue.visible() && hud.generalDialogueHit({(float)mx, (float)my}));
        if (mapOpen && !input.lastUsedPad && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // the pointer movement: dragging with the right button moves the map, 2^zoom blocks a
            // pixel, along the camera's turn.
            const glm::dvec2 right(-std::sin(player.yaw), std::cos(player.yaw)), up(-std::cos(player.yaw), -std::sin(player.yaw));
            const double unit = std::pow(2.0, mapZoom);
            const glm::dvec2 drag = right * (mx - lastX) * unit + up * -(my - lastY) * unit;
            if (drag != glm::dvec2(0)) {
                targetMapPosition.x += drag.x, targetMapPosition.y += drag.y;
                mapMoved = true;
            }
        } else if (!inventoryOpen && !mapOpen && !menusOpen && glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
                   (answers.visible() ? answerDragLook
                                      : (mouseLook || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS))) {
            // The look input, scaled by the camera speed option and turned over by invert-Y.
            constexpr float RADIANS_A_PIXEL = .005f;
            const float speed = RADIANS_A_PIXEL * gameOptions.lookSpeed(), vertical = gameOptions.invertY ? -1.0f : 1.0f;
            player.yaw -= float(mx - lastX) * speed;
            player.pitch = std::clamp(player.pitch + float(my - lastY) * speed * vertical, -.2f, 1.5f);
        }
        lastX = mx;
        lastY = my;
        const GamepadCamera padTurn = gamepadCamera(gameInput.gamepad().look, frameMs);
        constexpr double PAD_RADIANS = 0.017453292519943295;
        if (upgrade.open) {
            upgradeView.yawDegrees = (float)((double)upgradeView.yawDegrees + padTurn.yaw);
            upgradeView.pitchDegrees = (float)((double)upgradeView.pitchDegrees + padTurn.pitch);
        } else {
            player.yaw = (float)((double)player.yaw - padTurn.yaw * PAD_RADIANS);
            player.pitch = (float)((double)player.pitch - padTurn.pitch * PAD_RADIANS);
        }
        static double inventoryWheel = 0; // (the wheel scrolls the inventory while it is open)
        if (inventoryOpen)
            inventoryWheel += input.scroll;
        else if (gameInput.held(gameoptions::WALK)) // the walk key and the wheel: the camera's distance
            player.zoom = std::clamp(player.zoom * std::pow(.88f, (float)input.scroll), 3.f, 90.f);
        else if (input.scroll != 0) { // the mouse wheel handling: the map's zoom, about the cursor
            const int oldZoom = mapZoom;
            mapZoom = std::clamp(mapZoom - (int)input.scroll, 0, worldmap::MAX_ZOOM);
            if (mapZoom != oldZoom && mapCursorKnown) {
                const glm::dvec2 offset = (glm::dvec2(mapCursor) - glm::dvec2(targetMapPosition)) * (mapZoom < oldZoom ? 0.5 : -1.0);
                targetMapPosition.x += offset.x, targetMapPosition.y += offset.y;
                targetMapPosition.z = groundOf(targetMapPosition.x, targetMapPosition.y);
                mapPosition = targetMapPosition;
                mapMoved = true;
            }
        }
        // the mouse handling on the open map: the middle button marks the cursor's spot
        // with a star (or takes the star there away); two presses within 200 ms zoom in on it.
        {
            static double lastButtonDown = -1;
            for (int button : input.buttons) {
                mapSelection.down();
                if (button == GLFW_MOUSE_BUTTON_MIDDLE && mapOpen && mapCursorKnown && mapCursor.z >= 0) {
                    const std::array<int32_t, 3> spot = {(int32_t)mapCursor.x, (int32_t)mapCursor.y,
                                                         (int32_t)std::max(0.0, groundOf(mapCursor.x, mapCursor.y))};
                    auto same = std::find_if(saveGame.treasures.begin(), saveGame.treasures.end(),
                                             [&](const auto &t) { return t[0] == spot[0] && t[1] == spot[1]; });
                    if (same == saveGame.treasures.end())
                        saveGame.treasures.push_back(spot);
                    else
                        saveGame.treasures.erase(same);
                }
                if (button == GLFW_MOUSE_BUTTON_LEFT && mapOpen && flightSelected &&
                    glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
                    if (boughtFlights.count(selectedFlightPoint)) { // off on the eagle
                        if (spawned.startFlight(selectedFlightPoint[0], selectedFlightPoint[1])) {
                            mapOpen = false;
                            logLine("Flight: to " + std::to_string(selectedFlightPoint[0]) + " " + std::to_string(selectedFlightPoint[1]));
                        }
                    } else if (const int price = flightPrice(selectedFlightPoint); playerGold < price) {
                        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), "Not enough money.", {255, 51, 51, 255}});
                        audio.playInterface(soundrules::soundid::HUMAN_VOICE);
                    } else {
                        playerGold -= price;
                        boughtFlights.insert(selectedFlightPoint);
                        saveGame.flightPoints.push_back(selectedFlightPoint);
                        audio.playInterface(soundrules::soundid::DROP_COIN);
                        logLine("Flight: bought the flight point at " + std::to_string(selectedFlightPoint[0]) + " " +
                                std::to_string(selectedFlightPoint[1]) + " for " + std::to_string(price) + " gold");
                    }
                }
                const double elapsed = time - lastButtonDown;
                lastButtonDown = time;
                if (elapsed < 0.2 && mapOpen && mapCursorKnown && mapCursor.z > 0) {
                    targetMapPosition = mapCursor;
                    targetMapPosition.z = groundOf(targetMapPosition.x, targetMapPosition.y);
                    mapZoom = std::max(mapZoom - 1, 0);
                    mapMoved = true;
                }
            }
        }
        player.yaw += spin * (float)dt;
        input.scroll = 0;
        { // The Zoom In and Zoom Out controls (unbound by default; the d-pad on a gamepad).
            constexpr float ZOOM_PER_SECOND = 2.0f; // (CubeVulkan's: how fast the bindings zoom is not given)
            if (gameInput.held(gameoptions::ZOOM_IN))
                player.zoom = std::clamp(player.zoom / std::pow(ZOOM_PER_SECOND, (float)dt), 3.f, 90.f);
            if (gameInput.held(gameoptions::ZOOM_OUT))
                player.zoom = std::clamp(player.zoom * std::pow(ZOOM_PER_SECOND, (float)dt), 3.f, 90.f);
        }

        // The native player: the game's controller reads the keys (its game-object fields)
        // and the world's update moves it, at the creatures' step below; here it is where
        // the last step left it.
        const bool nativeMove = nativePlayer && !player.fly;
        glm::vec3 before = player.position;
        if (nativeMove) {
            const bool free = playerStunned <= 0 && !inventoryOpen && !menusOpen;
            using namespace gameoptions;
            controlInput = {};
            controlInput.forward = free && gameInput.held(UP);
            controlInput.back = free && gameInput.held(DOWN);
            controlInput.left = free && gameInput.held(LEFT);
            controlInput.right = free && gameInput.held(RIGHT);
            controlInput.stickX = free ? gameInput.gamepad().movement.x : 0;
            controlInput.stickY = free ? gameInput.gamepad().movement.y : 0;
            controlInput.jump = free && gameInput.held(JUMP);
            controlInput.dodge = free && gameInput.held(DODGE);
            controlInput.classSkill = free && gameInput.held(CLASS_SKILL);
            controlInput.blocked = inventoryOpen || mapOpen || menusOpen; // a window has the keys
            controlInput.ordinaryUi = panelsBeforeInput || menusOpen;
            controlInput.typing = chat.typing();
            controlInput.walk = free && gameInput.held(WALK);
            controlInput.lamp = free && gameInput.held(LAMP);
            controlInput.pet = free && gameInput.held(CALL_PET);
            controlInput.ultimate = free && gameInput.held(ULTIMATE_SKILL);
            { // the special items carried for this region (the special item check, 0x5e890)
                const worldgen::Int2 here = regionHere();
                bool glider = false, boat = false;
                for (const Carried &c : bag)
                    if (c.item.category == 24 && c.item.region[0] == here.x && c.item.region[1] == here.y)
                        glider = glider || c.item.id == 1, boat = boat || c.item.id == 2;
                spawned.setSpecialItems(glider, boat);
                // The artifacts carried give the player their skill points and, in the game's own
                // progression, its level (Pyro's progression keeps its experience levels).
                const lore::ArtifactProgress artifacts = carriedArtifacts();
                spawned.setPlayerArtifacts(artifacts.skills);
                if (mode == progression::Mode::Vanilla && playerLevel != artifacts.level) {
                    playerLevel = artifacts.level;
                    applyGear();
                    logLine("Level " + std::to_string(playerLevel) + " (" + std::to_string(artifacts.level - 1) + " artifacts)");
                }
            }
            {
                // The quick item key (Q, the game's default) uses the first food carried, as
                // the game's uses the food ring's choice (the food slots).
                static bool quickWas = false;
                const bool quick = free && (gameInput.held(gameoptions::QUICK_ITEM) ||
                                            (autoWalk && std::fmod(time, 10.0) > 5 && std::fmod(time, 10.0) < 5.1));
                if (!nativeInventory && quick && !quickWas)
                    for (size_t j = 0; j < bag.size(); j++) {
                        Carried &c = bag[j];
                        if (c.item.category != 1 || c.item.id == 9 || c.item.id == 13) // not bombs or portal stones
                            continue;
                        const nativeplayer::Item raw = nativeplayer::item(c.item.category, c.item.id, c.item.material,
                                                                          c.item.rarity, c.item.modifier, c.item.region[0], c.item.region[1]);
                        if (spawned.useItem(raw.data())) {
                            logLine("Quick item: " + bagName(c.item, itemName(c.item)));
                            consumeCarried(j);
                        }
                        break;
                    }
                quickWas = quick;
            }
            controlInput.attack = free && (gameInput.held(gameoptions::ATTACK) || gameInput.pressed(gameoptions::ATTACK));
            controlInput.strongAttack = free && mouseLook && (gameInput.held(gameoptions::SPECIAL_ATTACK) || gameInput.pressed(gameoptions::SPECIAL_ATTACK));
            controlInput.attack = controlInput.attack || (autoAttack && std::fmod(time, 1.2) < 0.15);
            {
                // Where the player looks, as the game's input finds it: along the camera's
                // ray from its focus to the first solid block within 60 (100 blocks out
                // without one), or the nearest monster the crosshair is on, if that is in
                // front of the player. (CubeVulkan's camera; the crosshair's test on a
                // creature is a block around its middle, not the game's projected box.)
                const glm::vec3 focus = player.position + glm::vec3(0, 0, 1.6f);
                const glm::vec3 dir = -glm::vec3(std::cos(player.yaw) * std::cos(player.pitch),
                                                 std::sin(player.yaw) * std::cos(player.pitch), std::sin(player.pitch));
                const glm::vec3 eye = focus - dir * player.zoom;
                float reach = 0;
                while (reach < 60 && !extractedWorld.solidAt(focus.x + dir.x * reach, focus.y + dir.y * reach, focus.z + dir.z * reach))
                    reach += .25f;
                float attackAlong = -1, chargeDistance = -1;
                controlInput.chargeTarget = 0;
                for (const SpawnedCreatures::Target &c : spawned.targets(extractedWorld, player.position, 100)) {
                    const glm::vec3 middle = c.at; // (its record: a block above its feet, about its middle)
                    const float along = glm::dot(middle - eye, dir);
                    if (along < 0 || glm::length(eye + dir * along - middle) > 1.0f)
                        continue; // not under the crosshair
                    // The strong attack's charge target: the first monster, or any nearer one.
                    const float closeness = glm::dot(c.at - player.position, c.at - player.position);
                    if ((c.hostile && controlInput.chargeTarget == 0) || chargeDistance < 0 || closeness < chargeDistance)
                        chargeDistance = closeness, controlInput.chargeTarget = c.id;
                    if (c.hostile && (attackAlong < 0 || along < attackAlong))
                        attackAlong = along;
                }
                auto toDots = [&](glm::vec3 p, int64_t out[3]) {
                    out[0] = (int64_t)std::llround((double)p.x * 65536) + extractedWorld.originDotsX();
                    out[1] = (int64_t)std::llround((double)p.y * 65536) + extractedWorld.originDotsY();
                    out[2] = (int64_t)std::llround(((double)p.z + SpawnedCreatures::NATIVE_FEET - ExtractedWorld::Z_OFFSET) * 65536);
                };
                nativecontrol::Aim aim;
                toDots(focus, aim.rayOrigin);
                toDots(eye, aim.eye);
                aim.forward[0] = dir.x, aim.forward[1] = dir.y, aim.forward[2] = dir.z;
                aim.wallDistance = reach < 60 ? reach : -1.0f;
                aim.aimDepth = attackAlong;
                controlInput.hasAim = false;
                if (const nativeai::Creature *me = spawned.localPlayer()) {
                    nativecontrol::aimPoint(aim, me->record, controlInput.aimAt);
                    controlInput.hasAim = true;
                }
                crosshairCreature = controlInput.chargeTarget;
            }
            if (autoDodge) // the dodge key, 0.1 s every 3 s, between --auto-walk's jumps
                controlInput.dodge = controlInput.dodge || (std::fmod(time, 3.0) > 1.3 && std::fmod(time, 3.0) < 1.4);
            if (autoWalk) { // forward, a quarter turn every 4 s, a jump every 1.5 s held 0.4 s
                controlInput.forward = true;
                player.yaw += (float)dt * (std::fmod(time, 4.0) < 0.5 ? 3.14159265f : 0.0f);
                controlInput.jump = controlInput.jump || std::fmod(time, 1.5) < 0.4;
                controlInput.ultimate = controlInput.ultimate || std::fmod(time, 8.0) < 0.1; // and the special skill
            }
            static double logged = 0;
            if (time - logged >= (autoWalk || autoAttack || autoDodge ? 1 : 5)) {
                logged = time;
                logLine("Player (native): at " + std::to_string(player.position.x) + " " + std::to_string(player.position.y) + " " +
                        std::to_string(player.position.z) + ", rising " + std::to_string(player.verticalSpeed) + ", ability " + std::to_string(ability) + ", stamina " + std::to_string(spawned.playerStamina()) + ", moves started " + std::to_string(spawned.playerMoves()));
            }
            // The game's camera yaw (degrees) has its forward at -row 1 of its turn: CubeVulkan's
            // yaw less a quarter turn. Its rows for swimming: row 0 to the left, row 2 ahead
            // with the pitch (CubeVulkan's camera, not the game's measured matrix).
            const double yawDeg = (double)player.yaw * 57.29577951308232 - 90.0;
            controlInput.cameraYaw = yawDeg;
            const float t = (float)(yawDeg * 0.017453292519943295), cp = std::cos(player.pitch), sp = std::sin(player.pitch);
            const float rows[3][3] = {{std::cos(t), std::sin(t), 0}, {0, 0, 1}, {std::sin(t) * cp, -std::cos(t) * cp, -sp}};
            std::memcpy(controlInput.camera, rows, sizeof rows);
            glm::vec3 at, velocity;
            float facing;
            bool onGround;
            if (spawned.playerState(extractedWorld, at, facing, velocity, onGround)) {
                player.position = at;
                player.verticalSpeed = onGround ? 0 : velocity.z;
                heading = facing;
            }
            // Its ability is the record's (the game's moves, which the character's
            // recorded animations are keyed by).
            float abilityMs = 0;
            ability = spawned.playerAbility(abilityMs);
            abilityTime = abilityMs / 1000.0;
        } else {
            // Movement relative to the camera.
            glm::vec2 forward(-std::cos(player.yaw), -std::sin(player.yaw)), right(-forward.y, forward.x);
            glm::vec2 move(0);
            if (playerStunned <= 0) {
                if (gameInput.held(gameoptions::UP)) move += forward;
                if (gameInput.held(gameoptions::DOWN)) move -= forward;
                if (gameInput.held(gameoptions::RIGHT)) move -= right;
                if (gameInput.held(gameoptions::LEFT)) move += right;
            }
            // The class skill key (classes without a sprint skill) and debug flight keep
            // CubeVulkan's own speeds (not measured); walking follows the game's measured speed (Body.h).
            bool dash = !moves.skill && gameInput.held(gameoptions::CLASS_SKILL);
            float speed = (dash ? 30.f : Walk::MAX_SPEED) * (player.fly ? 4.f : 1.f);
            if (player.fly) {
                if (glm::length(move) > 0)
                    move = glm::normalize(move) * speed * (float)dt;
                walk.velocity = glm::vec2(0);
            } else {
                walk.update(glm::length(move) > 0 ? glm::normalize(move) : glm::vec2(0), dt, speed);
                move = walk.velocity * (float)dt;
            }
            if (useExtracted && !player.fly) {
                // Walls stop the player; each axis separately, so walls can be slid along.
                if (!blocked(player.position.x + move.x, player.position.y, player.position.z))
                    player.position.x += move.x;
                else
                    walk.velocity.x = 0;
                if (!blocked(player.position.x, player.position.y + move.y, player.position.z))
                    player.position.y += move.y;
                else
                    walk.velocity.y = 0;
            } else {
                player.position.x += move.x;
                player.position.y += move.y;
            }
            float ground = useExtracted ? floorUnder(player.position.x, player.position.y, player.position.z + STEP)
                                        : groundAt(player.position.x, player.position.y);
            if (player.fly) {
                if (gameInput.held(gameoptions::JUMP)) player.position.z += speed * (float)dt;
                if (gameInput.held(gameoptions::WALK)) player.position.z -= speed * (float)dt;
                player.position.z = std::max(player.position.z, ground);
            } else {
                // Steps and slopes are climbed automatically; jumping with space.
                if (gameInput.held(gameoptions::JUMP) && player.position.z <= ground + .05f)
                    player.verticalSpeed = 9;
                player.verticalSpeed -= 28 * (float)dt;
                player.position.z += player.verticalSpeed * (float)dt;
                if (player.position.z <= ground) {
                    player.position.z = player.position.z < ground - 1.5f ? ground : std::min(ground, player.position.z + 12 * (float)dt);
                    player.position.z = std::max(player.position.z, ground - .01f);
                    player.verticalSpeed = 0;
                }
            }
        }

        if (autoAttack && nativeMove && spawnedReady) { // the nearest monster: step up to it and look at it (testing only)
            int race = 0;
            glm::vec3 at;
            if (spawned.nearest(extractedWorld, player.position, race, at, 1) && glm::length(at - player.position) < 300) {
                glm::vec2 d = glm::vec2(at - player.position);
                if (glm::length(d) > 2.5f || std::abs(at.z - player.position.z) > 2.5f) {
                    const glm::vec2 back = (glm::length(d) > .01f ? glm::normalize(d) : glm::vec2(1, 0)) * 2.0f;
                    player.position = glm::vec3(at.x - back.x, at.y - back.y, at.z);
                    d = glm::vec2(at - player.position);
                }
                player.yaw = std::atan2(-d.y, -d.x);
                player.pitch = .15f;
            }
        }
        if (autoAttack && sinceAttack > .6 && !nativeMove) {
            input.buttons.push_back(GLFW_MOUSE_BUTTON_LEFT);
            // The nearest monster, preferring those a quest under way asks for.
            auto wanted = [&](size_t k) {
                size_t r = actors.recordingOf(k);
                for (auto &q : quests)
                    if (q.state == 0 && r < creatureNameIds.size() && creatureNameIds[r] == q.target)
                        return true;
                return false;
            };
            int best = -1;
            for (auto &l : loot) // a pouch nearby first: step onto it (testing only)
                if (l.age > .6 && glm::length(l.at - player.position) < 30) {
                    player.position = l.at, l.taken = l.onGround; // (and takes it, as the use key would)
                    break;
                }
            // One that 8 swings have not hurt is out of reach (inside the ground): skip it.
            static std::set<size_t> unreachable;
            static int lastBest = -1, tries = 0;
            static float lastHealth = 0;
            for (size_t k = 0; k < actors.count(); k++) {
                if (!actors.hittable(k) || unreachable.count(k))
                    continue;
                bool better = best < 0 || (wanted(k) && !wanted(best)) ||
                              (wanted(k) == wanted(best) && glm::length(actors.position(k) - player.position) <
                                                                glm::length(actors.position(best) - player.position));
                if (better)
                    best = (int)k;
            }
            if (best >= 0) {
                tries = best == lastBest && actors.health(best) >= lastHealth ? tries + 1 : 0;
                lastBest = best, lastHealth = actors.health(best);
                if (tries >= 8)
                    unreachable.insert(best), lastBest = -1;
            }
            if (best >= 0) { // Look at it, so the attack faces it.
                glm::vec2 d = glm::vec2(actors.position(best) - player.position);
                if (glm::length(d) > moves.reach || std::abs(actors.position(best).z - player.position.z) > 2.5f) { // Out of reach: step up to it (testing only).
                    glm::vec3 at = actors.position(best);
                    glm::vec2 back = (glm::length(d) > .01f ? glm::normalize(d) : glm::vec2(1, 0)) * std::min(moves.reach * .7f, 6.f);
                    player.position = glm::vec3(at.x - back.x, at.y - back.y, at.z);
                    d = glm::vec2(at - player.position);
                }
                player.yaw = std::atan2(d.y, d.x) + 3.14159265f;
            }
        }
        if (vendorTest >= 0 && time > 2 && spawnedReady) { // --open-vendor
            float best = 1e9f;
            for (const SpawnedCreatures::Label &l : spawned.labels(extractedWorld, player.position, 400.0f))
                if (l.cls == (int)vendorTest && l.distance2 < best) {
                    best = l.distance2;
                    vendorTrade = {true, shopkeeperOf(l.id)};
                }
            if (vendorTrade.open) {
                inventoryOpen = hud.hasInventoryArt() && haveGear;
                bagShown = true;
                logLine("Vendor test: class " + std::to_string(vendorTrade.shop.cls) + " id " + std::to_string(vendorTrade.shop.id) + " home " +
                        std::to_string(vendorTrade.shop.home[0] >> 16) + " " + std::to_string(vendorTrade.shop.home[1] >> 16));
            } else
                logLine("Vendor test: no vendor of class " + std::to_string((int)vendorTest) + " near");
            vendorTest = -1;
        }
        if (identifyTest && time > 2 && haveGear) { // --open-identify
            inventoryOpen = hud.hasInventoryArt();
            bagShown = true;
            identify.show();
            refreshBagOpening();
            for (const Carried &c : bag)
                if (c.item.category == gameshop::UNIDENTIFIED) {
                    if (nativeInventory)
                        identify.bind({c.item.tab, c.item.index});
                    else
                        identify.put(rawOf(c.item));
                    break;
                }
            identifyTest = false;
        }
        if (upgradeTest >= 0 && time > 2 && haveGear) { // --open-upgrade
            inventoryOpen = hud.hasInventoryArt();
            bagShown = true;
            upgrade = {};
            upgrade.open = true;
            upgrade.hand = (int)upgradeTest == upgradewidget::LEFT_HAND ? upgradewidget::LEFT_HAND : upgradewidget::RIGHT_HAND;
            upgradeTest = -1;
        }
        if (closeInventoryAt >= 0 && time > closeInventoryAt) { // --close-inventory
            inventoryOpen = false;
            closeInventoryAt = -1;
        }
        if (!inventoryOpen) // (the shop, the identify window and the upgrade widget close with the inventory)
            vendorTrade.open = upgrade.open = false, inventoryRegionActive = false, identify.close();
        lootWindowItems.clear();
        identificationView = {};
        // Inventory screen: the list holds every measured item not being worn.
        if (inventoryOpen) {
            // the inventory tab of an item, the stackable categories and the rarity colour's rarity colours.
            auto tabOf = [](int category) {
                switch (category) {
                case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 15: case 25: case 26: return 0;
                case 20: case 21: return 4;
                case 2: case 11: return 3;
                case 22: case 24: case 29: return 1;
                case 23: return 5;
                default: return 2;
                }
            };
            auto stacks = [](int category) { return worldgen::Inventory::stackable((uint8_t)category); };
            auto rarityColour = [](int rarity) {
                static const glm::vec4 colours[6] = {{0.7f, 0.7f, 0.7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}, {0.3f, 0.45f, 1, 1}, {0.55f, 0.2f, 1, 1}, {1, 1, 0, 1}};
                return colours[std::clamp(rarity, -1, 4) + 1];
            };
            listed.clear();
            inventoryView.items.clear();
            inventoryView.canonicalTabs = nativeInventory ? &nativeInventory->inventory.tabs : nullptr;
            const worldgen::Int2 here = regionHere();
            if (!inventoryRegionActive) {
                inventoryView.activeRegion = {here.x, here.y};
                inventoryRegionActive = true;
            }
            for (size_t i = 0; !nativeInventory && i < gear.itemCount(); i++)
                if (!gear.isWorn(i)) {
                    const Gear::Item &it = gear.item(i);
                    listed.push_back(i);
                    inventoryView.items.push_back({itemName(it), it.sprite, 1,
                                                   (catalog.mayUse(it.category, it.id, it.material, gear.playerClass()) && levelAllows(it)),
                                                   tabOf(it.category), stacks(it.category), rarityColour(it.rarity)});
                    inventoryView.items.back().region = {it.region[0], it.region[1]};
                }
            const size_t gearRows = listed.size();
            std::vector<inventoryslots::Index> nativeRows;
            nativesaves::Cell viewRegion{here.x, here.y};
            if (const auto *actor = spawned.playerRecord())
                std::memcpy(viewRegion.data(), actor + worldgen::creature::REGION, sizeof viewRegion);
            if (nativeInventory) {
                const auto &tabs = nativeInventory->inventory.tabs;
                for (size_t tab = 0; tab < tabs.size(); ++tab)
                    for (size_t slot = 0; slot < tabs[tab].size(); ++slot) {
                        const auto &stack = tabs[tab][slot];
                        auto item = Gear::fromRecord(stack.item);
                        if (item.category)
                            item.sprite = spawned.itemSprite(stack.item.data());
                        loadInventorySprite(item.sprite);
                        Hud::Inventory::Entry entry;
                        entry.name = item.category ? bagName(item, itemName(item)) : "";
                        entry.icon = item.sprite;
                        entry.count = stack.count;
                        entry.allowed = nativerules::classMayUse(stack.item.data(), gear.playerClass());
                        entry.tab = (int)tab;
                        entry.stack = stacks(item.category);
                        entry.color = guiitem::textColour(stack.item, viewRegion);
                        entry.region = {item.region[0], item.region[1]};
                        entry.slotIndex = (int)slot;
                        entry.original = stack.item;
                        inventoryView.items.push_back(std::move(entry));
                        nativeRows.push_back({(int)tab, (int)slot});
                    }
            } else
                for (const Carried &c : bag) {
                    inventoryView.items.push_back({bagName(c.item, itemName(c.item)), c.item.sprite, c.count, true, tabOf(c.item.category),
                                                   stacks(c.item.category), rarityColour(c.item.rarity)});
                    inventoryView.items.back().region = {c.item.region[0], c.item.region[1]};
                }
            auto refreshRegions = [&] {
                std::vector<inventoryui::Region> regions;
                for (const auto &entry : inventoryView.items)
                    if (entry.tab == inventoryView.tab && entry.count != 0)
                        regions.push_back(entry.region);
                inventoryView.regionChoices = inventoryui::regionChoices(std::move(regions), {here.x, here.y});
                inventoryView.regionName = landNameOf({inventoryView.activeRegion[0], inventoryView.activeRegion[1]});
            };
            refreshRegions();
            // The vendor (the vendor update): its offers, or what was sold to it, newest first.
            std::vector<gameshop::Item> vendorRaws;
            inventoryView.vendor.open = vendorTrade.open;
            auto refreshVendor = [&] {
                vendorRaws.clear();
                inventoryView.vendor.items.clear();
                if (vendorTrade.open) {
                    std::vector<gameshop::Offer> offers;
                    if (inventoryView.vendor.tab == 1)
                        for (auto it = soldItems.rbegin(); it != soldItems.rend(); ++it)
                            offers.push_back({it->first, it->second});
                    else {
                        const worldgen::Int2 region = regionHere();
                        const int tier = gameshop::shopTier(region.x, region.y, saveGame.solvedMissions);
                        const std::vector<gameshop::Item> &bought = boughtFromVendor[vendorTrade.shop.id];
                        offers = gameshop::stock(vendorTrade.shop, day, gear.playerClass(), tier,
                                                 [&](const gameshop::Item &item) { return gameshop::boughtCount(bought, item); });
                    }
                    for (const gameshop::Offer &o : offers) {
                        const uint8_t *n = o.item.data();
                        int32_t id;
                        std::memcpy(&id, n + 4, 4);
                        Gear::Item shown;
                        shown.category = n[0], shown.id = id, shown.material = n[0x1c], shown.rarity = (int8_t)n[0x14];
                        std::memcpy(&shown.modifier, n + 8, 4);
                        std::string sprite = spawnedReady ? spawned.itemSprite(n) : std::string();
                        loadInventorySprite(sprite);
                        Hud::Inventory::Entry e;
                        e.name = bagName(shown, itemName(shown)) + (gameshop::renowned(o.item) ? "+" : "");
                        e.icon = sprite;
                        e.count = o.count;
                        e.allowed = nativerules::classMayUse(o.item.data(), gear.playerClass());
                        e.tab = inventoryView.vendor.tab;
                        e.color = guiitem::textColour(o.item, viewRegion);
                        e.price = inventoryView.vendor.tab == 1 ? gameshop::sellPrice(o.item) : gameshop::price(o.item);
                        e.original = o.item;
                        inventoryView.vendor.items.push_back(e);
                        vendorRaws.push_back(o.item);
                    }
                    // What the bag's items sell for (the inventory panel's drawing, with the vendor open).
                    for (size_t k = 0; k < inventoryView.items.size(); k++) {
                        if (nativeInventory) {
                            const auto *stack = nativeInventory->inventory.slot(nativeRows[k]);
                            inventoryView.items[k].price = stack && stack->count != 0 ? gameshop::sellPrice(stack->item) : 0;
                        } else {
                            const auto &item = k < listed.size() ? gear.item(listed[k]) : bag[k - listed.size()].item;
                            inventoryView.items[k].price = gameshop::sellPrice(rawOf(item));
                        }
                    }
                }
            };
            refreshVendor();
            // Mouse in framebuffer pixels (the window may be scaled).
            double cx, cy;
            pointerPosition(&cx, &cy);
            int ww, wh, fw, fh;
            glfwGetWindowSize(window, &ww, &wh);
            glfwGetFramebufferSize(window, &fw, &fh);
            glm::vec2 mouse((float)cx * fw / std::max(ww, 1), (float)cy * fh / std::max(wh, 1));
            inventoryView.hoverItem = hud.inventoryItemAt(mouse, inventoryView);
            inventoryView.hoverSlot = hud.inventorySlotAt(mouse, inventoryView);
            inventoryView.hoverTab = hud.inventoryTabAt(mouse, inventoryView);
            inventoryView.vendor.hoverItem = hud.vendorItemAt(mouse, inventoryView);
            inventoryView.vendor.hoverTab = hud.vendorTabAt(mouse, inventoryView);
            inventoryView.vendor.hoverButton = hud.inventoryButtonAt(mouse, inventoryView, 1);
            inventoryView.hoverButton = hud.inventoryButtonAt(mouse, inventoryView);
            inventoryView.pointer = mouse;
            if (hoverTest >= 0) {
                inventoryView.hoverItem = (int)hoverTest;
                inventoryView.pointer = glm::vec2(fw * 0.45f, fh * 0.25f);
            }
            inventoryslots::Index hoveredNative;
            if (nativeInventory)
                hoveredNative = hud.inventorySourceAt(mouse, inventoryView);
            if (nativeInventory && hoverTest >= 0 && inventoryView.hoverItem >= 0 && (size_t)inventoryView.hoverItem < nativeRows.size())
                hoveredNative = nativeRows[(size_t)inventoryView.hoverItem];
            // --open-crafting: the crafting widget alone on that tab.
            if (craftTabTest >= 0) {
                craftingOpen = true, bagShown = false;
                inventoryView.crafting.tab = std::clamp((int)craftTabTest, 0, 5);
                inventoryView.crafting.selected = -1;
                craftTabTest = -1;
            }
            // Crafting (the crafting list, the crafting tab, the crafting update): the formulas by
            // tab with how many times each can be made, the selected one's details, the craft
            // button held to make it.
            inventoryView.bagShown = bagShown;
            inventoryView.crafting.open = craftingOpen;
            inventoryView.crafting.classType = gear.playerClass();
            inventoryView.crafting.items.clear();
            inventoryView.crafting.details = {};
            craftRows.clear();
            if (craftingOpen) {
                int32_t region[2] = {0, 0};
                if (const uint8_t *who = spawned.playerRecord())
                    std::memcpy(region, who + worldgen::creature::REGION, 8);
                auto rawOfGear = [](const Gear::Item &item) { return Gear::toRecord(item); };
                auto rawOfThing = [](const gamecraft::Thing &item) { return gamecraft::toRecord(item); };
                auto matches = [&](const Gear::Item &item, const gamecraft::Thing &wanted) {
                    const auto raw = rawOfGear(item), key = rawOfThing(wanted);
                    return nativeinteract::sameItem(raw.data(), key.data());
                };
                // Native ingredient equality includes modifier, region, formula and active cubes.
                auto held = [&](const gamecraft::Thing &t) {
                    int n = 0;
                    for (const Carried &c : bag)
                        if (matches(c.item, t)) n += c.count;
                    return n;
                };
                // An item's icon: the sprite the game draws for it, rendered once.
                auto iconOf = [&](const gameshop::Item &raw) {
                    std::string sprite = spawnedReady ? spawned.itemSprite(raw.data()) : std::string();
                    loadInventorySprite(sprite);
                    return sprite;
                };
                auto shownOf = [](const gamecraft::Thing &t) {
                    Gear::Item g;
                    g.category = t.category, g.id = t.id, g.material = t.material, g.rarity = t.rarity, g.modifier = t.modifier;
                    g.region[0] = t.region[0], g.region[1] = t.region[1];
                    g.formula = t.formula;
                    const auto raw = gamecraft::toRecord(t);
                    Gear::cubesFrom(raw.data(), g);
                    return g;
                };
                // (rank 0 always, a higher one once a crafting-book place of the region is solved)
                const auto tabs = gamecraft::formulas(knownFormulas, gear.playerClass(), region[0], region[1], [&](int rank) {
                    return gamecraft::rankUnlocked(rank, region[0], region[1], saveGame.solvedMissions);
                });
                Hud::Inventory::Crafting &cv = inventoryView.crafting;
                for (int t = 0; t < gamecraft::TABS; t++)
                    for (const gamecraft::Formula &f : tabs[(size_t)t]) {
                        const gameshop::Item raw = rawOfThing(f.result);
                        const Gear::Item shown = shownOf(f.result);
                        Hud::Inventory::Entry e;
                        e.name = bagName(shown, itemName(shown)) + (gameshop::renowned(raw) ? "+" : "");
                        e.icon = iconOf(raw);
                        e.count = gamecraft::times(f, held);
                        e.allowed = nativerules::classMayUse(raw.data(), gear.playerClass());
                        e.tab = t;
                        e.color = guiitem::textColour(raw, viewRegion);
                        e.original = raw;
                        cv.items.push_back(e);
                        craftRows.push_back(f);
                    }
                // The selection: kept, or the tab's first formula (the tab selection).
                if (cv.selected < 0 || cv.selected >= (int)cv.items.size() || cv.items[(size_t)cv.selected].tab != cv.tab) {
                    cv.selected = -1;
                    for (size_t k = 0; k < cv.items.size() && cv.selected < 0; k++)
                        if (cv.items[k].tab == cv.tab)
                            cv.selected = (int)k;
                }
                cv.hoverItem = hud.craftingItemAt(mouse, inventoryView);
                cv.hoverTab = hud.craftingTabAt(mouse, inventoryView);
                cv.hoverButton = hud.inventoryButtonAt(mouse, inventoryView, 2);
                // the formula details drawing: the ingredients held and needed, the workstation
                // (water where the player is in it; the others' static entity within 4 blocks).
                if (cv.selected >= 0) {
                    const gamecraft::Formula &f = craftRows[(size_t)cv.selected];
                    Hud::Inventory::Crafting::Details &d = cv.details;
                    d.valid = true;
                    d.name = cv.items[(size_t)cv.selected].name;
                    if (!d.name.empty() && d.name.back() == '+')
                        d.name.pop_back(); // (the item naming alone)
                    d.color = cv.items[(size_t)cv.selected].color;
                    d.icon = cv.items[(size_t)cv.selected].icon;
                    bool missing = false;
                    for (const gamecraft::Ingredient &in : f.ingredients) {
                        d.ingredients.push_back({iconOf(rawOfThing(in.item)), held(in.item), in.quantity});
                        missing |= d.ingredients.back().have < in.quantity;
                    }
                    bool station = true;
                    if (f.workstation == gamecraft::Water) {
                        const uint8_t *who = spawned.playerRecord();
                        station = who && (who[0x5c] & 2); // (the record's in-water flag)
                    } else if (const int type = gamecraft::stationEntity(f.workstation); type >= 0) {
                        station = false;
                        for (const auto &[kind, at] : workstations)
                            if (kind == type && glm::dot(at - player.position, at - player.position) < 16.0f)
                                station = true;
                    }
                    if (!station)
                        d.missing = gamecraft::requirement(f.workstation);
                    d.ready = station && !missing;
                    d.buttonOver = d.ready && hud.craftButtonAt(mouse, inventoryView);
                    d.hoverIngredient = hud.ingredientAt(mouse, inventoryView);
                }
                // The craft button (loot-crafting.md §3.1): held, it makes the selected recipe in
                // 1000 ms, each further one 1.2 times faster down to 200 ms.
                const bool held0 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || input.gamepadPointer.held[0];
                bool started = false;
                const bool made = craftButton.update((float)(dt * 1000.0), true, cv.details.buttonOver && held0, &started);
                if (started) {
                    craftingFormula = craftRows[(size_t)cv.selected];
                    // Crafted gear is bound to the region the player stands in when it starts.
                    nativeplayer::Item result = rawOfThing(craftingFormula.result);
                    if (nativedrop::inRegionSet(result[0]))
                        craftingFormula.result.region[0] = region[0], craftingFormula.result.region[1] = region[1];
                }
                if (made) {
                    int32_t carried = 0;
                    for (const Carried &c : bag)
                        if (nativeinteract::sameItem(rawOf(c.item).data(), rawOfThing(craftingFormula.result).data()))
                            carried += c.count;
                    if (!useprompt::mayCarryMore(craftingFormula.result.category, craftingFormula.result.id, carried)) {
                        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), "You can't carry more of these items.", {255, 51, 51, 255}});
                        audio.playInterface(gameshop::SOUND_REFUSED);
                    } else {
                        // Making one (§3.2): the ingredients leave the bag, the result (and any extra
                        // pieces) comes in as a received item.
                        for (const gamecraft::Ingredient &in : craftingFormula.ingredients)
                            if (nativeInventory)
                                nativeInventory->inventory.takeCraft(rawOfThing(in.item), in.quantity);
                            else
                                for (size_t k = 0; k < bag.size(); k++) {
                                    const Gear::Item &b = bag[k].item;
                                    if (!matches(b, in.item))
                                        continue;
                                    if (in.quantity < bag[k].count)
                                        bag[k].count -= in.quantity;
                                    else
                                        bag.erase(bag.begin() + (long long)k);
                                    break;
                                }
                        rebuildInventoryViews();
                        audio.playInterface(soundrules::soundid::CRAFT);
                        const int extra = gamecraft::extraPieces(craftingFormula.result, [&] { return (int)(lootDice() & 0x7fff); });
                        if (extra)
                            audio.playInterface(soundrules::soundid::CRAFT_SPECIAL);
                        receiveMany(rawOfThing(craftingFormula.result), extra + 1);
                        logLine("Crafted " + itemName(shownOf(craftingFormula.result)) + (extra ? " (+" + std::to_string(extra) + ")" : ""));
                    }
                }
                cv.details.progress = craftButton.progress;
            } else
                craftButton.update((float)(dt * 1000.0), false, false);
            { // The hovered item's details (the item details text) for the preview.
                inventoryView.info.clear();
                inventoryView.infoIcon.clear();
                const Gear::Item *shown = nullptr;
                std::optional<Gear::Item> nativeShown;
                int count = 1;
                if (nativeInventory) {
                    if (const auto *stack = nativeInventory->inventory.slot(hoveredNative); stack && stack->item[0] != 0) {
                        nativeShown = Gear::fromRecord(stack->item);
                        nativeShown->sprite = spawnedReady ? spawned.itemSprite(stack->item.data()) : std::string();
                        shown = &*nativeShown;
                        count = stack->count;
                    }
                } else if (inventoryView.hoverItem >= 0 && inventoryView.hoverItem < (int)listed.size())
                    shown = &gear.item(listed[(size_t)inventoryView.hoverItem]);
                else if (inventoryView.hoverItem >= (int)listed.size() && inventoryView.hoverItem - listed.size() < bag.size())
                    shown = &bag[(size_t)inventoryView.hoverItem - listed.size()].item,
                    count = bag[(size_t)inventoryView.hoverItem - listed.size()].count;
                if (!shown && inventoryView.hoverSlot >= 0 && gear.equipped(equipSlotOf(inventoryView.hoverSlot)).category)
                    shown = &gear.equipped(equipSlotOf(inventoryView.hoverSlot));
                const uint8_t *who = spawned.playerRecord();
                if (shown && who) {
                    const Gear::Item &it = *shown;
                    const auto raw = Gear::toRecord(it);
                    const int32_t *playerRegion = reinterpret_cast<const int32_t *>(who + worldgen::creature::REGION);
                    const int effective = nativerules::itemLevel(raw.data(), playerRegion);
                    static const glm::vec4 rarities[6] = {{0.7f, 0.7f, 0.7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}, {0.3f, 0.45f, 1, 1}, {0.55f, 0.2f, 1, 1}, {1, 1, 0, 1}};
                    auto rarityColour = [&](int r) { return rarities[std::clamp(r, -1, 4) + 1]; };
                    auto tenth = [](float v) {
                        char b[32];
                        std::snprintf(b, sizeof b, "%g", std::round(v * 10.0f) / 10.0f);
                        return std::string(b);
                    };
                    const bool equipment = (it.category >= 3 && it.category <= 9) || it.category == 26;
                    const bool renowned = equipment && it.modifier % (unsigned)(10 * (it.rarity + 1)) == 0;
                    auto &info = inventoryView.info;
                    info.push_back({itemName(it) + (renowned ? "+" : ""), 15, 4, rarityColour(effective), 24, true});
                    if (it.region[0] || it.region[1]) {
                        if (equipment) {
                            info.push_back({std::string((size_t)std::max(0, it.rarity + 1), (char)0xb1), 18, 3, glm::vec4(0.3f, 0.3f, 0.3f, 1), 24});
                            info.push_back({std::string((size_t)std::max(0, effective + 1), (char)0xb1), 18, 3, rarityColour(effective), 0, false, true});
                        }
                        if (const std::string land = landNameOf({it.region[0], it.region[1]}); !land.empty())
                            info.push_back({land + (renowned ? "+" : ""), 12, 3,
                                            effective == it.rarity ? glm::vec4(1, 0.9f, 0.5f, 1) : glm::vec4(0.75f, 0.75f, 0.75f, 1), 24});
                    }
                    if (!catalog.mayUse(it.category, it.id, it.material, gear.playerClass())) { // the classes that can
                        std::string classes;
                        static const char *const names[5] = {"", "Warrior ", "Ranger ", "Mage ", "Rogue "};
                        for (int c = 1; c <= 4; c++)
                            if (catalog.mayUse(it.category, it.id, it.material, c))
                                classes += names[c];
                        info.push_back({classes, 12, 3, glm::vec4(1, 0, 0, 1), 20});
                    }
                    const glm::vec4 cyan(0, 1, 1, 1), lilac(0.75f, 0.5f, 1, 1);
                    if (it.category == 3)
                        info.push_back({"DAMAGE " + tenth(nativerules::weaponDamage(raw.data(), who)), 15, 3, cyan, 20});
                    if (const float v = nativerules::itemArmor(raw.data(), who); v >= 0.1f)
                        info.push_back({"ARMOR " + tenth(v), 15, 3, cyan, 20});
                    if (const float v = nativerules::itemResistance(raw.data(), who); v >= 0.1f)
                        info.push_back({"RESISTANCE " + tenth(v), 15, 3, cyan, 20});
                    if (const float v = nativerules::itemHealth(raw.data(), who); v >= 0.1f)
                        info.push_back({"HP " + tenth(v), 15, 3, lilac, 16});
                    if (const float v = nativerules::itemHaste(raw.data(), who); v >= 0.001f)
                        info.push_back({"HASTE " + tenth(v * 100.0f) + "%", 12, 3, lilac, 16});
                    if (const float v = nativerules::itemCritical(raw.data(), who); v >= 0.001f)
                        info.push_back({"CRITICAL " + tenth(v * 100.0f) + "%", 12, 3, lilac, 16});
                    if (const float v = nativerules::itemStaminaRegen(raw.data(), who); v >= 0.001f)
                        info.push_back({"REGENERATION " + tenth(v * 100.0f) + "%", 12, 3, lilac, 16});
                    if (it.category == 1 && it.id != 9 && it.id != 13) // healing food and potions
                        info.push_back({"Restores health over " + std::to_string(nativerules::useTime(raw.data()) / 1000) + " seconds", 12, 3,
                                        glm::vec4(0.25f, 1, 0.25f, 1), 16});
                    (void)count;
                    inventoryView.infoIcon = it.sprite;
                }
            }
            // --- The identify window and the weapon upgrade widget ------------------------------
            {
                const float gui = hud.interfaceScale();
                const glm::vec2 guiWindow((float)fw / gui, (float)fh / gui);
                const glm::vec2 guiMouse = (pointerTestX >= 0 ? glm::vec2((float)pointerTestX, (float)pointerTestY) : mouse) / gui;
                static const glm::vec4 rarityColours[6] = {{0.7f, 0.7f, 0.7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}, {0.3f, 0.45f, 1, 1}, {0.55f, 0.2f, 1, 1}, {1, 1, 0, 1}};
                auto rarityColour = [&](int rarity) { return rarityColours[std::clamp(rarity, -1, 4) + 1]; };
                // An item's icon, drawn from its model the first time it is needed.
                auto iconOf = [&](const std::string &sprite) {
                    static std::set<std::string> tried;
                    if (!sprite.empty() && tried.insert(sprite).second)
                        if (auto path = assets.resolve(fs::path("sprites") / (sprite + ".cub"))) {
                            CubModel cub;
                            if (cub.load(*path)) {
                                hud.addItemIcon(sprite, cub);
                                hud.layout(renderer, renderer.screenSize());
                            }
                        }
                    return sprite;
                };
                // Takes one of an item from its first stack in the bag.
                auto takeOne = [&](const gameshop::Item &raw) {
                    for (size_t row = 0; row < bag.size(); ++row)
                        if (nativeinteract::sameItem(rawOf(bag[row].item).data(), raw.data())) {
                            consumeCarried(row);
                            return;
                        }
                };
                std::vector<int> unclaimed; // the clicks the windows leave to the inventory

                auto selectedForIdentification = [&]() -> Carried * {
                    if (nativeInventory) {
                        if (!identify.resolve(nativeInventory->inventory))
                            return nullptr;
                        for (auto &carried : bag)
                            if (inventoryslots::Index{carried.item.tab, carried.item.index} == identify.selection)
                                return &carried;
                    } else if (identify.slotted)
                        for (auto &carried : bag)
                            if (nativeinteract::sameItem(rawOf(carried.item).data(), identify.item.data()))
                                return &carried;
                    return nullptr;
                };
                Carried *slotted = selectedForIdentification();
                if (!slotted && !nativeInventory)
                    identify.takeOut();
                const auto idLayout = hud.identificationLayout();
                const glm::vec2 idMouse = pointerTestX >= 0 ? glm::vec2((float)pointerTestX, (float)pointerTestY) : mouse;
                for (int button : input.buttons) {
                    if (!identify.open || (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT)) {
                        unclaimed.push_back(button);
                        continue;
                    }
                    if (nativeInventory) {
                        const auto *stack = nativeInventory->inventory.slot(hoveredNative);
                        if (stack && stack->count != 0) {
                            identify.bind(hoveredNative);
                            slotted = selectedForIdentification();
                            continue;
                        }
                    } else if (inventoryView.hoverItem >= (int)gearRows && (size_t)inventoryView.hoverItem - gearRows < bag.size()) {
                        identify.put(rawOf(bag[(size_t)inventoryView.hoverItem - gearRows].item));
                        slotted = selectedForIdentification();
                        continue;
                    }
                    if (!idLayout.inWindow(idMouse))
                        unclaimed.push_back(button);
                }
                input.buttons = std::move(unclaimed);
                for (int button : input.released) {
                    if (!identify.acceptsRelease(button))
                        continue;
                    slotted = selectedForIdentification();
                    if (idLayout.onButton(idMouse, slotted && identify.item[0] == gameshop::UNIDENTIFIED)) {
                        if (identifywindow::canIdentify(identify, slotted ? slotted->count : 0, playerGold)) {
                            const auto outcome = identifywindow::identifyOnce(identify, [&] { return (int)(lootDice() % 32768); });
                            playerGold -= outcome.paid;
                            if (nativeInventory)
                                nativeInventory->inventory.consumeOne(identify.selection);
                            else
                                takeOne(identify.item);
                            receive(outcome.made);
                            audio.playInterface(gameshop::SOUND_IDENTIFIED);
                            slotted = selectedForIdentification();
                        }
                    } else if (idLayout.onGoodbye(idMouse)) {
                        identify.close();
                        bagShown = false;
                        inventoryOpen = craftingOpen;
                    }
                }
                if (identify.open) {
                    auto &shown = identificationView;
                    if (identify.slotted && slotted && identify.item[0] == gameshop::UNIDENTIFIED) {
                        shown.slotted = true;
                        shown.original = identify.item;
                        shown.name = bagName(slotted->item, itemName(slotted->item));
                        shown.icon = iconOf(!slotted->item.sprite.empty() ? slotted->item.sprite
                                            : spawnedReady                ? spawned.itemSprite(identify.item.data())
                                                                          : std::string());
                        int32_t region[2] = {here.x, here.y};
                        if (const auto *actor = spawned.playerRecord())
                            std::memcpy(region, actor + worldgen::creature::REGION, 8);
                        const int rarity = nativerules::itemLevel(identify.item.data(), region);
                        shown.colour = rarityColour(rarity);
                        shown.regionColour = rarity == slotted->item.rarity ? glm::vec4(1, .9f, .5f, 1) : glm::vec4(.75f, .75f, .75f, 1);
                        static const char *const kinds[] = {"Plains",    "Ocean",      "Hills",     "Woodlands", "Jungle",
                                                            "DarkWoods", "SnowForest", "Mountains", "Wetlands",  "Desert",
                                                            "Savannah",  "Snowlands",  "Deadlands", "Firelands"};
                        const int x = slotted->item.region[0], y = slotted->item.region[1];
                        const int kind = worldgen::regionBiome(x, y);
                        if (kind >= 0 && kind < 14 && speech.hasEntry(kinds[kind]))
                            shown.regionDescription = speech.place(
                                kinds[kind], itemNameParts.name((int32_t)worldgen::random((uint32_t)x + 57u * (uint32_t)y), -1));
                        shown.count = slotted->count;
                    }
                    shown.price = identifywindow::price(identify);
                    shown.gold = playerGold;
                    shown.ready = identifywindow::canIdentify(identify, slotted ? slotted->count : 0, playerGold);
                    shown.overButton = idLayout.onButton(idMouse, shown.slotted);
                    shown.overGoodbye = idLayout.onGoodbye(idMouse);
                }

                // The weapon upgrade widget (loot-crafting.md §4), opened by right-clicking a worn
                // weapon: its palette from the bag, the voxel under the pointer, and the clicks.
                for (int button : input.buttons)
                    if (!nativeInventory && button == GLFW_MOUSE_BUTTON_RIGHT && inventoryView.hoverSlot >= 0) {
                        const Gear::Slot s = equipSlotOf(inventoryView.hoverSlot);
                        if ((s == Gear::WeaponRight || s == Gear::WeaponLeft) && gear.equipped(s).category) {
                            const int hand = s == Gear::WeaponLeft ? upgradewidget::LEFT_HAND : upgradewidget::RIGHT_HAND;
                            const bool again = upgrade.open && upgrade.hand == hand;
                            upgrade = {};
                            upgrade.open = !again, upgrade.hand = hand;
                            if (upgrade.open)
                                vendorTrade.open = false, identify.close();
                        }
                    }
                const Gear::Slot hand = upgrade.hand == upgradewidget::LEFT_HAND ? Gear::WeaponLeft : Gear::WeaponRight;
                if (upgrade.open && !gear.equipped(hand).category)
                    upgrade.open = false;
                if (upgrade.open) {
                    const upgradewidget::Layout upLayout = upgradewidget::layout(guiWindow);
                    const Gear::Item &weapon = gear.equipped(hand);
                    gameshop::Item record = rawOf(weapon);
                    // The weapon's model, loaded when the hand's weapon changes.
                    std::string sprite = gear.sprite(hand).empty() ? weapon.sprite : gear.sprite(hand);
                    if (sprite.empty() && spawnedReady)
                        sprite = spawned.itemSprite(record.data());
                    if (sprite != upgradeModelSprite) {
                        upgradeModel = CubModel{};
                        if (auto path = assets.resolve(fs::path("sprites") / (sprite + ".cub")); path && !sprite.empty())
                            upgradeModel.load(*path);
                        upgradeModelSprite = sprite;
                    }
                    // The palette, from the bag as it is now.
                    std::vector<std::pair<gameshop::Item, int>> carried;
                    for (const Carried &c : bag)
                        carried.push_back({rawOf(c.item), c.count});
                    upgrade.palette = upgradewidget::paletteOf(carried);
                    upgrade.pointed = upLayout.onView(guiMouse)
                                          ? upgradewidget::pick(upgradeModel, record.data(), upgradeView, upLayout.viewPixel(guiMouse, upgradeView.pixels))
                                          : std::nullopt;
                    // A click's result: one cube item taken, the cubes kept with the worn weapon.
                    auto applyClick = [&](const upgradewidget::ClickResult &r) {
                        if (r.takeOne)
                            takeOne(r.taken);
                        if (r.weaponChanged) {
                            Gear::Item changed = gear.equipped(hand);
                            Gear::cubesFrom(record.data(), changed);
                            gear.updateWorn(hand, changed);
                            if (nativeInventory)
                                nativeInventory->equipment[(size_t)hand] = Gear::toRecord(changed);
                            applyGear();
                            // (the native player's own record wears it: the pose draws its cubes)
                            constexpr size_t WORN = 0x278, ITEM_SIZE = 0xa0;
                            if (const uint8_t *me = spawned.playerRecord())
                                logLine("Upgrade: the worn weapon holds " +
                                        std::to_string(me[WORN + (size_t)hand * ITEM_SIZE + Gear::CUBE_COUNT_AT]) + " cubes");
                        }
                    };
                    unclaimed.clear();
                    for (int button : input.buttons) {
                        if (button != GLFW_MOUSE_BUTTON_LEFT) {
                            unclaimed.push_back(button);
                            continue;
                        }
                        if (const int box = upLayout.paletteBoxAt(guiMouse); box >= 0) { // a palette entry chosen
                            if (box < (int)upgrade.palette.size())
                                upgrade.selected = upgrade.selected == box ? -1 : box, upgrade.carrying = false;
                            continue;
                        }
                        if (const int turn = upLayout.turnAt(guiMouse)) { // the view turned
                            upgradeView.yawDegrees = std::fmod(upgradeView.yawDegrees + 45.0f * (float)turn + 360.0f, 360.0f);
                            continue;
                        }
                        // The widget's click (0x292650); the pointer over the window or the
                        // inventory is "over another widget".
                        const bool overOther = upLayout.inWindow(guiMouse) || inventoryView.hoverItem >= 0 || inventoryView.hoverSlot >= 0 ||
                                               inventoryView.hoverTab >= 0;
                        applyClick(upgradewidget::click(upgrade, record.data(), overOther));
                        if (!upLayout.inWindow(guiMouse))
                            unclaimed.push_back(button);
                    }
                    input.buttons = std::move(unclaimed);
                    // --upgrade-demo: a cube of each of the first palette entries, set through the
                    // widget's own click where the picture shows the weapon.
                    if (upgradeDemo && upgradeModel.sx) {
                        for (int k = 0; k < (int)upgrade.palette.size() && k < 3; k++)
                            for (int step = 0; step < 32; step++) {
                                const glm::vec2 pixel((float)upgradeView.pixels * (0.3f + 0.4f * (float)step / 32.0f),
                                                      (float)upgradeView.pixels * (0.42f + 0.08f * (float)k));
                                upgrade.pointed = upgradewidget::pick(upgradeModel, record.data(), upgradeView, pixel);
                                if (!upgrade.pointed)
                                    continue;
                                upgrade.selected = k;
                                applyClick(upgradewidget::click(upgrade, record.data(), true));
                                logLine("Upgrade demo: a cube of material " + std::to_string(upgrade.palette[(size_t)k].item[0x1c]) + " at " +
                                        std::to_string(upgrade.pointed->x) + " " + std::to_string(upgrade.pointed->y) + " " +
                                        std::to_string(upgrade.pointed->z));
                                break;
                            }
                        upgrade.selected = -1;
                        upgrade.pointed.reset();
                        upgradeDemo = false;
                    }
                    // The picture, drawn again when the weapon, its cubes or the view changed.
                    record = rawOf(gear.equipped(hand));
                    uint64_t cubesSeen = 1469598103934665603ull; // (FNV-1a over the cube bytes)
                    for (size_t k = Gear::CUBES_AT; k <= Gear::CUBE_COUNT_AT; k++)
                        cubesSeen = (cubesSeen ^ record[k]) * 1099511628211ull;
                    const std::string key = upgradeModelSprite + "|" + std::to_string(upgradeView.yawDegrees) + "|" +
                                            std::to_string(upgradeView.pitchDegrees) + "|" + std::to_string(cubesSeen);
                    if (key != upgradePictureKey && upgradeModel.sx) {
                        hud.addIcon("upgrade-weapon", upgradewidget::render(upgradeModel, record.data(), upgradeView), upgradeView.pixels);
                        hud.layout(renderer, renderer.screenSize());
                        upgradePictureKey = key;
                    }
                    upgradewidget::Shown shown;
                    shown.title = hand == Gear::WeaponLeft ? "Upgrade: Left Weapon" : "Upgrade: Right Weapon";
                    shown.weaponName = itemName(gear.equipped(hand));
                    shown.weaponColour = rarityColour(gear.equipped(hand).rarity);
                    shown.pictureIcon = upgradeModel.sx ? "upgrade-weapon" : "";
                    shown.cubes = record[Gear::CUBE_COUNT_AT];
                    shown.cap = gamecraft::cubeCap(record.data());
                    for (const upgradewidget::PaletteEntry &e : upgrade.palette)
                        shown.palette.push_back({iconOf(spawnedReady ? spawned.itemSprite(e.item.data()) : std::string()), e.count,
                                                 upgradewidget::cubeColour(e.item[0x1c])});
                    shown.selected = upgrade.selected;
                    shown.hoverBox = upLayout.paletteBoxAt(guiMouse);
                    shown.hoverTurn = upLayout.turnAt(guiMouse);
                    if (upgrade.pointed && upgradeModel.sx) {
                        const float toUnits = upLayout.viewSize / (float)upgradeView.pixels;
                        shown.marker = upLayout.viewAt + upgradewidget::project(upgradeModel, upgradeView, *upgrade.pointed) * toUnits;
                        shown.markerSize = upgradewidget::voxelPixels(upgradeModel, upgradeView) * toUnits;
                        if (upgrade.selectionValid())
                            shown.markerColour = upgradewidget::cubeColour(upgrade.palette[(size_t)upgrade.selected].item[0x1c]) * glm::vec4(1, 1, 1, 0.7f);
                        else if (upgrade.carrying)
                            shown.markerColour = upgradewidget::cubeColour(upgrade.carriedMaterial) * glm::vec4(1, 1, 1, 0.7f);
                    }
                    shown.hint = upgrade.selectionValid() ? "Click to set the cube"
                                 : upgrade.carrying       ? "Click to put it down"
                                 : upgrade.palette.empty() ? "No cubes carried"
                                                           : "Pick a cube below";
                    const Hud::Screen items = upgradewidget::screen(upLayout, shown);
                    lootWindowItems.insert(lootWindowItems.end(), items.begin(), items.end());
                }
            }
            auto changePanelTab = [&](int which, int next) {
                auto change = [&](auto &panel) {
                    if (panel.tab == next)
                        return;
                    panel.selectTab(next);
                    if (which == 0)
                        refreshRegions();
                    else if (which == 1)
                        refreshVendor();
                    panel.scroll = std::clamp(panel.scroll, 0, std::max(0, hud.inventoryRowCount(inventoryView, which) - 1));
                    panel.selected = hud.inventoryFirstItem(inventoryView, which);
                    audio.playInterface(soundrules::soundid::MENU_TAB);
                };
                if (which == 0)
                    change(inventoryView);
                else if (which == 1)
                    change(inventoryView.vendor);
                else
                    change(inventoryView.crafting);
            };
            auto changeRegion = [&](int direction) {
                inventoryui::changeRegion(inventoryView.activeRegion, inventoryView.regionChoices, direction);
                refreshInventory();
                refreshRegions();
                inventoryView.scroll = std::clamp(inventoryView.scroll, 0, std::max(0, hud.inventoryRowCount(inventoryView) - 1));
                audio.playInterface(soundrules::soundid::MENU_TAB);
            };
            auto scrollPanel = [&](int which, int &offset, int step) {
                const int last = std::max(0, hud.inventoryRowCount(inventoryView, which) - 1);
                const int before = std::clamp(offset, 0, last);
                offset = std::clamp(before + step, 0, last);
                if (offset != before)
                    audio.playInterface(soundrules::soundid::MENU_TAB);
            };
            if (bagShown) { // the inventory scrolling / scrolling down, by the buttons and the wheel
                int step = 0;
                for (int button : input.buttons)
                    if (button == GLFW_MOUSE_BUTTON_LEFT && inventoryView.hoverButton)
                        step += inventoryView.hoverButton == 1 ? -1 : 1;
                step -= (int)inventoryWheel;
                inventoryWheel -= (int)inventoryWheel;
                if (input.gamepadPointer.scrollPanel == 0)
                    step += input.gamepadPointer.scrollRows;
                scrollPanel(0, inventoryView.scroll, step);
            }
            if (craftingOpen) { // the crafting widget's scroll buttons (and the wheel, without the bag)
                Hud::Inventory::Crafting &cv = inventoryView.crafting;
                int step = 0;
                for (int button : input.buttons)
                    if (button == GLFW_MOUSE_BUTTON_LEFT && cv.hoverButton)
                        step += cv.hoverButton == 1 ? -1 : 1;
                if (!bagShown) {
                    step -= (int)inventoryWheel;
                    inventoryWheel -= (int)inventoryWheel;
                }
                if (input.gamepadPointer.scrollPanel == 1)
                    step += input.gamepadPointer.scrollRows;
                scrollPanel(2, cv.scroll, step);
            }
            if (vendorTrade.open) {
                int step = input.gamepadPointer.scrollPanel == 2 ? input.gamepadPointer.scrollRows : 0;
                for (int button : input.buttons)
                    if (button == GLFW_MOUSE_BUTTON_LEFT && inventoryView.vendor.hoverButton)
                        step += inventoryView.vendor.hoverButton == 1 ? -1 : 1;
                scrollPanel(1, inventoryView.vendor.scroll, step);
            }
            gameshop::TradeHooks trade;
            trade.inventory = [&] { refreshInventory(); };
            trade.shop = [&] {
                refreshVendor();
                refreshInventory();
            };
            trade.crafting = [&] { refreshInventory(); }; // collection contents are regenerated on the next frame
            trade.rememberBought = [&](const auto &item) { boughtFromVendor[vendorTrade.shop.id].push_back(item); };
            trade.announce = [&](const auto &item) {
                const auto view = Gear::fromRecord(item);
                if (view.category == 24)
                    audio.startSuccess();
                texts.push_back({player.position + glm::vec3(0, 0, 3.1f), bagName(view, itemName(view)), {255, 255, 255, 255}});
            };
            trade.sound = [&](int sound) {
                int64_t at[3];
                playerUnits(at);
                audio.playAt(sound, at, sound == gameshop::SOUND_SOLD ? gameshop::SOLD_VOLUME : 1, 1);
            };
            trade.refused = [&](gameshop::Refusal reason) {
                if (reason != gameshop::Refusal::ProtectedTab)
                    texts.push_back({player.position + glm::vec3(0, 0, 3.1f),
                                     reason == gameshop::Refusal::Money ? "Not enough money." : "You can't carry more of these items.",
                                     {255, 51, 51, 255}});
                audio.playInterface(gameshop::SOUND_REFUSED);
            };
            for (int button : input.buttons) {
                if (nativeInventory && button == GLFW_MOUSE_BUTTON_MIDDLE && !mapOpen) {
                    if (auto *actor = spawned.localPlayer())
                        if (auto item = nativeInventory->inventory.dropOne(hoveredNative, actor->record)) {
                            auto request = nativeinteract::fresh(nativeinteract::DROP);
                            std::memcpy(request.data() + nativeinteract::ITEM, item->data(), item->size());
                            actor->interactions.push_back(request);
                            rebuildInventoryViews();
                        }
                    continue;
                }
                if ((button != GLFW_MOUSE_BUTTON_LEFT && !(nativeInventory && button == GLFW_MOUSE_BUTTON_RIGHT)) ||
                    inventoryView.hoverButton)
                    continue;
                if (const int region = hud.inventoryRegionButtonAt(mouse, inventoryView)) {
                    changeRegion(region);
                    continue;
                }
                if (craftingOpen && inventoryView.crafting.hoverTab >= 0) { // the crafting tabs
                    changePanelTab(2, inventoryView.crafting.hoverTab);
                    continue;
                }
                if (craftingOpen && inventoryView.crafting.hoverItem >= 0) { // the inventory selection frame(true)
                    inventoryView.crafting.selected = inventoryView.crafting.hoverItem;
                    audio.playInterface(soundrules::soundid::MENU_SELECT);
                    continue;
                }
                if (craftingOpen && inventoryView.crafting.hoverButton)
                    continue;
                if (vendorTrade.open && inventoryView.vendor.hoverTab >= 0) {
                    changePanelTab(1, inventoryView.vendor.hoverTab);
                    continue;
                }
                if (nativeInventory && vendorTrade.open) {
                    if (auto *actor = spawned.localPlayer()) {
                        const float oldMaximum = spawned.playerMaxHealth();
                        const auto *source = nativeInventory->inventory.slot(hoveredNative);
                        if (source && source->count != 0)
                            gameshop::sellSlot(nativeInventory->inventory, hoveredNative, soldItems, trade);
                        else if (inventoryView.hoverSlot >= 0 &&
                                 nativeInventory->equipment[(size_t)equipSlotOf(inventoryView.hoverSlot)][0] != 0) {
                            gameshop::sellEquipment(*nativeInventory, (int)equipSlotOf(inventoryView.hoverSlot), soldItems, trade);
                            rebuildInventoryViews();
                            applyGear(true);
                        } else if (inventoryView.vendor.hoverItem >= 0 && (size_t)inventoryView.vendor.hoverItem < vendorRaws.size()) {
                            const auto offer = vendorRaws[(size_t)inventoryView.vendor.hoverItem];
                            if (inventoryView.vendor.tab == 1)
                                gameshop::repurchase(nativeInventory->inventory, soldItems, (size_t)inventoryView.vendor.hoverItem, trade);
                            else
                                gameshop::purchase(nativeInventory->inventory, offer, trade);
                        }
                        rebuildInventoryViews();
                        playerHp = inventoryslots::adjustHealth(actor->record, oldMaximum, [&] { return spawned.playerMaxHealth(); });
                        chat.stopEditing();
                    }
                    continue;
                }
                if (vendorTrade.open && inventoryView.vendor.hoverItem >= 0 &&
                    inventoryView.vendor.hoverItem < (int)vendorRaws.size()) { // Game's buying (and buying back)
                    const gameshop::Item offer = vendorRaws[(size_t)inventoryView.vendor.hoverItem];
                    const bool back = inventoryView.vendor.tab == 1;
                    // (loot-shops.md §4.1, §4.3: buying pays the price, buying back the selling price)
                    const int cost = back ? gameshop::sellPrice(offer) : gameshop::price(offer);
                    const glm::u8vec4 warning{255, 51, 51, 255};
                    if (cost > playerGold) {
                        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), "Not enough money.", warning});
                        audio.playInterface(gameshop::SOUND_REFUSED);
                        continue;
                    }
                    int32_t subtype;
                    std::memcpy(&subtype, offer.data() + 4, 4);
                    int carried = 0;
                    for (const Carried &c : bag)
                        if (nativeinteract::sameItem(rawOf(c.item).data(), offer.data()))
                            carried += c.count;
                    if (!useprompt::mayCarryMore(offer[0], subtype, carried)) {
                        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), "You can't carry more of these items.", warning});
                        audio.playInterface(gameshop::SOUND_REFUSED);
                        continue;
                    }
                    playerGold -= cost;
                    receive(offer);
                    if (back) { // (one unit a click; how the game shortens an entry is open, §8.2)
                        const size_t k = soldItems.size() - 1 - (size_t)inventoryView.vendor.hoverItem;
                        if (--soldItems[k].second <= 0)
                            soldItems.erase(soldItems.begin() + (long long)k);
                    } else
                        boughtFromVendor[vendorTrade.shop.id].push_back(offer); // keeps it from coming back today
                    int64_t purchasePosition[3];
                    playerUnits(purchasePosition);
                    audio.playAt(gameshop::SOUND_BOUGHT, purchasePosition, 1, 1);
                    logLine("Vendor: bought for " + std::to_string(cost) + " gold");
                    continue;
                }
                if (vendorTrade.open && inventoryView.hoverItem >= 0) { // selling (not specials or artifacts)
                    const int k = inventoryView.hoverItem;
                    if (inventoryView.items[(size_t)k].tab == 1 || inventoryView.items[(size_t)k].tab == 5) {
                        logLine("Vendor: that is not for sale");
                        continue;
                    }
                    Gear::Item it;
                    if (nativeInventory) {
                        const auto *stack = nativeInventory->inventory.slot(hoveredNative);
                        if (!stack || stack->count == 0)
                            continue;
                        it = Gear::fromRecord(stack->item);
                    } else
                        it = k < (int)listed.size() ? gear.item(listed[(size_t)k]) : bag[(size_t)k - listed.size()].item;
                    const gameshop::Item raw = rawOf(it);
                    playerGold += gameshop::sellPrice(raw);
                    int64_t salePosition[3];
                    playerUnits(salePosition);
                    audio.playAt(gameshop::SOUND_SOLD, salePosition, gameshop::SOLD_VOLUME, 1);
                    gameshop::rememberSold(soldItems, raw);
                    if (k < (int)listed.size()) {
                        gear.remove(listed[(size_t)k]);
                        applyGear();
                    } else if (nativeInventory) {
                        nativeInventory->inventory.consumeOne(hoveredNative);
                        refreshInventory();
                    } else
                        consumeCarried((size_t)k - listed.size());
                    logLine("Vendor: sold " + itemName(it) + " for " + std::to_string(gameshop::sellPrice(raw)) + " gold");
                    continue;
                }
                if (inventoryView.hoverTab >= 0) { // the inventory tab selection
                    changePanelTab(0, inventoryView.hoverTab);
                    continue;
                }
                if (nativeInventory) {
                    if (auto *actor = spawned.localPlayer()) {
                        const float oldMaximum = spawned.playerMaxHealth();
                        auto outcome = upgrade.open ? inventoryslots::UseResult{}
                                                    : inventoryslots::use(*nativeInventory, hoveredNative, actor->record, button,
                                                                          glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                                                              glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS,
                                                                          spawned.playerAbilityDuration());
                        using Action = inventoryslots::Action;
                        if (outcome.interaction) {
                            auto request = nativeinteract::fresh(nativeinteract::USE_ITEM);
                            std::memcpy(request.data() + nativeinteract::ITEM, outcome.interaction->data(), nativesaves::ITEM_SIZE);
                            actor->interactions.push_back(request);
                        }
                        if (outcome.action == Action::Refused || outcome.action == Action::WrongRegion)
                            audio.playInterface(outcome.action == Action::Refused ? 0x35 : 0x34);
                        else if (outcome.action == Action::Equipped || outcome.action == Action::WeaponEquipped) {
                            rebuildInventoryViews();
                            applyGear(true);
                            refreshInventory();
                            audio.playInterface(0x5b);
                            if (outcome.action == Action::WeaponEquipped && !nativeInventory->inventory.tabs.empty())
                                refreshInventory();
                        } else if (outcome.action == Action::PetFood)
                            inventoryOpen = false;
                        else if (outcome.action == Action::Map) {
                            mapSelection.useItem(nativeInventory->inventory.slot(hoveredNative));
                        } else if (!upgrade.open && !vendorTrade.open && inventoryView.hoverSlot >= 0 && hoveredNative.tab < 0) {
                            if (nativeInventory->unequip((int)equipSlotOf(inventoryView.hoverSlot))) {
                                rebuildInventoryViews();
                                applyGear(true);
                                refreshInventory();
                            }
                        }
                        rebuildInventoryViews();
                        playerHp = inventoryslots::adjustHealth(actor->record, oldMaximum, [&] { return spawned.playerMaxHealth(); });
                        chat.stopEditing();
                    }
                    continue;
                }
                if (inventoryView.hoverItem >= (int)gearRows) {
                    const size_t j = (size_t)inventoryView.hoverItem - gearRows;
                    Carried &c = bag[j];
                    const std::string name = bagName(c.item, itemName(c.item));
                    if (nativePlayer && !player.fly && (c.item.category == 1 || c.item.category == 0x1c)) {
                        // The native player eats or drinks as the game's input starts it.
                        const nativeplayer::Item raw = nativeplayer::item(c.item.category, c.item.id, c.item.material, c.item.rarity,
                                                                          c.item.modifier, c.item.region[0], c.item.region[1]);
                        if (spawned.useItem(raw.data())) {
                            logLine("Inventory: using " + name + " (the game's item use)");
                            consumeCarried(j);
                        }
                    } else if (c.item.category == 1 && eatMs < 0) { // food
                        const nativeplayer::Item raw = nativeplayer::item(c.item.category, c.item.id, c.item.material, c.item.rarity,
                                                                          c.item.modifier, c.item.region[0], c.item.region[1]);
                        eatUse = nativerules::useTime(raw.data());
                        eatMs = 0;
                        logLine("Inventory: eating " + name + " (" + std::to_string(eatUse) + " ms)");
                        if (--c.count <= 0)
                            bag.erase(bag.begin() + (long long)j);
                    } else
                        logLine("Inventory: " + name + " x" + std::to_string(c.count) +
                                (c.item.category == 0x1c ? " (potions' buffs are not ported yet)" : " (not something to use)"));
                } else if (inventoryView.hoverItem >= 0) {
                    size_t i = listed[inventoryView.hoverItem];
                    const Gear::Item &it = gear.item(i);
                    if (!inventoryView.items[inventoryView.hoverItem].allowed)
                        logLine(levelAllows(it) ? "Inventory: your class cannot use " + itemName(it) + " (the game's class rule)"
                                                : "Inventory: " + itemName(it) + " needs level " + std::to_string(itemLevel(it)));
                    else {
                        gear.equip(i);
                        applyGear();
                        logLine("Inventory: equipped " + itemName(it));
                    }
                } else if (inventoryView.hoverSlot >= 0) {
                    Gear::Slot s = equipSlotOf(inventoryView.hoverSlot);
                    if (gear.wornMeasured(s)) {
                        logLine("Inventory: took off " + itemName(gear.equipped(s)));
                        gear.unequip(s);
                        applyGear();
                    }
                }
            }
            if (input.gamepadPointer.regionDirection)
                changeRegion(input.gamepadPointer.regionDirection);
            for (const auto &tab : input.gamepadPointer.tabs) {
                const int which = tab.panel == 1 ? 2 : tab.panel == 2 ? 1 : 0;
                const int current = which == 0 ? inventoryView.tab : which == 1 ? inventoryView.vendor.tab : inventoryView.crafting.tab;
                changePanelTab(which, (current + tab.direction + (which == 1 ? 2 : 6)) % (which == 1 ? 2 : 6));
            }
            input.buttons.clear(); // No attacks while the inventory is open.
            // Slot icons: what is equipped (measured items, or the recorded sprite of
            // gear the survey could not swap, such as the left weapon).
            static const int partMatrix[10] = {8, 7, -1, -1, 12, 11, 2, -1, -1, -1};
            for (int s = 0; s < 10; s++) {
                Gear::Slot g = equipSlotOf(s);
                std::string icon;
                if (gear.wornMeasured(g))
                    icon = gear.sprite(g);
                else if (gear.equipped(g).category != 0 && partMatrix[s] >= 0)
                    icon = spriteForPartMatrix(partMatrix[s]);
                if (nativeInventory) {
                    inventoryView.slotItems[(size_t)s] = nativeInventory->equipment[(size_t)g];
                    icon = gear.equipped(g).sprite;
                } else
                    inventoryView.slotItems[(size_t)s].reset();
                inventoryView.slots[s] = icon;
            }
            std::string name = character.name;
            for (auto &c : name)
                c = (char)std::toupper((unsigned char)c);
            inventoryView.name = name;
            inventoryView.level = character.level > 0 ? character.level : 1;
            static const char *classes[] = {"", "WARRIOR", "RANGER", "MAGE", "ROGUE"};
            static const char *specs[5][2] = {{"", ""}, {"BERSERKER", "GUARDIAN"}, {"SNIPER", "SCOUT"},
                                              {"FIRE MAGE", "WATER MAGE"}, {"ASSASSIN", "NINJA"}};
            int cls = std::clamp(gear.playerClass(), 0, 4), spec = std::clamp(character.specialization, 0, 1);
            inventoryView.className = classes[cls];
            inventoryView.specialization = specs[cls][spec];
            char buffer[32];
            inventoryView.stats.clear();
            inventoryView.stats.push_back({"HIT POINTS", std::to_string((int)playerMaxHp)});
            inventoryView.stats.push_back({"ATTACK POWER", std::to_string((int)playerAttack)});
            std::snprintf(buffer, sizeof buffer, "%.1f", playerArmor);
            inventoryView.stats.push_back({"ARMOR", buffer});
            std::snprintf(buffer, sizeof buffer, "%.1f%%", playerCritical * 100);
            inventoryView.stats.push_back({"CRITICAL", buffer});
            { // The game's character panel (CharacterWidget): the native player's sheet.
                Hud::Inventory::Sheet &sheet = inventoryView.sheet;
                const SpawnedCreatures::Sheet native = spawned.sheet();
                sheet.valid = native.valid;
                sheet.caster = cls == 3;
                sheet.weaponLevel = native.weaponLevel;
                sheet.armorLevel = native.armorLevel;
                sheet.hp = playerMaxHp;
                sheet.power = playerAttack;
                sheet.armor = playerArmor;
                sheet.resistance = native.resistance;
                sheet.critical = playerCritical * 100.0f;
                sheet.haste = (native.tempo - 1.0f) * 100.0f;
                sheet.regeneration = native.regeneration * 100.0f;
                sheet.manaGeneration = 100.0f; // (1 without the mana-absorption buff)
                // The travel skills by the artifacts' levels (the climbing speed and the rest).
                const lore::ArtifactProgress carried = carriedArtifacts();
                std::copy(carried.skills.begin(), carried.skills.end(), sheet.artifacts);
                for (int k = 0; k < 7; k++) {
                    const float a = (float)sheet.artifacts[k], f = 1.0f - 1.0f / (a / 20.0f + 1.0f);
                    switch (k) {
                    case 0: case 1: sheet.travel[k] = (f + 1.0f) * 0.5f * 100.0f; break;             // climbing, swimming
                    case 2: sheet.travel[k] = (1.0f - f) * 0.1f * 100.0f; break;                    // diving
                    case 3: sheet.travel[k] = (f * 1.4f + 1.1f) * 100.0f; break;                    // riding
                    case 4: case 5: sheet.travel[k] = (f * 1.9f + 1.1f) * 100.0f; break;            // gliding, sailing
                    default: sheet.travel[k] = (f * 12.0f + 8.0f) * 0.5f; break;                    // light radius
                    }
                }
            }
        }
        // The game's default controls: attack (left mouse), special (right mouse; a
        // tap, since holding it turns the camera), dodge (middle mouse), class skill
        // (Shift), P and the ultimate (R).
        sinceAttack += dt;
        if (inventoryOpen || answers.visible()) attackQueued = false;
        if (!answers.visible() && gameInput.pressed(gameoptions::ATTACK) && ATTACK_A)
            attackQueued = true; // Keep the native swing intact; queue its successor.
        else if (gameInput.pressed(gameoptions::DODGE) && ability != moves.dodge && !mapOpen)
            startMove(moves.dodge);
        else if (gameInput.pressed(gameoptions::SPECIAL_ATTACK)) {
            if (mouseLook) startMove(moves.special);
            else {
                rightDownAt = time;
                pointerPosition(&rightDownX, &rightDownY);
            }
        }
        for (int button : input.released) {
            // Identification outcomes fall through to this cached-target acceptance. There is
            // no additional inventory, mode, source-item, stun or matching-down requirement.
            if (const auto destination = mapSelection.destination(button, [&](auto cell) { return usedCells.count(cell) != 0; });
                destination && spawned.startShrineReturn((*destination)[0], (*destination)[1]))
                mapOpen = false;
            const bool special = GameInput::buttonMatches(gameControls[gameoptions::SPECIAL_ATTACK], button);
            if (special && mouseLook && moves.releaseSpecial && ability == moves.special) {
                ability = 0;
                startMove(moves.releaseSpecial);
            } else if (special && rightDownAt >= 0) {
                double rx, ry;
                pointerPosition(&rx, &ry);
                if (time - rightDownAt < .3 && std::abs(rx - rightDownX) + std::abs(ry - rightDownY) < 8)
                    startMove(moves.special);
                rightDownAt = -1;
            }
        }
        if (autoUse && time > 3) {
            autoUse = false;
            autoUsePress = true;
        }
        // The use key (E; UsePrompt.h, docs/specs/use-prompt.md): what it aims at and would do
        // this frame, shown on its quick button, and on the frame it goes down, what it does.
        // Only the native player has one. The crosshair's view is last frame's camera.
        usePrompt = {};
        useIcon.clear();
        auto selectedItemUse = [&](nativeai::Creature &actor) {
            if (!nativeInventory)
                return;
            const auto choices = itemselector::candidates(nativeInventory->inventory);
            if (choices.empty())
                return;
            const auto index = choices[itemselector::wrappedIndex(itemSelector.index, choices.size())];
            const auto result = inventoryslots::useItem(*nativeInventory, index, actor.record);
            if (result.interaction) {
                auto request = nativeinteract::fresh(nativeinteract::USE_ITEM);
                std::memcpy(request.data() + nativeinteract::ITEM, result.interaction->data(), nativesaves::ITEM_SIZE);
                actor.interactions.push_back(request);
            }
            if (result.action == inventoryslots::Action::Map)
                mapSelection.useItem(nativeInventory->inventory.slot(index));
            itemSelector.hide(); // a nonempty selection closes even if the helper rejects it
            rebuildInventoryViews();
        };
        auto itemInputHeld = [&](int action) {
            return !capturingBeforeInput &&
                   (GameInput::physicalBindingHeld(window, gameControls[(size_t)action]) || gameInput.gamepad().held[(size_t)action]);
        };
        auto *useActor = spawnedReady && nativeMove ? spawned.localPlayer() : nullptr;
        const bool admittedUseVisit = useActor && useprompt::inputAdmitted(useActor->record, capturingBeforeInput, chat.typing());
        if (auto *me = useActor; admittedUseVisit) {
            auto placeOf = [&](const uint8_t *record) {
                int64_t dots[3];
                std::memcpy(dots, record + worldgen::creature::POSITION, sizeof dots);
                return extractedWorld.toLocal(dots);
            };
            auto rawItem = [](const Gear::Item &it) {
                return nativeplayer::item(it.category, it.id, it.material, it.rarity, it.modifier, it.region[0], it.region[1], it.formula);
            };
            // How many of an item the bag holds (0x5e780): the same kind, modifier and material.
            auto carriedOf = [&](int category, int id, uint32_t modifier, int material) {
                int count = 0;
                for (const Carried &c : bag)
                    if (c.item.category == category && c.item.id == id && c.item.modifier == modifier && c.item.material == material)
                        count += c.count;
                return count;
            };
            useprompt::Scene scene;
            scene.player = me->record;
            scene.playerAt = placeOf(me->record);
            scene.abilityDuration = spawned.playerAbilityDuration();
            scene.camera = useCamera;
            spawned.forEach([&](const uint8_t *r) { scene.creatures.push_back({r, placeOf(r), nativerules::hostile(me->record, r)}); });
            scene.creatureById = [&](int64_t id) -> const uint8_t * { return spawned.record(id); };
            scene.sight = [&](glm::vec3 from, glm::vec3 to, float limit) {
                return useprompt::clearLine(from, to, limit,
                                            [&](int x, int y, int z) { return extractedWorld.solidAt(x + 0.5, y + 0.5, z + 0.5); });
            };
            scene.useDown = itemInputHeld(gameoptions::USE) || std::exchange(autoUsePress, false);
            scene.dodgeHeld = gameInput.held(gameoptions::DODGE);
            scene.walkHeld = gameInput.held(gameoptions::WALK);
            scene.dialogOpen = generalDialogue.visible();
            scene.selectorShown = itemSelector.shown;

            // The static objects of the zones within 8 blocks of the player (zone +0x60), each
            // as the world keeps it now: a door shut or open, a seat's user, an item given.
            const double blockX = (double)scene.playerAt.x + (double)(extractedWorld.originDotsX() >> 16);
            const double blockY = (double)scene.playerAt.y + (double)(extractedWorld.originDotsY() >> 16);
            auto zoneOf = [](double block) { return (int32_t)std::floor(block / 64.0); };
            std::vector<worldgen::ZoneObject> sceneObjects; // beside scene.objects
            for (int32_t zx = zoneOf(blockX - 8); zx <= zoneOf(blockX + 8); zx++)
                for (int32_t zy = zoneOf(blockY - 8); zy <= zoneOf(blockY + 8); zy++) {
                    std::vector<worldgen::ZoneObject> objects;
                    if (!extractedWorld.gameObjects(zx, zy, objects))
                        continue;
                    const std::vector<nativeai::Object> *kept = spawned.zoneObjects(zx, zy);
                    for (size_t i = 0; i < objects.size(); i++) {
                        const worldgen::ZoneObject &o = objects[i];
                        useprompt::Object u;
                        const int64_t dots[3] = {o.x, o.y, o.z};
                        u.at = extractedWorld.toLocal(dots);
                        if (glm::dot(u.at - scene.playerAt, u.at - scene.playerAt) > 16.0f)
                            continue; // (beyond the use key's 4 blocks)
                        u.zoneX = zx, u.zoneY = zy, u.index = (int32_t)i, u.type = o.type;
                        const worldgen::ObjectSize base = worldgen::objectSize(o.type);
                        u.size = glm::vec3(base.x, base.y, base.z) * o.scale;
                        u.quarterTurn = (o.turn & 1) != 0;
                        if (kept && i < kept->size()) {
                            u.active = (*kept)[i][0x28] != 0;
                            std::memcpy(&u.occupant, (*kept)[i].data() + 0x30, 8);
                        }
                        if (const worldgen::Item &w = o.takes; w.category) {
                            Gear::Item it;
                            it.category = w.category, it.id = w.id, it.material = w.material, it.rarity = w.rarity, it.modifier = w.modifier;
                            it.region[0] = w.region.x, it.region[1] = w.region.y, it.formula = (int)w.realm;
                            u.item = rawItem(it);
                            if (kept && i < kept->size())
                                std::memcpy(u.item.data(), (*kept)[i].data() + nativeinteract::OBJECT_ITEM, nativesaves::ITEM_SIZE);
                            u.itemCount = kept && i < kept->size() ? (*kept)[i][0xd8] : o.takesCount;
                            u.itemName = itemName(it);
                            u.carried = nativeInventory ? nativeInventory->inventory.quantity(u.item)
                                                        : carriedOf(w.category, w.id, w.modifier, w.material);
                            u.active = u.active && !usedObjects.count({zx, zy, (int32_t)i});
                        }
                        if (o.type == 0x16) { // a chest: opened once its cell is a known place
                            u.chestCellKnown = usedCells.count(lore::cellOf(o.x, o.y)) != 0;
                            u.chestHasItem = o.reward.category != 0;
                        }
                        scene.objects.push_back(u);
                        sceneObjects.push_back(o);
                    }
                }

            // The items lying within 4 blocks (zone +0x30, and what deaths dropped).
            std::vector<Loot *> sceneLoot; // beside scene.groundItems
            for (Loot &l : loot) {
                if (!l.onGround || !l.hasItem || glm::dot(l.at - scene.playerAt, l.at - scene.playerAt) > 16.0f)
                    continue;
                useprompt::GroundItem g;
                g.type = l.item.category, g.subtype = l.item.id;
                g.at = l.at;
                g.settleMs = l.dropMs;
                g.noAutoPickup = (l.flags & grounditems::ground::STAYS_FOR_OTHERS) != 0;
                g.pending = l.taken;
                g.carriesOne = carriedOf(l.item.category, l.item.id, l.item.modifier, l.item.material) > 0;
                const int64_t dots[2] = {(int64_t)std::floor((double)l.at.x * 65536.0) + extractedWorld.originDotsX(),
                                         (int64_t)std::floor((double)l.at.y * 65536.0) + extractedWorld.originDotsY()};
                g.cellKnown = usedCells.count(lore::cellOf(dots[0], dots[1])) != 0;
                g.ownsSpecial = std::any_of(bag.begin(), bag.end(), [&](const Carried &c) { return c.item.category == 0x18 && c.item.id == l.item.id; });
                for (const Carried &c : bag) // (stacked as collect stacks them)
                    if (c.item.category == l.item.category && c.item.id == l.item.id && c.item.material == l.item.material &&
                        c.item.rarity == l.item.rarity && c.item.modifier == l.item.modifier)
                        g.carriedCount += c.count;
                if (l.model >= 0 && world.modelMesh(l.model)) {
                    const glm::vec3 size = world.modelSize(l.model);
                    const glm::mat4 m = dropPlacement(l, size, time);
                    for (size_t k = 0; k < 8; k++)
                        g.box[k] = glm::vec3(m * glm::vec4(useprompt::boxCorners(glm::vec3(0), size)[k], 1.0f));
                    g.hasModel = true;
                }
                scene.groundItems.push_back(g);
                sceneLoot.push_back(&l);
            }

            // The equipped vehicle (chardata +0x148): CubeVulkan has no such slot, so the first
            // special item (category 0x18) carried for this region stands in for it.
            const worldgen::Int2 region = regionHere();
            const Carried *vehicle = nullptr;
            for (const Carried &c : bag)
                if (c.item.category == 0x18 && c.item.region[0] == region.x && c.item.region[1] == region.y && (!vehicle || c.item.id < vehicle->item.id))
                    vehicle = &c;
            if (vehicle) {
                scene.vehicle = rawItem(vehicle->item);
                scene.vehicleName = itemName(vehicle->item);
            }

            const useprompt::Targets targets = useprompt::chooseTargets(scene);
            usePrompt = useprompt::decide(scene, targets, useKey);
            const useprompt::Prompt &p = usePrompt;

            // A creature aimed at may mark its mission's zone on the map (spec 2.6).
            if (const std::optional<std::array<int32_t, 2>> zone = useprompt::markedZone(spawned.record(targets.creature));
                zone && !missionKnown(*zone)) {
                saveGame.marked.push_back({(*zone)[0], (*zone)[1], "", ""});
                if (discoveredPlaces.insert(*zone).second)
                    saveGame.discoveredPlaces.push_back(*zone);
                saveGame.missionTimers[*zone] = mapmarks::TALK_NOTE_MS;
            }
            // Coins and the like are picked up by walking over them (spec 2.4).
            for (int i : targets.walkedOver)
                sceneLoot[(size_t)i]->taken = true;

            // The icon on the button.
            if (p.icon == useprompt::Prompt::Icon::ObjectItem)
                useIcon = spawned.itemSprite(scene.objects[(size_t)p.object].item.data());
            else if (p.icon == useprompt::Prompt::Icon::GroundItem) {
                const Loot &l = *sceneLoot[(size_t)p.groundItem];
                useIcon = !l.item.sprite.empty() ? l.item.sprite : spawned.itemSprite(rawItem(l.item).data());
            } else if (p.icon == useprompt::Prompt::Icon::Vehicle)
                useIcon = spawned.itemSprite(scene.vehicle.data());

            if (p.pressed) {
                using useprompt::Action;
                logLine("Use key: '" + p.text + "'" + (p.creature ? ", creature " + std::to_string(p.creature) : std::string()) +
                        (p.object >= 0 ? ", object type " + std::to_string(scene.objects[(size_t)p.object].type) : std::string()));
                const uint8_t *creature = spawned.record(p.creature);
                int race = 0;
                if (creature)
                    std::memcpy(&race, creature + worldgen::creature::RACE, 4);
                // The overworld events' creatures answer for themselves (OverworldEvents.h).
                bool answered = false;
                if (creature && events.enabled() && (p.action == Action::Converse || p.action == Action::Activate)) {
                    OverworldEvents::Said said;
                    auto w = eventWorld();
                    if (events.interact(w, p.creature, said)) {
                        answered = true;
                        if (!said.line.empty()) {
                            Talk t;
                            int speakerRace = 0;
                            if (const uint8_t *who = spawned.record(said.speaker))
                                std::memcpy(&speakerRace, who + worldgen::creature::RACE, 4);
                            t.npc = Speaker{true, SIZE_MAX, said.speaker, speakerRace};
                            t.lines.push_back(said.line);
                            talk = t;
                        }
                    }
                }
                if (!answered && p.action != Action::Converse)
                    useprompt::applyToPlayer(p, scene, *me);
                const glm::vec3 creatureAt = creature ? placeOf(creature) : scene.playerAt;
                switch (answered ? Action::None : p.action) {
                case Action::Revive:
                    reviveNow = true;
                    break;
                case Action::ExaminePetrified:
                    showDialog({"define_model", std::to_string(race), "he_is_petrified"}, (uint32_t)(p.creature % 100000));
                    break;
                case Action::Shop:
                    vendorTrade = {true, shopkeeperOf(p.creature)};
                    identify.close(), upgrade.open = false; // (they stand where the shop's widget goes)
                    inventoryOpen = hud.hasInventoryArt() && haveGear;
                    bagShown = true;
                    craftingOpen = false;
                    refreshBagOpening();
                    refreshInventory(); // shop rebuild's bound widget refresh
                    if (!nativeInventory)
                        inventoryView.vendor.tab = 0, inventoryView.vendor.scroll = 0;
                    logLine("Use: trading with " + std::to_string(p.creature) + " (class " + std::to_string(vendorTrade.shop.cls) + ")");
                    break;
                case Action::ClassWindow: // the identify window beside the inventory (loot-shops.md §5)
                    inventoryOpen = hud.hasInventoryArt() && haveGear;
                    bagShown = true;
                    vendorTrade.open = upgrade.open = false;
                    if (inventoryOpen) {
                        identify.show();
                        craftingOpen = false;
                        refreshBagOpening();
                    }
                    logLine("Use: the identify window of " + std::to_string(p.creature));
                    break;
                case Action::FlightMaster:
                    mapOpen = true;
                    flightmasterMode = true;
                    break;
                case Action::Converse:
                    startTalk(Speaker{true, SIZE_MAX, p.creature, race});
                    spawned.activate(p.creature);
                    break;
                case Action::Shrine:
                    mapSelection.openShrines();
                    logLine("Use: shrine selection");
                    break;
                case Action::Crafting: {
                    const bool wasOpen = inventoryOpen;
                    craftingOpen = !(inventoryOpen && craftingOpen);
                    if (!wasOpen)
                        bagShown = false;
                    inventoryOpen = (bagShown || craftingOpen) && hud.hasInventoryArt() && haveGear;
                    if (!inventoryOpen)
                        craftingOpen = false, bagShown = true;
                    logLine("Use: crafting at station mode " + std::to_string(p.station));
                    break;
                }
                case Action::GiveItem: {
                    const useprompt::Object &o = scene.objects[(size_t)p.object];
                    const worldgen::Item &w = sceneObjects[(size_t)p.object].takes;
                    int left = o.type == 9 ? 0 : o.itemCount; // (type 9 keeps the item)
                    if (nativeInventory && o.type != 9) {
                        nativeInventory->inventory.takeCreature(o.item, (uint8_t)o.itemCount);
                        rebuildInventoryViews();
                        left = 0;
                    }
                    for (auto it = bag.begin(); it != bag.end() && left > 0;) {
                        if (it->item.category == w.category && it->item.id == w.id && it->item.modifier == w.modifier &&
                            it->item.material == w.material) {
                            const int taken = std::min(left, it->count);
                            left -= taken, it->count -= taken;
                            if (it->count <= 0) {
                                it = bag.erase(it);
                                continue;
                            }
                        }
                        ++it;
                    }
                    const std::array<int32_t, 3> key{o.zoneX, o.zoneY, o.index};
                    if (usedObjects.insert(key).second)
                        saveGame.usedObjects.push_back(key);
                    spawned.setObjectActive(o.zoneX, o.zoneY, (size_t)o.index, false);
                    audio.playInterface(soundrules::soundid::DROP_ITEM);
                    logLine("Use: gave object " + std::to_string(o.type) + " its item (" + o.itemName + ")");
                    break;
                }
                case Action::OpenChest: {
                    chestToOpen = sceneObjects[(size_t)p.object];
                    const useprompt::Object &o = scene.objects[(size_t)p.object];
                    spawned.setObjectActive(o.zoneX, o.zoneY, (size_t)o.index, false); // (an opened chest stays open)
                    break;
                }
                case Action::Customize:
                    logLine("Use: the customization bench's window is not ported");
                    break;
                case Action::ExamineLore: {
                    const worldgen::ZoneObject &o = sceneObjects[(size_t)p.object];
                    const lore::Cell cell = lore::cellOf(o.x, o.y);
                    startTale(Examined{lore::examine(cell).value_or(lore::Reading{cell, {}}), scene.objects[(size_t)p.object].at});
                    break;
                }
                case Action::GoddessGift:
                    showDialog({"gift_of_the_goddess"}, chatter() % 10000);
                    break;
                case Action::UseSelectedItem:
                    selectedItemUse(*me);
                    break;
                case Action::PickUpItem:
                    sceneLoot[(size_t)p.groundItem]->taken = true;
                    break;
                case Action::CannotCarry:
                    texts.push_back({player.position + glm::vec3(0, 0, 3.1f), "You can't carry more of these items.", {255, 51, 51, 255}});
                    break;
                case Action::BagFull: {
                    static const std::map<int, const char *> full = {{0, "Your equipment bag is full."}, {2, "Your item bag is full."},
                                                                     {3, "Your ingredient bag is full."}, {4, "Your pet bag is full."}};
                    if (auto f = full.find(p.bag); f != full.end())
                        texts.push_back({player.position + glm::vec3(0, 0, 3.1f), f->second, {255, 51, 51, 255}});
                    break;
                }
                default:
                    break;
                }
            }
        }
        if (nativeInventory && admittedUseVisit)
            if (auto *me = useActor) {
                const bool held = itemInputHeld(gameoptions::QUICK_ITEM);
                if (held && !quickItemWasDown)
                    selectedItemUse(*me);
                quickItemWasDown = held;
            }
        if (!inventoryOpen) {
            if (gameInput.pressed(gameoptions::CLASS_SKILL))
                startMove(moves.skill);
            if (gameInput.pressed(gameoptions::ULTIMATE_SKILL))
                startMove(moves.ultimate);
            for (int key : movesPressed)
                if (key == GLFW_KEY_P && !GameInput::keyIsBound(gameControls, key)) // (CubeVulkan's tap skill key)
                    startMove(moves.tap);
        }
        // Native choice activation runs after normal GUI release dispatch and
        // uses the last draw's hovered row, including releases outside the panel.
        if (std::find(input.released.begin(), input.released.end(), GLFW_MOUSE_BUTTON_LEFT) != input.released.end()) {
            const bool speakerExists = spawnedReady && spawned.record(answers.speaker()) != nullptr;
            const auto action = answers.release(speakerExists, playerGold, hour * 3600000.f, (uint8_t)playerSpec);
            if (action.kind == dialoguechoice::Action::Rest) {
                playerGold -= action.coins;
                auto request = nativeinteract::fresh(5);
                std::memcpy(request.data() + nativeinteract::TARGET, &action.speaker, sizeof action.speaker);
                spawned.queueInteraction(spawned.playerId(), request);
            } else if (action.kind == dialoguechoice::Action::ChangeSpecialization) {
                playerSpec = action.specialization;
                setSpecializationMetadata(playerSpec);
                moves = classMoves(playerCls, playerSpec);
                ATTACK_A = moves.attack[0]; ATTACK_B = moves.attack[1];
                if (haveGear) applyGear();
                else if (nativePlayer) spawned.setPlayerGear(nativeRecord(-1, nullptr));
                TalkScript confirmation;
                confirmation.tokens = {"define_specialization", std::to_string(playerCls), std::to_string(playerSpec),
                                       "your_specialization_is_now"};
                std::mt19937 feedbackRandom((uint32_t)action.speaker);
                auto feedback = playScript(confirmation, speech, feedbackRandom, &talkNames, generalDialogue.pages());
                generalDialogue.applyCompilation(std::move(feedback.pages));
            }
        }
        movesPressed.clear();
        bool holdingAttack = !inventoryOpen && !answers.visible() && gameInput.held(gameoptions::ATTACK);
        if (!ability && ATTACK_A && (attackQueued || holdingAttack)) {
            startMove(sinceAttack < 1.2 && lastAttack == ATTACK_A ? ATTACK_B : ATTACK_A);
            lastAttack = ability;
            sinceAttack = 0;
            attackQueued = false;
        }
            input.finishPointerCallbacks();
            input.buttons.clear();
        input.released.clear();
        if (poseAbility > 0) {
            ability = (int)poseAbility;
            abilityTime = poseTime - dt;
        }
        if (ability && !nativeMove) {
            abilityTime += dt;
            if (mouseLook && ability == moves.special && moves.releaseSpecial &&
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
                abilityTime = std::min(abilityTime, std::max(0., character.abilityDuration(ability) - .001));
            // The roll carries the character with the speed the game recorded.
            float roll = character.abilitySpeed(ability, abilityTime);
            if (roll > 0 && !player.fly) {
                glm::vec2 d(std::cos(heading), std::sin(heading));
                glm::vec2 m = d * roll * (float)dt;
                if (!useExtracted || !blocked(player.position.x + m.x, player.position.y, player.position.z))
                    player.position.x += m.x;
                if (!useExtracted || !blocked(player.position.x, player.position.y + m.y, player.position.z))
                    player.position.y += m.y;
            }
        }

        // Character motion from what actually happened this frame.
        {
            glm::vec2 moved = glm::vec2(player.position - before);
            float step = glm::length(moved);
            auto now = player.verticalSpeed != 0 && !player.fly ? CharacterRecording::Motion::Air
                       : step > .001f                        ? CharacterRecording::Motion::Walk
                                                             : CharacterRecording::Motion::Idle;
            if (step > .001f && ability == 0 && !nativeMove)
                heading = std::atan2(moved.y, moved.x);
            motionTime = now == motion ? motionTime + dt : 0;
            motion = now;
            walked += step;
        }

        // Ground items (loot-pickup.md §1): wall items keep to their wall, the timers count down
        // and an item whose pick-up delay ends lands. Picked up ones are received. Loot pouches
        // (CubeVulkan's quest rewards): walking over one picks it up. (In the generated world
        // the ground items are the host's zones', taken through the pick-up interaction.)
        for (auto it = loot.begin(); it != loot.end();) {
            it->age += dt;
            glm::vec3 d = it->at - player.position;
            if (it->onGround && it->item.category == grounditems::type::WALL_ITEM && useExtracted) {
                int64_t at[3];
                toUnits(it->at, at);
                const double ox = (double)(extractedWorld.originDotsX() >> 16), oy = (double)(extractedWorld.originDotsY() >> 16);
                grounditems::snapToWall(at[0], at[1], it->rotation, at[2], [&](int32_t x, int32_t y, int32_t z) {
                    return extractedWorld.solidAt(x - ox + .5, y - oy + .5, z + ExtractedWorld::Z_OFFSET + .5);
                });
                const glm::vec3 snapped = extractedWorld.toLocal(at);
                it->at.x = snapped.x, it->at.y = snapped.y;
            }
            const bool delayEnded = grounditems::countDown(it->dropMs, it->timerMs, (float)dt * 1000.0f);
            if (delayEnded && it->onGround && !serverMode) {
                // Its pick-up delay ran out: it lands with a sound (sound.md §5.2).
                const soundrules::LandingSound landed = soundrules::itemLanded(it->item.category, it->forPlayer, clientRand);
                int64_t at[3];
                toUnits(it->at, at);
                audio.playAt(landed.number, at, 1.0f, landed.pitch);
                if (landed.chime)
                    audio.playAt(soundrules::soundid::CRAFT_SPECIAL, at, 1.0f, 1.0f);
            }
            // A dropped item is taken with the use key, the nearest within 10 blocks (the
            // game's input); a pouch (CubeVulkan's quest rewards) by walking over it.
            const bool take = it->onGround ? it->taken && it->dropMs <= 0
                                           : it->age > .6 && glm::length(glm::vec2(d)) < 1.8f && std::abs(d.z) < 2.5f;
            if (!serverMode && take) {
                if (it->zoneItem[0] != INT_MIN && session.clientGame()) {
                    // A zone's ground item is its world's: picking it up is an interaction (kind 7)
                    // the host performs; its pick-up notice then gives it to this player
                    // (zone-object-interaction.md §6, loot-pickup.md §4). Asked again after a second
                    // while it is still there.
                    if (time - it->asked > 1.0) {
                        it->asked = time;
                        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::PICK_UP);
                        std::memcpy(in.data() + nativeinteract::ITEM, it->raw.data(), it->raw.size());
                        std::memcpy(in.data() + nativeinteract::ZONE_X, &it->zoneItem[0], 4);
                        std::memcpy(in.data() + nativeinteract::ZONE_Y, &it->zoneItem[1], 4);
                        std::memcpy(in.data() + nativeinteract::OBJECT, &it->zoneItem[2], 4);
                        spawned.queueInteraction(spawned.playerId(), in);
                    }
                    ++it;
                    continue;
                }
                const int count = it->notice ? receivePickup(*it) : 1;
                for (int k = 0; k < count; k++)
                    collect(*it);
                // An item that stays for others (flag bit 1) stays in its zone's list.
                const bool stays = (it->flags & grounditems::ground::STAYS_FOR_OTHERS) != 0;
                if (!stays && it->zoneItem[0] != INT_MIN && takenZoneItems.insert(it->zoneItem).second)
                    saveGame.takenItems.push_back({it->zoneItem[0], it->zoneItem[1], it->zoneItem[2]});
                if (!stays && it->zoneItem[0] != INT_MIN && spawnedReady) // (the zone's own list, which world.db keeps)
                    spawned.takeGroundItem(it->zoneItem[0], it->zoneItem[1], (size_t)it->zoneItem[2]);
                it = loot.erase(it);
            } else if (!it->onGround && it->age > 300)
                it = loot.erase(it); // A pouch left behind: gone after five minutes. (Ground items stay in their zone.)
            else
                ++it;
        }
        // An attack lands partway through the swing on monsters just in front.
        if (dealsDamage(ability) && !attackLanded && abilityTime >= .2) {
            attackLanded = true;
            bool manaGained = false;
            glm::vec2 facing(std::cos(heading), std::sin(heading));
            for (size_t k = 0; k < actors.count(); k++) {
                glm::vec3 d = actors.position(k) - player.position;
                float flat = glm::length(glm::vec2(d));
                if (!actors.hittable(k) || flat > moves.reach || std::abs(d.z) > 2.5f ||
                    (flat > .5f && glm::dot(glm::vec2(d) / flat, facing) < .2f))
                    continue;
                const Gear::Item &weapon = gear.equipped(Gear::WeaponRight);
                int weaponId = haveGear ? (weapon.category == 3 ? weapon.id : -1) : character.weaponId;
                combat::Attacker attacker{playerAttack, playerAttack * spellPerAttack, playerCritical, ability, playerCombo,
                                          combat::maxCombo(weaponId), playerCharge, &playerEffects};
                combat::Hit h = combat::hit(attacker, actors.defense(k), dice);
                float hit = h.damage * attackMultiplier;
                if (hit > 0) {
                    playerCombo++, sinceLanded = 0;
                    playerHp = std::min(playerMaxHp, playerHp + h.healed * attackMultiplier); // War Frenzy
                    double length = character.abilityDuration(ability);
                    if (chosen && chosen->classType == 1 && length > 0 && combat::warriorFrenzy(length, dice))
                        playerEffects.add(combat::WAR_FRENZY, 0, 8);
                    if ((float)combat::nativeRand(dice) / 32767 <
                                            combat::stunChance(ability, playerCharge, true, 0, actors.flags2(k)))
                        actors.stun(k, combat::stunSeconds(false));
                }
                struck = (int)k;
                // Hits fill mana: the game's rule needs the recorded mana generation (newer
                // captures); older ones keep the logged +0.2 per basic hit.
                if (isAttack(ability) && !manaGained) {
                    manaGained = true; // once per attack
                    double length = character.abilityDuration(ability);
                    playerMp = std::min(1.f, playerMp + (character.manaGeneration > 0 && length > 0
                                                             ? combat::manaFromHit(ability, length, character.manaGeneration, dice)
                                                             : moves.mpPerHit));
                }
                floating.push_back({actors.position(k) + glm::vec3(0, 0, 2.4f), (int)std::round(hit), 0, false});
                if (h.poisons)
                    actors.poison(k, h.poison * attackMultiplier);
                if (actors.hurt(k, hit)) {
                    logLine("Defeated a monster (" + std::to_string((int)actors.maxHealth(k)) + " HP) at " + std::to_string(time) + " s");
                    defeated(k);
                }
            }
            // The generated world's creatures: the hit is the game's (0x29eb60 on their
            // records), worth what CubeVulkan's player attack makes it. The client world only
            // makes its damage number, which the host applies; it is shown with the others.
            {
                for (const SpawnedCreatures::Target &t : spawned.targets(extractedWorld, player.position, moves.reach + 2)) {
                    glm::vec3 d = t.at - player.position;
                    float flat = glm::length(glm::vec2(d));
                    if (flat > moves.reach || std::abs(d.z) > 2.5f || (flat > .5f && glm::dot(glm::vec2(d) / flat, facing) < .2f))
                        continue;
                    const bool magic = combat::magic(ability);
                    const bool isCritical = combat::critical(playerCritical, magic ? t.resistance : t.armor, dice);
                    const float power = combat::usesSpellPower(ability) ? playerAttack * spellPerAttack : playerAttack;
                    const float out = combat::outgoing(power, ability, playerCharge, isCritical, dice) * attackMultiplier;
                    const int type = ability == 31 || ability == 32 ? 1 : ability == 33 || ability == 34 ? 2 : 0;
                    const glm::vec2 dir = flat > .01f ? glm::vec2(d) / flat : facing;
                    const float lost = spawned.strike(t.id, out, isCritical, playerCharge, ability, type, dir);
                    if (lost < 0)
                        continue;
                    if (lost > 0)
                        playerCombo++, sinceLanded = 0;
                    if (isAttack(ability) && !manaGained) {
                        manaGained = true;
                        double length = character.abilityDuration(ability);
                        playerMp = std::min(1.f, playerMp + (character.manaGeneration > 0 && length > 0
                                                                 ? combat::manaFromHit(ability, length, character.manaGeneration, dice)
                                                                 : moves.mpPerHit));
                    }
                }
            }
        }
        // Poison keeps hitting (ability 18): every 400 ms, through armor.
        for (auto [k, hit] : actors.takePoisonHits()) {
            floating.push_back({actors.position(k) + glm::vec3(0, 0, 2.4f), (int)std::round(hit), 0, false});
            if (actors.hurt(k, hit)) {
                logLine("A monster died of poison at " + std::to_string(time) + " s");
                defeated(k);
            }
        }
        if (actors.count()) {
            float taken = 0;
            double stunTaken = 0;
            auto floor = [&](float x, float y, float z, float &h) { return extractedWorld.floorAt(x, y, z, HEADROOM, h); };
            actors.update(dt, player.position, floor, guardedDefense(), taken, &stunTaken);
            if (stunTaken > 0 && playerStunned <= 0 && sinceStunned >= 3 && !playerEffects.find(combat::BULWARK))
                playerStunned = stunTaken, ability = 0;
            if (taken > 0) {
                playerHp -= taken;
                floating.push_back({player.position + glm::vec3(0, 0, 2.6f), (int)std::round(taken), 0, true});
                sinceHurt = 0;
            }
        }
        // Mana: the mage's regenerates (+0.1/s); everyone else's fades (-0.05/s), as
        // logged in the game. Stamina refills when no skill is draining it (rate
        // not measured yet: 4 s from empty).
        if (!(nativePlayer && !player.fly)) { // (the native player's are its record's: the game's upkeep)
            playerMp = moves.mpRegenerates ? std::min(1.f, playerMp + .1f * (float)dt) : std::max(0.f, playerMp - .05f * (float)dt);
            if (ability != moves.skill)
                playerStamina = std::min(1.f, playerStamina + .25f * (float)dt);
        }
        sinceHurt += dt;
        if ((sinceLanded += dt) > combat::COMBO_RESET)
            playerCombo = 0;
        playerEffects.update(dt);
        combat::refillBlock(playerBlock, dt, ability, combat::guarding(ability, playerCls, playerSpec, playerStunned > 0));
        if (playerStunned > 0 && (playerStunned -= dt) <= 0)
            sinceStunned = 0;
        else if (playerStunned <= 0)
            sinceStunned += dt;
        if (sinceHurt > 5)
            playerHp = std::min(playerMaxHp, playerHp + playerMaxHp * .02f * (float)dt);
        if (eatMs >= 0) { // eating: a share of the health every 200 ms of the food's use time
            const double before = eatMs;
            eatMs += dt * 1000.0;
            if ((int)eatMs / 200 != (int)before / 200 && (float)eatUse > (float)before && eatUse > 0) {
                const float heal = playerMaxHp * (200.0f / (float)eatUse);
                playerHp = std::min(playerMaxHp, playerHp + heal);
                texts.push_back({player.position + glm::vec3(0, 0, 2.6f), "+" + std::to_string((int)std::lround(heal)), {120, 255, 120, 255}});
            }
            if (eatMs >= eatUse)
                eatMs = -1;
        }
        if (nativePlayer && !player.fly && spawnedReady && spawned.playerMaxHealth() > 0) { // the native player's health is its record's (once it exists)
            playerMp = spawned.playerMana(), playerStamina = spawned.playerStamina(); // as are its mana and stamina
            playerHp = spawned.playerHealth();
            if (const float most = spawned.playerMaxHealth(); most > 0)
                playerMaxHp = most;
        }
        if (undying && playerMaxHp > 0) {
            playerHp = playerMaxHp;
            if (nativePlayer)
                spawned.setPlayerHealth(playerHp);
        }
        // The native player lies dead until the use key revives it (UsePrompt.h, "Revive"):
        // at the nearest Shrine of Life of its region
        // (the nearest shrine search without discovered markers: each place's shrine site,
        // within a land's span), healed.
        static bool deadWaiting = false;
        bool revive = playerHp <= 0;
        if (revive && nativePlayer && !player.fly && !autoAttack) {
            if (!deadWaiting)
                logLine("You died (the use key revives you at a Shrine of Life)");
            deadWaiting = true;
            revive = reviveNow;
        }
        reviveNow = false;
        if (revive && nativePlayer && !player.fly && useExtracted) {
            deadWaiting = false;
            const int64_t bx = (int64_t)std::floor(player.position.x) + (extractedWorld.originDotsX() >> 16);
            const int64_t by = (int64_t)std::floor(player.position.y) + (extractedWorld.originDotsY() >> 16);
            const worldgen::Int2 home = worldgen::regionAt((int32_t)bx, (int32_t)by);
            auto cellOf = [](int64_t b) { return (int32_t)std::floor((double)b / 16.0); };
            auto placeOf = [](int32_t cell) { return (int32_t)std::floor((double)cell / 64.0); };
            double best = -1;
            int32_t sx = 0, sy = 0;
            for (int32_t px = placeOf(cellOf(bx - 8192)); px <= placeOf(cellOf(bx + 8192)); px++)
                for (int32_t py = placeOf(cellOf(by - 8192)); py <= placeOf(cellOf(by + 8192)); py++) {
                    const int32_t cx = px * 64 + 1 + (int32_t)(worldgen::random((uint32_t)px + 148289u + (uint32_t)py * 57u) % 62);
                    const int32_t cy = py * 64 + 1 + (int32_t)(worldgen::random((uint32_t)px + 1464073u + (uint32_t)py * 57u) % 62);
                    if (!(worldgen::regionAt(cx * 16 + 8, cy * 16 + 8) == home))
                        continue;
                    const double dx = (double)(cx * 16 + 8 - bx), dy = (double)(cy * 16 + 8 - by);
                    if ((best < 0 || best > dx * dx + dy * dy) && worldgen::cellObject(cx, cy) == 53) // Shrine of Life
                        best = dx * dx + dy * dy, sx = cx, sy = cy;
                }
            if (best >= 0) {
                const float x = (float)(sx * 16 + 6 - (extractedWorld.originDotsX() >> 16));
                const float y = (float)(sy * 16 + 8 - (extractedWorld.originDotsY() >> 16));
                // The shrine's floor (the game's structure level, the structure level, is not
                // ported): under its roof, the first floor below the top one, if it is not
                // below the terrain there; else the top.
                const float ground = worldgen::groundHeight(sx * 16 + 6, sy * 16 + 8) + (float)ExtractedWorld::Z_OFFSET - 1.0f; // the terrain (no buildings)
                float floor = floorUnder(x, y, 400.0f);
                if (const float under = floorUnder(x, y, floor - 3.0f); under < floor - 0.5f && under >= ground)
                    floor = under;
                respawn = glm::vec3(x, y, floor);
                logLine("Reviving at the Shrine of Life of cell " + std::to_string(sx) + " " + std::to_string(sy) + ", " +
                        std::to_string((int)std::sqrt(best)) + " blocks away, at " + std::to_string(respawn.x) + " " +
                        std::to_string(respawn.y) + " " + std::to_string(respawn.z) + " (terrain " + std::to_string(ground) + ")");
            }
        }
        if (revive) {
            // Revived: a tombstone on the map where the player died (MapMarks.h).
            placeTombstone(0.0f);
            // Back to where this world was first entered (the native player: the shrine), healed.
            logLine("You died; respawning at home");
            player.position = respawn;
            player.verticalSpeed = 0;
            playerHp = playerMaxHp;
            sinceHurt = 99;
            if (nativePlayer)
                spawned.setPlayerHealth(playerHp);
        }

        if (!session.started()) { // (otherwise the clock is the host's, packet 8)
            float next = hour + minutesPerSecond * (float)dt / 60;
            if (next >= 24)
                day++;
            hour = std::fmod(next, 24.f);
        }
        mods.update(dt);
        for (const Multiplayer::Notice &n : session.takeNotices()) // ("You were invited by" in the game's orange)
            texts.push_back({player.position + glm::vec3(0, 0, 3), n.text,
                             n.invitation ? glm::u8vec4(255, 128, 51, 255) : glm::u8vec4(120, 220, 255, 255)});
        for (const cwnet::ChatLine &line : session.takeChat()) chat.receive(line.speaker, line.text);
        hud.updateChat(chat, [&](int64_t id) -> std::optional<std::u16string> {
            if (const uint8_t *record = spawned.record(id)) {
                std::u16string name;
                for (int i = 0; i < 16 && record[0x958 + i]; ++i) name.push_back((char16_t)record[0x958 + i]);
                return name;
            }
            return {};
        });
        // The addressed feat consumer follows the ordinary received-chat visit.
        if (!captureFeat.empty() && !featFixtureQueued && time >= 2 && spawned.playerRecord()) {
            ++pendingFeatDisplays;
            featFixtureQueued = true;
        }
        if (const auto *me = spawned.playerRecord())
            while (pendingFeatDisplays) {
                --pendingFeatDisplays;
                feattext::Point physical;
                std::memcpy(physical.data(), me + 0x10, sizeof physical);
                featTexts.received(physical);
                hud.appendChat(chat, u"FEAT!\n", {200, 100, 255});
                audio.playInterface(feattext::SOUND);
            }
        bool chatFontChanged = hud.requireText(chat.text());
        for (const auto &row : chat.rows())
            for (const auto &word : row)
                chatFontChanged = hud.requireText(std::u16string(word.text.begin(), word.text.end())) || chatFontChanged;
        if (chatFontChanged) hud.layout(renderer, renderer.screenSize());
        // A creature that just died bursts into the voxels it was drawn with (only a
        // fresh death: a player joining later sees no bursts for older ones).
        wasDead.resize(actors.count(), 0);
        for (size_t k = 0; k < actors.count(); k++) {
            bool dead = !actors.visible(k);
            if (dead && !wasDead[k] && actors.deathTime(k) < 1 && glm::length(actors.position(k) - player.position) < renderSettings.fogDistance) {
                actors.pose(k, actorPose);
                auto &models = creatureModels[actors.recordingOf(k)];
                size_t pieceCount = 0;
                for (size_t i = 0; i < actorPose.size() && i < models.size(); i++) {
                    if (actorPose[i][3][3] == 0 || models[i] < 0)
                        continue;
                    glm::vec3 size = world.modelSize(models[i]);
                    glm::vec3 origin = partCorner ? glm::vec3(size.x * .5f, size.y * .5f, 0) : glm::vec3(0, 0, -size.z * .5f);
                    glm::mat4 m = actorPose[i] * glm::translate(glm::mat4(1), origin); // as drawn
                    float voxel = glm::length(glm::vec3(m[0]));
                    if (voxel <= 0)
                        continue;
                    std::vector<DeathEffects::Piece> pieces;
                    for (auto &v : world.voxels(models[i]))
                        pieces.push_back({glm::vec3(m * glm::vec4(v.at, 1)), v.color, voxel, glm::mat3(m) / voxel});
                    pieceCount += pieces.size();
                    glm::vec3 middle = glm::vec3(m * glm::vec4(0, 0, size.z * .5f, 1));
                    deathFx.burst(pieces, middle);
                }
                logLine("Death burst: " + std::to_string(pieceCount) + " exposed voxels at " + std::to_string(time) + " s");
                static bool firstBurst = true;
                if (firstBurst && !captureDeath.empty())
                    deathShots = {{time + .3, captureDeath + "-0.3s.png"}, {time + 1.2, captureDeath + "-1.2s.png"}};
                firstBurst = false;
            }
            wasDead[k] = dead;
        }
        if (!deathShots.empty() && time >= deathShots.front().first) {
            renderer.capture(deathShots.front().second);
            logLine("Death capture: " + deathShots.front().second);
            deathShots.erase(deathShots.begin());
        }
        deathFx.update(dt, [&](float x, float y, float z, float &h) {
                if (useExtracted)
                    return extractedWorld.floorAt(x, y, z, HEADROOM, h);
                h = groundAt(x, y);
                return true;
            }, [&](glm::dvec3 at) {
                return useExtracted ? extractedWorld.solidAt(at.x, at.y, at.z)
                                    : at.z < groundAt((float)at.x, (float)at.y);
            });
        // Zones whose blocks changed are meshed again.
        for (const auto &[ax, ay] : extractedWorld.takeChangedZones())
            world.refreshZone(ax - extractedWorld.originZoneX(), ay - extractedWorld.originZoneY());
        if (!serverMode) // a dedicated server draws nothing, so it streams no meshes
            world.update(player.position);

        { // The camera's smoothing (0.005 ^ (cameraSmoothness / 100), frontend-title.md §3.3).
            const float s = gamemath::blendFactor(dt * 1000.0, (float)gameOptions.smoothingConstant());
            viewYaw += (player.yaw - viewYaw) * s;
            viewPitch += (player.pitch - viewPitch) * s;
            viewZoom += (player.zoom - viewZoom) * s;
        }
        glm::vec3 target = player.position + glm::vec3(0, 0, 1.6f);
        glm::vec3 eye = target + viewZoom * glm::vec3(std::cos(viewYaw) * std::cos(viewPitch), std::sin(viewYaw) * std::cos(viewPitch),
                                                      std::sin(viewPitch));
        if (useExtracted) {
            // Pull the camera in front of walls and roofs between it and the player.
            glm::vec3 toEye = eye - target;
            float length = glm::length(toEye);
            for (float t = .3f; t < length; t += .2f) {
                glm::vec3 p = target + toEye * (t / length);
                if (extractedWorld.solidAt(p.x, p.y, p.z)) {
                    eye = target + toEye * (std::max(.3f, t - .3f) / length);
                    break;
                }
            }
        } else
            eye.z = std::max(eye.z, groundAt(eye.x, eye.y) + .8f);
        { // Sounds are heard from the camera's eye (sound.md §2).
            int64_t ear[3];
            toUnits(eye, ear);
            audio.setListener(ear);
            // An explosion's shake (§2.1) jolts the eye. HOOK: the spec gives how much shake an
            // explosion raises, not how the camera applies it or lets it die down; these two
            // constants stand in until it does.
            if (const float shake = audio.cameraShake(); shake > 0) {
                constexpr float SHAKE_BLOCKS = 0.25f, SHAKE_FADE_PER_SECOND = 2.0f;
                const glm::vec3 jolt((float)std::sin(time * 97.0), (float)std::cos(time * 89.0), (float)std::sin(time * 73.0));
                eye += jolt * (shake * SHAKE_BLOCKS);
                audio.setCameraShake(std::max(0.0f, shake - (float)dt * SHAKE_FADE_PER_SECOND));
            }
        }
        if (glm::length(target - eye) > 1e-4f) { // the view the use key's crosshair is in (next frame)
            const VkExtent2D screen = renderer.screenSize();
            useCamera.eye = eye;
            useCamera.forward = glm::normalize(target - eye);
            useCamera.right = glm::normalize(glm::cross(useCamera.forward, glm::vec3(0, 0, 1)));
            useCamera.up = glm::cross(useCamera.right, useCamera.forward);
            useCamera.tanHalfHeight = std::tan(glm::radians(renderSettings.fov) * 0.5f);
            useCamera.tanHalfWidth = useCamera.tanHalfHeight * (screen.height ? (float)screen.width / (float)screen.height : 1.0f);
        }
        Lighting lighting = lightingAt(hour, dayNight, regionLook(columnAt(player.position.x, player.position.y)));
        if (mode == progression::Mode::Pyro && !session.started())
            session.visitLocalRegion(regionHere(), playerLevel);
        opaque.clear();
        water.clear();
        if (!spawnedReady)
            world.collect(opaque, water);
        if (spawnedReady) {
            const auto now = std::chrono::steady_clock::now();
            const double ms = std::min(100.0, std::chrono::duration<double, std::milli>(now - spawnedClock).count());
            spawnedClock = now;
            // The network first: the host's tick (unless this player is a guest elsewhere), what the
            // client's host sent, and its world updates, before the client world's step.
            const uint32_t nowMs = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            if (session.started()) {
                if (HostSide *host = session.hostSide())
                    host->standIn = player.position;
                session.beginFrame(nowMs);
                session.consume();
                SpawnedCreatures &clock = serverMode ? hostWorld : spawned; // (the host's, by packet 8)
                hour = clock.timeOfDay() / 3600000.0f;
                day = clock.day();
            }
            spawned.update(extractedWorld, player.position);
            {
                // The player as the AI sees it (monsters near it notice it, as in the game).
                static glm::vec3 lastAt = player.position;
                const glm::vec3 velocity = ms > 0 ? (player.position - lastAt) / (float)(ms / 1000) : glm::vec3(0);
                lastAt = player.position;
                spawned.setPlayer(extractedWorld, player.position, heading, velocity, playerHp, playerLevel,
                                  player.verticalSpeed == 0 && !player.fly);
                if (haveGear)
                    spawned.setPlayerClass(gear.playerClass());
                // Crossing into another region changes what the gear is worth (the game's rules).
                if (mode != progression::Mode::Pyro && haveGear && !(regionHere() == playerRegion))
                    applyGear();
            }
            spawned.think(extractedWorld, ms, hour);
            if (nativePlayer && !player.fly)
                spawned.control(controlInput, ms);
            spawned.simulate(extractedWorld, ms);
            world.collect(opaque, water); // draw this step's live object state, not the previous frame's
            spawned.smooth((float)ms); // (0x2b68e0: the drawn places of creatures the host moved)
            if (netTrace >= 0 && time >= netTrace && time < netTrace + 1) { // (testing: what is drawn each frame)
                std::string line = "Net trace: " + std::to_string((int)(time * 1000)) + " ms:";
                spawned.forEach([&](const uint8_t *r) {
                    int64_t id;
                    float t;
                    std::memcpy(&id, r + worldgen::creature::ID, 8), std::memcpy(&t, r + 0x6c, 4);
                    if (r[worldgen::creature::ABILITY] && id != spawned.playerId())
                        line += " " + std::to_string(id % 100000) + ":" + std::to_string(r[worldgen::creature::ABILITY]) + "@" + std::to_string((int)t);
                });
                logLine(line);
            }
            if (testCoins >= 0 && time >= testCoins && session.hostSide() && !session.guestElsewhere()) {
                testCoins = -1; // (testing: five coins at the player's feet, laid by the host world)
                const nativesaves::Item coin = nativesaves::makeItem(grounditems::type::COIN, 0, grounditems::COIN_MATERIAL);
                int64_t at[3];
                playerUnits(at);
                hostWorld.layItem(extractedWorld, coin.data(), at, 1);
                for (int k = 1; k < 5; k++) {
                    const int64_t spot[3] = {at[0] + k * 20000, at[1], at[2]};
                    hostWorld.layItem(extractedWorld, coin.data(), spot, 1);
                }
                logLine("Test coins: five laid by the host");
            }
            // The hits this player made (the host applies them), shown as the game shows numbers.
            for (const cwnet::DamageRecord &number : spawned.events().damageNumbers) {
                int64_t attacker, target, at[3];
                float damage;
                std::memcpy(&attacker, number.data(), 8);
                std::memcpy(&target, number.data() + 0x08, 8);
                std::memcpy(&damage, number.data() + 0x10, 4);
                std::memcpy(at, number.data() + 0x20, sizeof at);
                if (attacker == spawned.playerId() && std::lround(damage) != 0)
                    floating.push_back({extractedWorld.toLocal(at) + glm::vec3(0, 0, 2.4f), (int)std::lround(damage), 0,
                                        target == spawned.playerId()});
            }
            // Kill credits naming this player (its experience comes when the death is seen).
            for (const cwnet::KillRecord &k : spawned.events().kills) {
                int64_t credited, dead;
                std::memcpy(&credited, k.data(), 8);
                std::memcpy(&dead, k.data() + 8, 8);
                if (credited == spawned.playerId())
                    creditedKills.insert(dead);
            }
            { // The world's sound requests (hits, abilities, steps, upkeep, effects, interactions).
                int64_t at[3];
                playerUnits(at);
                audio.playRequests(spawned.takeSounds(), at);
            }
            if (const int nights = spawned.takeInnNights()) { // slept at the innkeeper's (the interaction pass)
                day += nights;
                hour = 7.0f;
                logLine("Slept at the inn until 7:00 of day " + std::to_string(day));
            }
            if (deathTest >= 0) { // (testing: a tombstone as if the player had died that far east)
                placeTombstone(deathTest);
                deathTest = -1;
            }
            { // The world update's timers (MapMarks.h): the tombstone's and the map's "NEW" notes.
                float flightMs = 0;
                const int ability = spawned.playerRecord() ? spawned.playerAbility(flightMs) : 0;
                if (saveGame.tombstoneMs > 0 && mapmarks::tombstoneCounts(ability)) {
                    const glm::dvec3 d = glm::dvec3((double)saveGame.deathPosition[0], (double)saveGame.deathPosition[1],
                                                    (double)saveGame.deathPosition[2]) - playerInBlocks();
                    saveGame.tombstoneMs = mapmarks::tombstoneLeft(saveGame.tombstoneMs, (float)ms, glm::dot(d, d));
                }
                mapmarks::tickNotes(saveGame.missionTimers, (float)ms);
            }
            if (const uint8_t *me = spawned.playerRecord()) { // the world's mission update (0x2e1750)
                missions::World w;
                w.player = me;
                w.finished = &saveGame.finishedMissions, w.solved = &saveGame.solvedMissions;
                w.creatures = [&](const std::function<void(const uint8_t *)> &f) { spawned.forEach(f); };
                w.objects = [&](int zx, int zy) {
                    std::vector<worldgen::ZoneObject> out;
                    extractedWorld.gameObjects(zx, zy, out);
                    return out;
                };
                w.active = [&](const worldgen::ZoneObject &o) { // (+0x28: 1 until opened or given its item)
                    if (o.type == 0x16 && o.reward.category)
                        return !usedCells.count({(int32_t)std::floor((double)(o.x >> 16) / 16.0), (int32_t)std::floor((double)(o.y >> 16) / 16.0)});
                    if (!o.takes.category)
                        return true;
                    const int32_t zx = (int32_t)std::floor((double)(o.x >> 16) / 64.0), zy = (int32_t)std::floor((double)(o.y >> 16) / 64.0);
                    std::vector<worldgen::ZoneObject> objects;
                    extractedWorld.gameObjects(zx, zy, objects);
                    for (size_t i = 0; i < objects.size(); i++)
                        if (objects[i].x == o.x && objects[i].y == o.y && objects[i].type == o.type)
                            return !usedObjects.count({zx, zy, (int32_t)i});
                    return true;
                };
                w.missionCreatures = [&](int zx, int zy) { return spawned.missionIds(zx, zy); };
                w.dead = [&](int64_t id) { return spawned.isWorldDead(id); };
                w.hasSpecial = [&](int id) { // (0x5e890: an item 24 of the player's region; 4 always)
                    if (id == 4)
                        return true;
                    const worldgen::Int2 here = regionHere();
                    for (const Carried &c : bag)
                        if (c.item.category == 24 && c.item.id == id && c.item.region[0] == here.x && c.item.region[1] == here.y)
                            return true;
                    return false;
                };
                w.carries = [&](const worldgen::Item &it) { // (0x5e780 with 0x4df60's fields)
                    for (const Carried &c : bag)
                        if (c.item.category == it.category && c.item.id == it.id && c.item.modifier == it.modifier &&
                            c.item.region[0] == it.region.x && c.item.region[1] == it.region.y && (uint32_t)c.item.formula == it.realm)
                            return true;
                    return false;
                };
                missions::Effects fx;
                const size_t finishedBefore = saveGame.finishedMissions.size(), solvedBefore = saveGame.solvedMissions.size();
                if (missions::update(w, fx)) {
                    if (saveGame.finishedMissions.size() != finishedBefore || saveGame.solvedMissions.size() != solvedBefore) {
                        logLine("Missions: " + std::to_string(saveGame.finishedMissions.size()) + " finished, " +
                                std::to_string(saveGame.solvedMissions.size()) + " solved");
                        audio.startSuccess(); // a mission completed: the success track (sound.md §3.3)
                    }
                    spawned.setPlayerToggle(0x8000, fx.witch);
                    spawned.setPlayerToggle(0x800, fx.demon);
                    spawned.setPlayerToggle(0x4000, fx.barriers);
                    // The buffs last a minute, renewed when their strength changes or under a second is left.
                    auto keep = [&](int type, int strength) {
                        float has = 0, left = 0;
                        const bool on = spawned.playerBuff(type, has, left);
                        if (strength <= 0) {
                            if (on)
                                spawned.endPlayerBuff(type);
                        } else if (!on || has != (float)strength || left < 1000.0f)
                            spawned.givePlayerBuff(type, (float)strength, 60000.0f);
                    };
                    keep(0x21, fx.flames);
                    keep(0x22, fx.pumpsLeft);
                }
            }
            { // the game's frame update while flying (ability 0x9c): the wing beat twice a second from the first half second
                static float lastFlightMs = 0;
                float flightMs = 0;
                if (spawned.playerAbility(flightMs) == 0x9c) {
                    if (soundrules::windDue(lastFlightMs, flightMs)) { // the wind at the player (sound.md §5.1)
                        const soundrules::WindSound wind = soundrules::windSound(clientRand);
                        int64_t at[3];
                        playerUnits(at);
                        audio.playAt(soundrules::soundid::WIND, at, wind.volume, wind.pitch);
                    }
                    lastFlightMs = flightMs;
                } else
                    lastFlightMs = 0;
            }
            if (flyTest) { // (testing: the flight, from 5 s in, where the player is each second)
                static double started = -1, told = 0;
                if (started < 0 && time > 5) {
                    started = time;
                    const int32_t x = (int32_t)std::floor(player.position.x) + (int32_t)(extractedWorld.originDotsX() >> 16) + flyTestX;
                    const int32_t y = (int32_t)std::floor(player.position.y) + (int32_t)(extractedWorld.originDotsY() >> 16) + flyTestY;
                    logLine(std::string("Flight test: ") + (spawned.startFlight(x, y) ? "off" : "not started") + " to " + std::to_string(x) + " " +
                            std::to_string(y));
                }
                if (started >= 0 && time - told >= 1) {
                    told = time;
                    float abilityMs = 0;
                    const int a = spawned.playerAbility(abilityMs);
                    logLine("Flight test: at " + std::to_string((int)player.position.x) + " " + std::to_string((int)player.position.y) + " " +
                            std::to_string((int)player.position.z) + ", ability " + std::to_string(a) + " " + std::to_string((int)abilityMs) + " ms");
                }
            }
            static double contactClock = 0;
            if ((contactClock += ms) > 10000) {
                contactClock = 0;
                size_t ground, wet, all;
                spawned.contacts(ground, wet, all);
                size_t moving, pathing;
                spawned.walking(moving, pathing);
                logLine("Creatures: " + std::to_string(ground) + " of " + std::to_string(all) + " on the ground, " +
                        std::to_string(wet) + " in water, " + std::to_string(moving) + " walking (" + std::to_string(pathing) +
                        " along a path), " + std::to_string(spawned.afterPlayer()) + " after the player" + (spawned.roadTrips() ? ", " + std::to_string(spawned.roadTrips()) + " packs on the roads" : std::string()));
                size_t using_ = 0, started = 0;
                spawned.abilities(using_, started);
                logLine("Creatures: " + std::to_string(using_) + " in an ability (" + std::to_string(started) +
                        " started since the last count); " + (nativePlayer ? "the player started " + std::to_string(spawned.playerMoves()) + " moves and landed " + std::to_string(spawned.playerStrikes()) + " hits (drawn in " + std::to_string(spawned.playerParts()) + " parts, " + std::to_string(spawned.heroicShouts()) + " heroic shouts, " + std::to_string(spawned.summonsMade()) + " summoned); " : std::string()) + std::to_string(spawnedHits) + " hits on the player (" +
                        std::to_string((int)std::round(spawnedDamage)) + " damage), " + std::to_string(spawnedBlocked) +
                        " blocked, " + std::to_string(spawned.killed()) + " creatures killed by others; " +
                        std::to_string(spawned.objectUses()) + " zone objects taken up (" + std::to_string(spawned.doorSwings()) +
                        " door swings; " + std::to_string(spawned.interactionsHanded()) + " interactions waiting over the AI's ticks)");
                if (const std::string a = spawned.angriest(extractedWorld, player.position); !a.empty())
                    logLine("Creatures: nearest after the player: " + a);
                double tx, ty;
                if (spawned.firstTrip(tx, ty))
                    logLine("Road travel: the first pack heads for " + std::to_string((int)tx) + ", " + std::to_string((int)ty));
            }
            static bool reported = false;
            if (!reported && spawned.count()) {
                reported = true;
                int race = 0;
                glm::vec3 at;
                if (spawned.nearest(extractedWorld, player.position, race, at))
                    logLine("Creatures: " + std::to_string(spawned.count()) + " near; nearest race " + std::to_string(race) + " at " +
                            std::to_string(at.x) + " " + std::to_string(at.y) + " " + std::to_string(at.z) + ", ground " +
                            std::to_string(groundAt(at.x, at.y)) + ", player at z " + std::to_string(player.position.z));
                for (int h : {0, 2, 3, 4, 5, 6, 7, 8})
                    if (const size_t n = spawned.list(extractedWorld, h).size())
                        logLine("Creatures: " + std::to_string(n) + " of hostility " + std::to_string(h));
                for (auto &[r, p] : spawned.list(extractedWorld, 1))
                    logLine("Creatures: monster race " + std::to_string(r) + " at " + std::to_string((int)p.x) + " " +
                            std::to_string((int)p.y) + " " + std::to_string((int)p.z) + ", ground " +
                            std::to_string((int)groundAt(p.x, p.y)));
            }
            spawned.draw(extractedWorld, world, ms, (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(),
                         player.position, renderSettings.fogDistance, opaque);
            // Deaths: the body bursts into the voxels it was drawn with, and a kill by the
            // player pays XP like the recorded monsters'. What they dropped (0x2a6860), coins
            // included, lies on the ground as the game's ground items.
            std::map<int64_t, int> coinsOf, rewardCoins; // coins laid, by the creature that dropped them
            // A game item (0xa0 bytes) lying on the ground: a creature's drop or a zone's own
            // (a generated item becomes what this player's class finds, the class-adapted item).
            auto groundLoot = [&](const uint8_t *raw, glm::vec3 at, float rotation, float dropMs, int level, bool forPlayer) {
                nativeplayer::Item made;
                std::memcpy(made.data(), raw, made.size());
                made = nativeplayer::transformed(made, haveGear ? gear.playerClass() : 0);
                const uint8_t *n = made.data();
                int32_t id;
                std::memcpy(&id, n + 4, 4);
                Gear::Item it;
                it.category = n[0], it.id = id, it.material = n[0x1c], it.rarity = std::clamp((int)n[0x14], 0, 4);
                std::memcpy(it.region, n + 0xc, 8);
                std::memcpy(&it.formula, n + 0x18, 4);
                std::memcpy(&it.modifier, n + 8, 4);
                it.level = dropLevel(it, level);
                if (auto *m = catalog.model(it.category, it.id, it.material); m && !m->sprites.empty())
                    it.sprite = *m->sprites.begin();
                const Gear::Slot slot = slotForCategory(it.category);
                const bool asGear = haveGear && slot != Gear::SLOTS && foundStats(it, slot);
                Loot l;
                l.at = at;
                l.hasItem = true, l.item = it, l.slot = asGear ? slot : Gear::SLOTS;
                l.toBag = !asGear; // resources, food, pets, specials (and gear the bag keeps)
                l.onGround = true;
                l.rotation = rotation, l.dropMs = dropMs, l.forPlayer = forPlayer;
                if (const std::string sprite = spawned.itemSprite(n); !sprite.empty())
                    l.model = world.loadModel(sprite);
                loot.push_back(l);
                return it;
            };
            // A whole ground item record (0xe0 bytes, drops-and-items.md §0.4): its item, angle,
            // pick-up delay, timer, flags, count and owner.
            auto groundRecord = [&](const uint8_t *g, glm::vec3 at, int level) {
                using namespace grounditems::ground;
                constexpr int64_t LOCAL_PLAYER = 1;
                float rotation, dropMs;
                int32_t timer, count;
                int64_t owner;
                std::memcpy(&rotation, g + ANGLE, 4);
                std::memcpy(&dropMs, g + DELAY, 4);
                std::memcpy(&timer, g + TIMER, 4);
                std::memcpy(&count, g + COUNT, 4);
                std::memcpy(&owner, g + OWNER, 8);
                const Gear::Item it = groundLoot(g, at, rotation, dropMs, level, owner == LOCAL_PLAYER);
                Loot &l = loot.back();
                std::memcpy(l.raw.data(), g + ITEM, l.raw.size());
                l.notice = true;
                l.timerMs = timer, l.flags = g[FLAGS], l.count = count, l.noticeType = g[ITEM];
                return it;
            };
            // What deaths and thanks dropped (0x2a6860, 0x2a7ca0): coins included, they hop in
            // place and lie in their zone's list until picked up (loot-pickup.md §3).
            for (const SpawnedCreatures::Dropped &dr : spawned.takeDrops()) {
                const uint8_t *n = dr.drop.data();
                const Gear::Item it = groundRecord(n, dr.at, dr.level);
                Loot &l = loot.back();
                l.droppedBy = dr.from;
                l.zone = {(int32_t)std::floor((dr.at.x + (float)(extractedWorld.originDotsX() >> 16)) / 64.0f),
                          (int32_t)std::floor((dr.at.y + (float)(extractedWorld.originDotsY() >> 16)) / 64.0f)};
                if (n[0] == grounditems::type::COIN)
                    (dr.reward ? rewardCoins : coinsOf)[dr.from]++;
                else
                    logLine("Dropped " + itemName(it));
            }
            for (const auto &[from, coins] : rewardCoins)
                logLine("Reward: " + std::to_string(coins) + " coins from " + std::to_string(from));
            // The items lying in the zones around the player (the zone's ground items, zone+0x30:
            // herbs, crops, the place's special item, treasure), as the zone keeps them while it is
            // in memory (with world.db's state on top); the ones taken stay taken. A zone that
            // leaves memory takes its lying items with it.
            {
                for (const auto &[zx, zy] : spawned.takeUnloadedZones()) {
                    zoneItemsLoaded.erase({zx, zy});
                    loot.erase(std::remove_if(loot.begin(), loot.end(),
                                              [&](const Loot &l) {
                                                  const bool inZone = (l.zoneItem[0] == zx && l.zoneItem[1] == zy) || (l.zone[0] == zx && l.zone[1] == zy);
                                                  return inZone && !l.taken;
                                              }),
                               loot.end());
                }
                // A zone whose ground items the host sent anew is shown anew.
                for (const auto &[zx, zy] : spawned.takeReplacedGround()) {
                    zoneItemsLoaded.erase({zx, zy});
                    loot.erase(
                        std::remove_if(loot.begin(), loot.end(), [&](const Loot &l) { return l.zoneItem[0] == zx && l.zoneItem[1] == zy; }),
                        loot.end());
                }
                // What this player picked up: the host's pick-up notices (loot-pickup.md §4).
                for (const cwnet::PickupRecord &n : spawned.events().pickups) {
                    using namespace grounditems::ground;
                    constexpr size_t PICKER = 0x00, NOTICE_ITEM = 0x08, NOTICE_COUNT = 0xa8, NOTICE_POSITION = 0xb0, NOTICE_FLAGS = 0xc8;
                    int64_t picker, at[3];
                    int16_t count;
                    std::memcpy(&picker, n.data() + PICKER, 8);
                    if (picker != spawned.playerId())
                        continue;
                    std::memcpy(&count, n.data() + NOTICE_COUNT, 2);
                    std::memcpy(at, n.data() + NOTICE_POSITION, sizeof at);
                    nativesaves::Record g{}; // (the ground item it was)
                    std::memcpy(g.data() + ITEM, n.data() + NOTICE_ITEM, 0xa0);
                    std::memcpy(g.data() + POSITION, at, sizeof at);
                    const int32_t many = count;
                    std::memcpy(g.data() + COUNT, &many, 4);
                    g[FLAGS] = n[NOTICE_FLAGS];
                    groundRecord(g.data(), extractedWorld.toLocal(at), 1);
                    const Loot got = loot.back();
                    loot.pop_back();
                    const int k = receivePickup(got);
                    for (int c = 0; c < k; c++)
                        collect(got);
                }
                const int32_t pzx = (int32_t)std::floor((player.position.x + (float)(extractedWorld.originDotsX() >> 16)) / 64.0f);
                const int32_t pzy = (int32_t)std::floor((player.position.y + (float)(extractedWorld.originDotsY() >> 16)) / 64.0f);
                for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                    for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++) {
                        if (zoneItemsLoaded.count({zx, zy}))
                            continue;
                        const std::vector<nativesaves::Record> *lying = spawnedReady ? spawned.groundItems(extractedWorld, zx, zy) : nullptr;
                        if (spawnedReady && !lying)
                            continue; // (its blocks are not in memory yet)
                        zoneItemsLoaded.insert({zx, zy});
                        const std::vector<worldgen::ZoneObject> objects = worldgen::zoneObjects(zx, zy);
                        for (const worldgen::ZoneObject &o : objects)
                            if (o.type == 22 && o.reward.category) {
                                rewardChests.push_back(o);
                                const int64_t dots[3] = {o.x, o.y, o.z};
                                const glm::vec3 at = extractedWorld.toLocal(dots);
                                logLine("Reward chest at " + std::to_string((int)at.x) + " " + std::to_string((int)at.y) + " " +
                                        std::to_string((int)at.z));
                            }
                        for (const worldgen::ZoneObject &o : objects)
                            if (o.type == 83 || (o.type >= 98 && o.type <= 103)) {
                                const int64_t dots[3] = {o.x, o.y, o.z};
                                workstations.push_back({o.type, extractedWorld.toLocal(dots)});
                            }
                        std::vector<nativesaves::Record> records;
                        if (lying)
                            records = *lying;
                        else // (the recorded creatures stand in: the generator's items, kept by the session)
                            for (const worldgen::GroundItem &g : worldgen::zone(zx, zy)->items)
                                records.push_back(zonestates::groundItemRecord(g, day));
                        for (size_t i = 0; i < records.size(); i++) {
                            const nativesaves::Record &g = records[i];
                            const int here = (int)i;
                            if (!zonestates::holdsItem(g))
                                continue;
                            const std::array<int64_t, 3> dots = zonestates::groundItemPosition(g);
                            groundRecord(g.data(), extractedWorld.toLocal(dots.data()), 1);
                            loot.back().zoneItem = {zx, zy, here};
                        }
                    }
            }
            if (chestToOpen) { // a chest the use key opened (UsePrompt.h, spec 3.7.3)
                const worldgen::ZoneObject chest = *std::exchange(chestToOpen, std::nullopt);
                // Its cell becomes a known place, which keeps it opened.
                const lore::Cell cell = lore::cellOf(chest.x, chest.y);
                if (usedCells.insert(cell).second)
                    saveGame.usedCells.push_back(cell);
                // Its item (a role's own treasure, or the zone's generated reward made into the
                // player's) goes into the bag, or a gold bag into the purse (collect).
                const worldgen::Item &reward = chest.reward;
                const nativeplayer::Item raw = nativeplayer::item(reward.category, reward.id, reward.material, reward.rarity, reward.modifier,
                                                                  reward.region.x, reward.region.y, (int)reward.realm);
                const Gear::Item got = groundLoot(raw.data(), player.position, 0.0f, 0.0f, 1, false);
                loot.back().taken = true;
                // The dialog names it, a special or a fine piece of gear as "the" item.
                const int type = got.category;
                const bool fine = type == 0x17 || type == 0x18 || (((type >= 3 && type <= 9) || type == 0x1a) && got.rarity > 2);
                showDialog({"define_item", std::to_string(type), std::to_string(got.id), std::to_string(got.modifier), std::to_string(got.material),
                            std::to_string(got.rarity), fine ? "you_received_the_item" : "you_received_an_item"},
                           chatter() % 10000);
                // The player holds it up.
                if (nativeai::Creature *me = spawned.localPlayer()) {
                    useprompt::Scene holder;
                    holder.player = me->record;
                    holder.abilityDuration = spawned.playerAbilityDuration();
                    useprompt::holdUp(holder, nativeplayer::item(got.category, got.id, got.material, got.rarity, got.modifier, got.region[0],
                                                                 got.region[1], got.formula));
                }
                logLine("Opened a chest: " + itemName(got));
            }
            const auto deaths = spawned.takeDeaths(extractedWorld);
            std::vector<steampoints::DeathCandidate> deathLights;
            for (const auto &d : deaths)
                if (d.record.size() > 0x1500) deathLights.push_back({d.identity, d.record.data()});
            transientLights.deaths(std::move(deathLights), [&](const auto &position, float radius, float range) {
                return renderer.steamSphereVisible(position, radius, range);
            });
            for (const SpawnedCreatures::Death &d : deaths) {
                events.death(d.id);
                if (std::find(saveGame.killed.begin(), saveGame.killed.end(), d.id) == saveGame.killed.end())
                    saveGame.killed.push_back(d.id); // (the world remembers who died)
                // Credited by its kill credit (packet 7 list 10), which names everyone it was
                // angry at (creature-hit.md §3.6).
                const bool mine = creditedKills.erase(d.id) > 0;
                logLine("Creatures: race " + std::to_string(d.race) + " (level " + std::to_string(d.level) + ") died" +
                        (mine ? " by the player" : ""));
                if (mine) {
                    const int enemy = mode == progression::Mode::Pyro ? creatureLevel(d.record.data()) : d.level;
                    creditKill(d.level, enemy, "", d.at, true);
                    if (mode == progression::Mode::Pyro && coinsOf[d.id] > 0) // (PyroProgression's coins share the level)
                        for (Loot &l : loot)
                            if (l.droppedBy == d.id && l.item.category == grounditems::type::COIN)
                                l.gold = std::max(1, enemy / coinsOf[d.id]);
                }
            }
            for (int64_t id : spawned.takeMarkedDead()) // tamed: the old id stays dead
                if (std::find(saveGame.killed.begin(), saveGame.killed.end(), id) == saveGame.killed.end())
                    saveGame.killed.push_back(id);
            for (const grounditems::Tamed &t : spawned.takeTames()) { // (loot-pickup.md §5.2)
                if (t.hadPet)
                    receive(t.oldPet);
                auto food = std::find_if(bag.begin(), bag.end(), [&](const Carried &c) {
                    return nativeinteract::sameItem(rawOf(c.item).data(), t.food.data());
                });
                if (food != bag.end())
                    consumeCarried((size_t)(food - bag.begin()));
                if (nativeInventory && spawned.playerRecord()) {
                    std::memcpy(nativeInventory->equipment[Gear::Pet].data(), spawned.playerRecord() + 0x8b8, nativesaves::ITEM_SIZE);
                    rebuildInventoryViews();
                }
                logLine("TAMED!");
            }
            if (events.enabled()) {
                OverworldEvents::World w = eventWorld();
                if (!eventsTest.empty()) { // --events-test: the mod's chat command
                    std::string reply;
                    if (!events.command("/events " + (eventsTest == "landmarks" ? eventsTest : "test " + eventsTest), reply))
                        logLine("Events: unknown test '" + eventsTest + "'");
                    else if (!reply.empty())
                        logLine("Events: " + reply);
                    eventsTest.clear();
                }
                if (eventsStatus >= 0 && time >= eventsStatus) {
                    std::string reply;
                    events.command("/events status", reply);
                    logLine(reply);
                    eventsStatus = -1;
                }
                if (eventsUse >= 0 && time >= eventsUse) {
                    std::vector<int64_t> ids;
                    spawned.forEach([&](const uint8_t *r) { int64_t id; std::memcpy(&id, r + worldgen::creature::ID, 8); ids.push_back(id); });
                    for (int pass = 0; pass < 2; pass++) // (a key found on the first pass opens a chest on the second)
                        for (int64_t id : ids)
                            if (OverworldEvents::Said said; events.interact(w, id, said))
                                logLine("Events: used " + std::to_string(id) + ": '" + said.line + "'");
                    eventsUse = -1;
                }
                if (!session.guestElsewhere()) // (the events are this machine's world's)
                    events.update(w, time);
                for (const std::string &line : events.takeLog())
                    logLine(line);
                for (const std::string &reply : events.takeReplies()) {
                    texts.push_back({player.position + glm::vec3(0, 0, 3.1f), reply, {255, 255, 255, 255}});
                    logLine("Events: " + reply);
                }
            }
            for (const SpawnedCreatures::Part &part : spawned.takeBursts()) {
                const glm::mat4 &m = part.place;
                const float voxel = glm::length(glm::vec3(m[0]));
                if (voxel <= 0)
                    continue;
                std::vector<DeathEffects::Piece> pieces;
                for (auto &v : world.voxels(part.model))
                    pieces.push_back({glm::vec3(m * glm::vec4(v.at, 1)), v.color, voxel, glm::mat3(m) / voxel});
                const glm::vec3 size = world.modelSize(part.model);
                deathFx.burst(pieces, glm::vec3(m * glm::vec4(size.x * .5f, size.y * .5f, size.z * .5f, 1)));
            }
            // The end of the frame: what this player caused goes to its host (with its record,
            // at most every 32 ms), and the client world's lists start empty again.
            session.endFrame(nowMs);
            spawned.events() = cwnet::WorldEvents{};
            static double netLogged = 0; // for the log: who this player is and whom it sees
            if (session.started() && !serverMode && time - netLogged > 5) {
                netLogged = time;
                std::string others;
                spawned.forEach([&](const uint8_t *r) {
                    int64_t id;
                    std::memcpy(&id, r + worldgen::creature::ID, 8);
                    if (r[worldgen::creature::HOSTILITY] == 0 && id != spawned.playerId()) {
                        const glm::vec3 at = extractedWorld.toLocal(reinterpret_cast<const int64_t *>(r + worldgen::creature::POSITION));
                        others += " " + std::to_string(id) + " at " + std::to_string((int)at.x) + " " + std::to_string((int)at.y);
                    }
                });
                logLine("Multiplayer: player " + std::to_string(session.playerId()) + (session.guestElsewhere() ? " (a guest)" : "") +
                        ", " + std::to_string(session.guests()) + " on this host, " + std::to_string(spawned.count()) +
                        " creatures seen; other players:" + (others.empty() ? " none" : others) + "; hits taken " +
                        std::to_string(session.mirrorSide() ? session.mirrorSide()->hitsTaken : 0));
            }
        }
        // A sprite drawn with one of the game's part matrices (origin at its voxel corner).
        auto drawPart = [&](int model, const glm::mat4 &m) {
            uint32_t mesh = world.modelMesh(model);
            if (!mesh || m[3][3] == 0)
                return;
            glm::vec3 size = world.modelSize(model);
            glm::vec3 origin = partCorner ? glm::vec3(size.x * .5f, size.y * .5f, 0) : glm::vec3(0, 0, -size.z * .5f);
            opaque.push_back({mesh, m * glm::translate(glm::mat4(1), origin), 0, 0, true});
        };
        for (size_t k = 0; k < actors.count(); k++) {
            if (!actors.visible(k) || glm::length(glm::vec2(actors.position(k) - player.position)) > renderSettings.fogDistance)
                continue;
            actors.pose(k, actorPose);
            auto &models = creatureModels[actors.recordingOf(k)];
            for (size_t i = 0; i < actorPose.size() && i < models.size(); i++)
                if (actorPose[i][3][3] != 0)
                    drawPart(models[i], actorPose[i]);
        }

        // Native death responses precede hit-display flashes in the persistent list.
        for (const auto &number : std::exchange(hitVisuals, {}))
            transientLights.hit(number, [&spawned](int64_t id) { return spawned.record(id); });
        transientLights.advance(frameMs); // births age before their first scene collection

        // Death fragments (one mesh, rebuilt each frame).
        if (fragmentMeshId)
            renderer.release(fragmentMeshId), fragmentMeshId = 0;
        if (!serverMode)
            deathFx.fragmentMesh(fragmentMesh);
        if (!fragmentMesh.empty() && (fragmentMeshId = renderer.upload(fragmentMesh)))
            opaque.push_back({fragmentMeshId, glm::mat4(1), 0, 0, true});
        if (testDrop && spawnedReady && haveGear && time > 2) {
            testDrop = false;
            const Gear::Item &w = gear.equipped(Gear::WeaponRight);
            const nativeplayer::Item raw = nativeplayer::item(w.category, w.id, w.material, w.rarity, w.modifier, w.region[0], w.region[1]);
            Loot l;
            l.at = player.position + glm::vec3(std::sin(player.yaw), -std::cos(player.yaw), 0) * 2.0f; // to the right
            l.hasItem = true, l.item = w, l.toBag = true, l.onGround = true, l.rotation = 30, l.dropMs = 600;
            if (const std::string sprite = spawned.itemSprite(raw.data()); !sprite.empty())
                l.model = world.loadModel(sprite);
            logLine("Test drop: " + itemName(w) + " model " + std::to_string(l.model) + " at " + std::to_string(l.at.x) + " " + std::to_string(l.at.y) + " " + std::to_string(l.at.z));
            loot.push_back(l);
        }
        // Dropped items as the game draws a zone's items: scaled (1/14; coins 0.04), lying
        // or standing by category, arcing five blocks high while they drop and tumbling.
        for (auto &l : loot) {
            const uint32_t mesh = l.onGround && l.model >= 0 && l.timerMs <= 0 ? world.modelMesh(l.model) : 0; // (+0xcc: not drawn)
            if (!mesh)
                continue;
            const glm::vec3 size = world.modelSize(l.model);
            const glm::mat4 m = dropPlacement(l, size, time);
            // (CubeVulkan's meshes stand on their middle: the game's corner is half a size off.)
            opaque.push_back({mesh, glm::translate(m, glm::vec3(size.x * .5f, size.y * .5f, 0)), 0, 0, true});
        }
        if (uint32_t mesh = world.modelMesh(pouchModel))
            for (auto &l : loot) {
                if (l.onGround && l.model >= 0 && world.modelMesh(l.model))
                    continue;
                glm::mat4 m = glm::translate(glm::mat4(1), l.at + glm::vec3(0, 0, .15f + .1f * std::sin((float)l.age * 3)));
                m = glm::rotate(m, (float)l.age, glm::vec3(0, 0, 1));
                opaque.push_back({mesh, glm::scale(m, glm::vec3(1 / 12.f)), 0, 0, true});
            }
        if (!characterModels.empty() && !(nativePlayer && !player.fly)) { // (the native player: drawn with the creatures, by the game's pose)
            if (!ability || !character.abilityPose(ability, abilityTime, pose)) {
                ability = 0;
                character.pose(motion, motionTime, walked, pose);
            }
            // A swapped weapon is held the way the game holds its type (grip learned
            // from creatures carrying one), on the recorded hand.
            if (haveGear) {
                std::string sprite = gear.sprite(Gear::WeaponRight);
                const glm::mat4 *grip = sprite.empty() ? nullptr : catalog.grip(catalog.typeOfSprite(sprite));
                int weaponPart = -1, handPart = -1;
                for (size_t i = 0; i < character.parts().size(); i++) {
                    if (character.parts()[i].matrix == 7)
                        weaponPart = (int)i;
                    if (character.parts()[i].matrix == 10)
                        handPart = (int)i;
                }
                if (grip && weaponPart >= 0 && handPart >= 0 && weaponPart < (int)pose.size() &&
                    handPart < (int)pose.size() && pose[handPart][3][3] != 0 &&
                    gear.equipped(Gear::WeaponRight).sprite != gear.originalSprite(Gear::WeaponRight))
                    pose[weaponPart] = pose[handPart] * *grip;
            }
            glm::mat4 place = glm::rotate(glm::translate(glm::mat4(1), player.position), heading - character.forward(),
                                          glm::vec3(0, 0, 1));
            for (size_t i = 0; i < pose.size() && i < characterModels.size(); i++)
                drawPart(characterModels[i], place * pose[i]);
        }
        const auto featViewport = renderer.screenSize();
        const auto featLabels = featTexts.visit(renderer.speechProjection(), {(float)featViewport.width, (float)featViewport.height},
                                               (int32_t)std::clamp(frameMs, 0.0, 1000.0));
        // Interface/frame cleanup is independent of drawing and does not consume an item.
        mapSelection.afterInterface();
        bool mapCursorFromOverlay = false;
        if (hudReady) {
            VkExtent2D screen = renderer.screenSize();
            if (screen.width != hudSize.width || screen.height != hudSize.height) {
                hudSize = screen;
                hud.layout(renderer, screen);
            }
            // The same view the renderer uses, to place plates and numbers over heads.
            float aspect = screen.height ? (float)screen.width / screen.height : 1.f;
            glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(renderSettings.fov), aspect, .1f,
                                                   renderSettings.fogDistance + 64);
            proj[1][1] *= -1;
            glm::mat4 viewProj = proj * glm::lookAtRH(eye, target, glm::vec3(0, 0, 1));
            auto project = [&](glm::vec3 p, glm::vec2 &out) {
                glm::vec4 c = viewProj * glm::vec4(p, 1);
                if (c.w <= .1f)
                    return false;
                out = glm::vec2((c.x / c.w * .5f + .5f) * screen.width, (c.y / c.w * .5f + .5f) * screen.height);
                return true;
            };
            Hud::State hs;
            hs.feats = &featLabels;
            hs.hp = playerHp;
            hs.maxHp = playerMaxHp;
            hs.mp = playerMp * 100; // The game shows mana out of 100.
            hs.stamina = playerStamina;
            project(player.position - glm::vec3(0, 0, .2f), hs.staminaAt);
            hs.maxMp = 100;
            hs.showXp = mode == progression::Mode::Pyro;
            hs.xp = playerXp;
            hs.xpMax = progression::xpToNext(playerLevel);
            npcspeech::Environment speechEnvironment;
            speechEnvironment.localPlayer = spawned.playerId();
            speechEnvironment.find = [&](int64_t id) -> std::optional<npcspeech::Actor> {
                const auto *record = spawned.record(id);
                return record ? std::optional<npcspeech::Actor>(npcspeech::actor(record)) : std::nullopt;
            };
            speechEnvironment.eachActor = [&](const auto &visit) {
                if (const auto *record = spawned.playerRecord()) visit(npcspeech::actor(record));
                spawned.forEach([&](const uint8_t *record) { visit(npcspeech::actor(record)); });
            };
            speechEnvironment.hasModel = [&](int model) { return spawned.spriteAsset(world, model).has_value(); };
            speechEnvironment.project = [&](npcspeech::Point point) -> std::optional<glm::vec2> {
                if (!renderer.speechProjection()) return {};
                const auto screen = renderer.screenSize();
                return npcspeech::project(point, *renderer.speechProjection(), {(float)screen.width,(float)screen.height}, hud.interfaceScale());
            };
            speechEnvironment.effects = [&](int64_t, int64_t recipient, const TalkPlay::Page &page, bool) {
                if (recipient != spawned.playerId()) return;
                for (const auto &cell : page.loreCells) creditLore(cell, true);
                for (const auto &mark : page.marks) markPlace(mark);
            };
            speechEnvironment.finished = [&](int64_t speaker) {
                auto request = nativeinteract::fresh(4);
                std::memcpy(request.data() + nativeinteract::TARGET, &speaker, sizeof speaker);
                spawned.queueInteraction(spawned.playerId(), request);
            };
            speechEnvironment.sound = [&](int sound, const npcspeech::Point &at) { audio.playAt(sound, at.data(), 1, 1); };
            npcSpeech.update(frameMs, speechEnvironment);
            if (const auto page = generalDialogue.activation()) {
                for (const auto &cell : page->loreCells) creditLore(cell, false);
                for (const auto &mark : page->marks) markPlace(mark);
            }
            if (const auto sound = generalDialogue.update(frameMs,
                    gameInput.held(gameoptions::ATTACK), gameInput.held(gameoptions::USE)))
                audio.playInterface(*sound);
            if (!(mapOpen && input.lastUsedPad)) {
                const auto stick = gameInput.gamepad().movement;
                const bool keyboardMove = gameInput.held(gameoptions::UP) || gameInput.held(gameoptions::DOWN) ||
                                          gameInput.held(gameoptions::LEFT) || gameInput.held(gameoptions::RIGHT);
                if ((!chat.typing() && keyboardMove) || stick.x * stick.x + stick.y * stick.y > .1f * .1f)
                    answers.dismissForMovement();
            }
            hs.gold = playerGold;
            { // The info texts and the location banner (Interface's update; the game's frame update's location texts).
                auto floorDiv = [](int64_t a, int64_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); };
                const int64_t dotsX = (int64_t)std::llround((double)player.position.x * 65536) + extractedWorld.originDotsX();
                const int64_t dotsY = (int64_t)std::llround((double)player.position.y * 65536) + extractedWorld.originDotsY();
                const int32_t bx = (int32_t)floorDiv(dotsX, 65536), by = (int32_t)floorDiv(dotsY, 65536);
                auto coordinate = [](int value) { // the coordinate format: twentieths, thousands split by points
                    int rest = std::abs(value) / 20;
                    if (!rest)
                        return std::string("0");
                    std::string text;
                    bool first = true;
                    do {
                        if (!first)
                            text = "." + text;
                        first = false;
                        char part[8];
                        std::snprintf(part, sizeof part, rest / 1000 ? "%03d" : "%d", rest % 1000);
                        text = part + text;
                        rest /= 1000;
                    } while (rest);
                    return value < 0 ? "-" + text : text;
                };
                const worldgen::ColumnField field = worldgen::columnField(bx, by);
                char clock[64];
                std::snprintf(clock, sizeof clock, "  TIME %d:%02d", (int)hour, (int)(hour * 60.0f) % 60);
                // (the map's cursor while the map is open and the pointer is on it)
                const int32_t shownX = mapOpen && mapCursorKnown ? (int32_t)mapCursor.x : bx, shownY = mapOpen && mapCursorKnown ? (int32_t)mapCursor.y : by;
                hs.infoSmall = "COORDS " + coordinate(shownX - 512) + " | " + coordinate(shownY - 512) + clock + "  TEMP " +
                               std::to_string(-20 - (int)(field.colours.warmth * -60.0f)) + " \xb0" + "C  HUM " +
                               std::to_string((int)(field.colours.dryness * 100.0f)) + "%  ZOOM 1:32";
                // The land: its biome's name with the region's generated one (the cell hash of the region).
                std::string text = landNameOf(worldgen::regionAt(bx, by));
                hs.infoLarge = text;
                landName = text;
                // The place the player is well inside (its influence at least half): the place naming.
                const int32_t cx = (int32_t)floorDiv(bx, 1024), cy = (int32_t)floorDiv(by, 1024);
                const bool big = worldgen::bigCell(cx, cy);
                int kind = cx == 0 && cy == 0 ? 1 : worldgen::placeKind(cx, cy, big);
                const worldgen::Place here = worldgen::placeAt(bx, by);
                if (here.kind != kind || here.weight < 0.5f)
                    kind = 0;
                hs.infoMedium.clear();
                if (kind > 0) {
                    int32_t px = cx, py = cy;
                    if (big)
                        px = (int32_t)floorDiv(px, 2) * 2, py = (int32_t)floorDiv(py, 2) * 2;
                    // the world's update: a place the player is well inside is discovered.
                    if (discoveredPlaces.insert({px, py}).second)
                        saveGame.discoveredPlaces.push_back({px, py});
                    if (const std::string name = placeNameOf(px, py, kind); !name.empty())
                        text = name, hs.infoMedium = name;
                }
                // And the land the player stands in.
                if (const worldgen::Int2 area = worldgen::regionAt(bx, by); discoveredAreas.insert({area.x, area.y}).second)
                    saveGame.discoveredAreas.push_back({area.x, area.y});
                // A changed name plays the banner ("location", 3.2 s), not while it plays. The interface
                // animates by the game's frame step, at most a second (the game's frame update).
                static std::string lastLocation;
                static float bannerMs = -1;
                if (bannerMs >= 0)
                    bannerMs += (float)std::min(frameMs, 1000.0);
                if (text.empty())
                    lastLocation.clear();
                else if (text != lastLocation && (bannerMs < 0 || bannerMs > 3200.0f)) {
                    lastLocation = text;
                    bannerMs = 0;
                }
                hs.locationName = lastLocation;
                hs.locationMs = bannerMs;
            }
            { // The quick buttons, cast bar and crosshair (the game's frame update, Interface's update).
                auto skillName = [&](int ability) {
                    const char *key = nativecontrol::skillKey(ability);
                    Speech::Context context;
                    context.tags = {"default"};
                    return key && speech.has(key) ? speech.say(key, context, chatter) : std::string();
                };
                hs.quickItem.key = "Q", hs.quickUse.key = "E", hs.quickSkill.key = "R";
                // An item's picture for a button: its sprite rendered once into the HUD's icons.
                auto showIcon = [&](const std::string &sprite) {
                    loadInventorySprite(sprite);
                    return !sprite.empty();
                };
                for (const Carried &c : bag) // compatibility sessions have no canonical selector storage
                    if (!nativeInventory && c.item.category == 1 && c.item.id != 9 && c.item.id != 13) {
                        // (the game's frame update: the item's dictionary name, the item naming keys's consumables)
                        static const char *const foods[15] = {
                            "Cookie",         "LifePotion",    "CactusPotion",      "EssenceOfWisdom", "GinsengSoup",
                            nullptr,          nullptr,         "SnowBerryMash",     "MushroomSpit",    "Bomb",
                            "PineappleSlice", "PumpkinMuffin", "GreaterLifePotion", "PortalStone",     "Apple"};
                        const char *key = c.item.id >= 0 && c.item.id < 15 ? foods[c.item.id] : nullptr;
                        const std::string named = key ? speech.name(key) : std::string();
                        hs.quickItem.info = named.empty() ? bagName(c.item, itemName(c.item)) : named;
                        hs.quickItem.count = std::to_string(c.count);
                        // Its picture: the sprite the game draws the item with (the item sprite lookup).
                        std::string sprite = c.item.sprite;
                        if (sprite.empty() && spawnedReady)
                            sprite = spawned.itemSprite(nativeplayer::item(c.item.category, c.item.id, c.item.material, c.item.rarity,
                                                                           c.item.modifier, c.item.region[0], c.item.region[1]).data());
                        showIcon(sprite);
                        hs.quickItem.icon = sprite;
                        break;
                    }
                if (nativeInventory) {
                    const auto choices = itemselector::candidates(nativeInventory->inventory);
                    // HUD/name selection precedes this frame's left/right repeat changes.
                    const size_t selected = itemselector::wrappedIndex(itemSelector.index, choices.size());
                    const auto analog = gamepadStick(input.gamepad.leftX, input.gamepad.leftY);
                    itemselector::Input selectorInput;
                    selectorInput.allowToggle = !chat.typing();
                    selectorInput.toggle = itemInputHeld(gameoptions::RING_MENU);
                    selectorInput.forward = itemInputHeld(gameoptions::UP);
                    selectorInput.back = itemInputHeld(gameoptions::DOWN);
                    selectorInput.left = itemInputHeld(gameoptions::LEFT);
                    selectorInput.right = itemInputHeld(gameoptions::RIGHT);
                    selectorInput.stickX = analog.x;
                    selectorInput.stickY = analog.y;
                    selectorInput.fullMap = mapOpen;
                    const auto update = itemSelector.frame(selectorInput, choices.size(), frameMs, hud.interfaceScale());
                    for (int i = 0; i < update.selectionSounds; ++i)
                        audio.playInterface(0x58);
                    hs.ring.shown = itemSelector.shown;
                    hs.ring.angle = itemSelector.angle;
                    hs.ring.radius = itemSelector.radius;
                    hs.ring.worldMilliseconds = spawned.elapsedWorldTime();
                    hs.ring.farPlane = (float)(gameOptions.zonesDrawn() << 6);
                    hs.ring.left = selectorInput.left || analog.x < -.2f;
                    hs.ring.right = selectorInput.right || analog.x > .2f;
                    if (choices.empty())
                        hs.ring.name = "You don't have any usable items.";
                    else if (const auto *stack = nativeInventory->inventory.slot(choices[selected])) {
                        int32_t subtype;
                        std::memcpy(&subtype, stack->item.data() + nativesaves::item::SUBTYPE, sizeof subtype);
                        hs.quickItem.info = speech.name(itemnames::itemKey(1, subtype));
                        hs.quickItem.count = std::to_string(stack->count);
                        hs.quickItem.original = stack->item;
                        hs.quickItem.icon = spawned.itemSprite(stack->item.data());
                        showIcon(hs.quickItem.icon);
                        hs.ring.name = itemnames::name(speech, itemNameParts, stack->item.data());
                        hs.ring.count = std::to_string(stack->count);
                        hs.ring.colour = itemselector::candidateColour(stack->item);
                    }
                    if (hs.ring.shown)
                        for (const auto &choice : choices) {
                            const auto *stack = nativeInventory->inventory.slot(choice);
                            if (!stack)
                                continue;
                            const auto sprite = spawned.itemSprite(stack->item.data());
                            showIcon(sprite);
                            hs.ring.items.push_back({sprite, &stack->item});
                        }
                }
                if (nativePlayer && spawnedReady) {
                    float cooldown = 0;
                    if (const int skill = spawned.specialSkill(cooldown))
                        hs.quickSkill.info = cooldown != 0.0f ? std::to_string((int)(cooldown + 999.0f) / 1000) + " s" : skillName(skill);
                    const SpawnedCreatures::CastBar cast = spawned.castBar();
                    hs.cast = cast.shown;
                    hs.castFill = cast.fraction;
                    hs.castName = cast.ability ? skillName(cast.ability) : cast.label;
                    // The use key's prompt (UsePrompt.h), chosen with this frame's targets.
                    hs.quickUse.info = usePrompt.text;
                    hs.quickUse.infoColor = usePrompt.colour;
                    if (showIcon(useIcon))
                        hs.quickUse.icon = useIcon;
                    // The creatures' life bars (the creature labels), the farthest first.
                    static const std::map<int, const char *> raceKeys = [] {
#include "RaceKeys.inc"
                        return std::map<int, const char *>(std::begin(RACE_KEYS), std::end(RACE_KEYS));
                    }();
                    auto folded = [](int v) {
                        return v >= 1000000 ? std::to_string(v / 1000000) + "." + std::to_string(v % 1000000 / 100000) + "M"
                               : v >= 10000 ? std::to_string(v / 1000) + "." + std::to_string(v % 1000 / 100) + "K"
                                            : std::to_string(v);
                    };
                    std::vector<SpawnedCreatures::Label> labelled = spawned.labels(extractedWorld, player.position, 200.0f);
                    std::sort(labelled.begin(), labelled.end(), [](auto &a, auto &b) { return a.distance2 > b.distance2; });
                    static double labelLog = 0;
                    if (time > labelLog && !labelled.empty()) { // (testing: what the labels read, every 5 s)
                        labelLog = time + 5;
                        const SpawnedCreatures::Label &l = labelled.back();
                        logLine("Labels: " + std::to_string(labelled.size()) + " near; nearest race " + std::to_string(l.race) + " hostility " +
                                std::to_string(l.hostility) + " level " + std::to_string(l.level) + " hp " + std::to_string((int)l.hp) + "/" +
                                std::to_string((int)l.maxHp) + " height " + std::to_string(l.height) + " name '" + l.name + "' crosshair " +
                                std::to_string(crosshairCreature == l.id));
                    }
                    for (const SpawnedCreatures::Label &l : inventoryOpen ? std::vector<SpawnedCreatures::Label>() : labelled) {
                        if (l.race == 285) // water bubbles
                            continue;
                        const bool big = l.id == crosshairCreature;
                        // A villager's quest (OverworldEvents' nameplate sprite): an offer's orange "!", a
                        // reward's orange "?", a quest under way's grey "?", above its name.
                        if (events.enabled() && l.hostility == 3 && l.distance2 < 1600.0f)
                            if (const int quest = [&] { auto w = eventWorld(); return events.questMarker(w, l.id); }()) {
                                Hud::State::MapMarker q;
                                if (project(l.at + glm::vec3(0, 0, l.height * 0.5f - 0.25f), q.at)) {
                                    q.at.y -= 54.0f;
                                    q.questSprite = quest == 1 ? "quest-offer" : quest == 2 ? "quest-event" : "quest-destination";
                                    q.scale = 0.5f;
                                    hs.mapMarkers.push_back(q);
                                }
                            }
                        if (!big && (!(l.hp < l.maxHp) || !(l.distance2 < 1600.0f) || l.hostility == 8))
                            continue;
                        const glm::vec3 top = l.at + glm::vec3(0, 0, l.height * 0.5f - 0.25f);
                        bool seen = true; // the line of sight check from the camera: nothing solid between
                        const float length = glm::length(top - eye);
                        for (float s = 0.5f; s < length - 0.5f && seen; s += 0.5f) {
                            const glm::vec3 q = eye + (top - eye) * (s / length);
                            seen = !extractedWorld.solidAt(q.x, q.y, q.z);
                        }
                        Hud::State::LifeBar bar;
                        if (!seen || !project(top, bar.at))
                            continue;
                        bar.big = big;
                        bar.kind = l.hostility == 1 ? (l.flags & 0x8000 ? 3 : 0) : l.hostility == 7 ? 2 : 1;
                        bar.onlyInfo = l.hostility == 8;
                        // Its own name, or the game's generated one for tinted creatures and townsfolk.
                        static GameNames gameNames = [&] {
                            GameNames g;
                            if (!g.load(assets.gameDir() / "Extract" / "names.txt"))
                                logLine("Names: Extract/names.txt missing (run the extractor in a world)");
                            return g;
                        }();
                        bar.name = !l.name.empty() ? l.name
                                   : (l.flags & 0x200) || l.hostility == 3 ? gameNames.name((int)l.id, l.race)
                                                                           : std::string();
                        static const std::map<int, const char *> roles = {
                            {128, "[Item Vendor]"}, {129, "[Weapon Vendor]"}, {130, " [Armor Vendor]"}, {131, "[Analyzer]"},
                            {132, "[Innkeeper]"}, {136, "[Archaeologist]"}, {137, "[Alchemist]"}, {133, "[Smith]"},
                            {134, "[Carpenter]"}, {135, "[Tailor]"}, {154, "[Flight Master]"}, {155, "[Guild Receptionist]"},
                            {156, "[Gem Trader]"}, {157, "[Shop Supplier]"}, {159, "[Master Sailor]"}, {160, "[Master Glider]"}};
                        if (auto it = roles.find(l.cls); it != roles.end())
                            bar.faction += it->second;
                        static const char *const masters[5] = {nullptr, "[Warrior Master]", "[Ranger Master]", "[Mage Master]", "[Rogue Master]"};
                        if ((l.flags & 0x40) && l.cls >= 1 && l.cls <= 4)
                            bar.faction += masters[l.cls];
                        // (a faction's name, "[...]" first, needs the faction naming: not ported yet)
                        // The level: a player's; under PyroProgression every creature's (its display hook, 0xb1966).
                        if (mode == progression::Mode::Pyro) {
                            if (const uint8_t *r = spawned.record(l.id))
                                bar.info = pyrocreatures::levelText(creatureLevel(r));
                        } else if (l.hostility == 0)
                            bar.info = "LV " + std::to_string(l.level) + " ";
                        if (l.flags & 0x4000000)
                            bar.info += "Possessed ";
                        if (auto it = raceKeys.find(l.race); it != raceKeys.end())
                            bar.info += speech.name(it->second);
                        if (l.hostility == 0)
                            bar.infoColor = glm::vec4(0, 0.8f, 1, 1);
                        else if (l.hostility == 1 || l.hostility == 7) {
                            static const glm::vec4 rarity[6] = {{0.7f, 0.7f, 0.7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1},
                                                                {0.3f, 0.45f, 1, 1}, {0.55f, 0.2f, 1, 1}, {1, 1, 0, 1}};
                            bar.infoColor = rarity[std::clamp(l.level, -1, 4) + 1];
                            if (showRarity) {
                                bar.info += " ";
                                for (int k = 0; k <= l.level; k++)
                                    bar.info += (char)0xb1;
                            }
                        }
                        bar.text = folded((int)l.hp) + "/" + folded((int)l.maxHp);
                        bar.fraction = l.hp / l.maxHp;
                        hs.lifeBars.push_back(bar);
                    }
                }
            }

            if (talkTo >= 0 && time > 1.5 && spawnedReady) { // --talk with the game's creatures: the nearest person
                int64_t id = 0;
                int race = 0;
                glm::vec3 at;
                if (spawned.nearestPerson(extractedWorld, player.position, 400.0f, id, race, at)) {
                    player.position = at + glm::vec3(2, 0, 0);
                    startTalk(Speaker{true, SIZE_MAX, id, race});
                    logLine("Talk: person " + std::to_string(id) + " (race " + std::to_string(race) + ")");
                    if (talk)
                        for (auto &line : talk->lines)
                            logLine("Talk: " + line);
                } else
                    logLine("Talk: no person within 400 blocks");
                talkTo = -1;
            }
            if (talkTo >= 0 && time > 1.5) { // --talk: walk up to that townsperson and talk
                std::vector<std::pair<float, size_t>> people;
                for (size_t k = 0; k < actors.count(); k++)
                    if (actors.townsperson(k))
                        people.push_back({glm::length(actors.position(k) - respawn), k}); // from home: the same order every run
                std::sort(people.begin(), people.end());
                if (talkTo < (int)people.size()) {
                    size_t k = people[talkTo].second;
                    talkedTo = k;
                    player.position = actors.position(k) + glm::vec3(2, 0, 0);
                    startTalk(Speaker{false, k});
                    for (auto &line : talk->lines)
                        logLine("Talk: " + line);
                }
                talkTo = -1;
            }
            if (talkAgain > 0 && time > talkAgain && lastTalkedSpeaker) { // --talk-again
                player.position = speakerAt(*lastTalkedSpeaker) + glm::vec3(2,0,0);
                startTalk(*lastTalkedSpeaker);
                if (talk) for (const auto &line : talk->lines) logLine("Talk: " + line);
                talkAgain = -1;
            }
            if (talk) { // the line being said, a few seconds each (longer lines longer)
                talk->shown += dt;
                const std::string &line = talk->lines[talk->line];
                if (talk->shown > 2.5 + line.size() * .045) {
                    talk->shown = 0;
                    if (++talk->line >= talk->lines.size())
                        talk.reset();
                    else
                        pageShown();
                }
                if (talk && glm::length(talkerAt(*talk) - player.position) > 10)
                    talk.reset(); // walked away
                Hud::State::Bubble bubble;
                if (talk && project(talkerAt(*talk) + glm::vec3(0, 0, 2.7f), bubble.at)) {
                    bubble.text = talk->lines[talk->line];
                    hs.bubbles.push_back(bubble);
                }
            }
            answers.updateVisibility(talk.has_value() || npcSpeech.addressedTo(spawned.playerId()));
            if (nativeInventory) {
                const bool ordinaryUi = inventoryOpen || craftingOpen || mapOpen || vendorTrade.open || identify.open || upgrade.open ||
                                        chat.typing() || answers.visible() || (menusReady && menus.ordinaryUi());
                if (const int sound = interfaceSoundEdges.sample(ordinaryUi, itemSelector.shown))
                    audio.playInterface(sound);
            }
            if (nativePlayer) {
                const bool ordinaryUi = inventoryOpen || mapOpen || menusOpen || chat.typing() || answers.visible();
                const bool leaning = (spawned.playerToggles() & 0x400) != 0;
                hs.crosshair = !generalDialogue.visible() && !ordinaryUi && !leaning && ability != 156;
                hs.zoomCrosshair = !generalDialogue.visible() && !ordinaryUi && leaning;
            }
            if (!npcSpeech.emotes().empty()) speechEnvironment.eachActor([&](const npcspeech::Actor &actor) {
                const auto active = npcSpeech.emotes().find(actor.id);
                if (active == npcSpeech.emotes().end() || !(actor.health > 0)) return;
                const auto pose = npcspeech::emotePose(active->second);
                const auto asset = spawned.spriteAsset(world, pose.model);
                if (!asset) return;
                const npcspeech::Point offset{-extractedWorld.originDotsX(), -extractedWorld.originDotsY(), 0};
                const auto game = npcspeech::emoteTransform(actor, pose, asset->header, offset);
                DrawItem draw;
                draw.mesh = world.modelMesh(asset->model);
                draw.model = glm::translate(glm::mat4(1), glm::vec3(0,0,(float)ExtractedWorld::Z_OFFSET)) * game *
                    glm::translate(glm::mat4(1), glm::vec3(asset->header.x*.5f,asset->header.y*.5f,0));
                draw.flags = GLOW;
                draw.kind = DrawKind::Emote;
                draw.nativeActor = npcspeech::actorSphere(actor);
                opaque.push_back(std::move(draw));
            });
            for (auto &q : quests) { // the tracker: what is left to do, in the game's words
                if (q.state == 2)
                    continue;
                if (hs.quest.empty())
                    hs.quest.push_back("QUESTS");
                Speech::Context context;
                std::string zoneKind = q.zone.substr(0, q.zone.find(':')), zone = q.zone.substr(q.zone.find(':') + 1);
                if (speech.nameTags(zoneKind).find("the") != std::string::npos)
                    context.tags.insert("zone_the");
                context.value = [&](const std::string &fn, const std::string &args) -> std::string {
                    if (fn == "mob") return speech.name(q.target, args.find("plural") != std::string::npos);
                    if (fn == "zone") return zone;
                    return "";
                };
                std::mt19937 fixed((uint32_t)q.giver);
                hs.quest.push_back(q.state == 0 ? speech.say("objective_defeat_enemies_in_zone", context, fixed) + " " +
                                                      std::to_string(q.done) + "/" + std::to_string(q.need)
                                                : speech.say("solved_defeat_enemies_in_zone", context, fixed) + " Return for your reward.");
            }
            for (auto it = texts.begin(); it != texts.end();) {
                it->age += dt;
                double life = it->big ? 3 : 1.6;
                if (it->age > life) {
                    it = texts.erase(it);
                    continue;
                }
                Hud::State::Message m;
                m.text = it->text;
                m.color = it->color;
                m.big = it->big;
                m.fade = (float)std::min(1.0, (life - it->age) / .5);
                if (project(it->at + glm::vec3(0, 0, (float)it->age * .9f), m.at))
                    hs.messages.push_back(m);
                ++it;
            }
            for (size_t k = 0; k < actors.count() && k < trails.size(); k++) {
                float health = actors.health(k) / std::max(actors.maxHealth(k), 1.f);
                trails[k] = std::max(health, trails[k] - .6f * (float)dt);
                if (!actors.engaged(k) || glm::length(actors.position(k) - player.position) > 30)
                    continue;
                Hud::Plate plate;
                if (!project(actors.position(k) + glm::vec3(0, 0, 2.2f), plate.at))
                    continue;
                size_t r = actors.recordingOf(k);
                plate.name = r < creatureNames.size() ? creatureNames[r] : "";
                plate.level = mode == progression::Mode::Pyro ? enemyLevels[k] : showLevelsSetting && r < creatures.all().size() ? creatures.all()[r].level : 0;
                plate.health = health;
                plate.trail = trails[k];
                hs.plates.push_back(plate);
            }
            for (auto it = floating.begin(); it != floating.end();) {
                it->age += dt;
                if (it->age > 1.2) {
                    it = floating.erase(it);
                    continue;
                }
                Hud::Number n;
                n.value = it->value;
                n.taken = it->taken;
                n.fade = (float)std::min(1.0, (1.2 - it->age) / .4);
                if (project(it->at + glm::vec3(0, 0, (float)it->age * 1.2f), n.at))
                    hs.numbers.push_back(n);
                ++it;
            }
            if (inventoryOpen)
                hs.inventory = &inventoryView;
            if (identify.open)
                hs.identification = &identificationView;
            { // the identify window, the upgrade widget and the chat line (Hud::State::windows)
                if (!lootWindowItems.empty())
                    hs.windows = &lootWindowItems;
            }
            hs.mapMarkers.insert(hs.mapMarkers.begin(), mapMarkerViews.begin(), mapMarkerViews.end());
            hs.guiClockMs = (int32_t)(int64_t)(time * 1000.0);
            hs.mapClockMs = hs.guiClockMs;
            hs.mapWorldElapsedMs = spawned.elapsedWorldTime();
            double mapPointerX, mapPointerY;
            pointerPosition(&mapPointerX, &mapPointerY);
            if (pointerTestX >= 0)
                mapPointerX = pointerTestX, mapPointerY = pointerTestY;
            hs.pointerAt = {(float)mapPointerX, (float)mapPointerY};
            // The actual overlay draw owns these cached targets. A hidden interface keeps
            // the preceding draw's result; release never repicks from new coordinates.
            if (!menusReady || menus.interfaceShown()) {
                mapmarks::Hover hovered;
                if (mapOpen && !mapSelection.dragged)
                    hovered = mapmarks::hover(
                        hs.mapMarkers, hs.pointerAt, hud.interfaceScale(),
                        [&](const mapmarks::Marker &marker) { return hud.mapLabelWidth(marker); }, mapSelection.shrinesOnly);
                mapSelection.overlayDrawn(hovered);
                flightSelected = false;
                if (hovered.flight && !hovered.flightToPlayer) {
                    flightSelected = true;
                    selectedFlightPoint = {hovered.flight->x, hovered.flight->y};
                }
                hoveredLoreCell = hovered.cell;
                hoveredArtifact = hovered.item;
                if (hovered.best >= 0) {
                    mapCursor = glm::dvec3(hovered.cursor);
                    mapCursorKnown = mapCursorFromOverlay = true;
                }
                mapmarks::enlargeTargets(hs.mapMarkers, hovered);
            }
            hs.mapShrineMode = mapSelection.shrinesOnly;
            hs.mapShrineTarget = mapSelection.target;
            // The lore panel of the lore building under the pointer (the lore spec's 4.5): its realm,
            // how much of the realm's lore is read, and the building's tale.
            if (mapOpen && hoveredLoreCell && lore::isLoreBuilding(*hoveredLoreCell)) {
                using Line = Hud::Inventory::InfoLine;
                const lore::Cell cell = *hoveredLoreCell;
                const uint32_t realm = lore::realmOf(cell);
                const auto read = loreBook.find(realm);
                const int percent = lore::percent(read == loreBook.end() ? 0 : read->second.size());
                if (const std::string name = realmName(realm); !name.empty())
                    hs.mapPreview.push_back(Line{name, 12, 3, glm::vec4(1.0f, 0.8f, 0.5f, 1), 30, true});
                const glm::vec4 grey(0.75f, 0.75f, 0.75f, 1), green(0.6f, 1.0f, 0.25f, 1);
                hs.mapPreview.push_back(Line{"Lore " + std::to_string(percent) + "%", 12, 3, percent < 100 ? grey : green, 30, true});
                TalkScript script;
                script.tokens = lore::tale(cell);
                std::mt19937 seeded(worldgen::random((uint32_t)cell[0] + 57u * (uint32_t)cell[1]));
                for (const std::string &line : playScript(script, speech, seeded, &talkNames).lines)
                    hs.mapPreview.push_back(Line{line, 12, 3, glm::vec4(1), 10, true});
            } else if (mapOpen && hoveredArtifact.category != 0 && talkNames.item) {
                // A revealed artifact's place: the artifact hidden there (spec: open question: the
                // game's drawing of it was not traced; CubeVulkan names it).
                using Line = Hud::Inventory::InfoLine;
                const int item[5] = {hoveredArtifact.category, hoveredArtifact.id, (int)hoveredArtifact.modifier,
                                     hoveredArtifact.material, hoveredArtifact.rarity};
                std::string name, key;
                talkNames.item(item, name, key);
                if (name.empty() && !key.empty())
                    name = speech.name(key);
                if (!name.empty())
                    hs.mapPreview.push_back(Line{name, 12, 3, glm::vec4(1.0f, 0.8f, 0.5f, 1), 30, true});
            }
            // Interface's its pointer node: the game's pointer in its menus; on the open map, the flat
            // distance from the player to the map's cursor (the distance text, a metre
            // being two blocks).
            hs.pointer = hud.hasPointer() && (inventoryOpen || mapOpen || answers.visible());
            // the flight point preview for the selected flight point: its name, the
            // distance, and the flight time (once known) or what it costs.
            if (mapOpen && flightSelected) {
                using Line = Hud::Inventory::InfoLine;
                const glm::dvec2 here(player.position.x + (double)(extractedWorld.originDotsX() >> 16),
                                      player.position.y + (double)(extractedWorld.originDotsY() >> 16));
                const int metres = (int)(glm::length(glm::dvec2(selectedFlightPoint[0], selectedFlightPoint[1]) - here) * 0.5);
                hs.mapPreview.push_back(Line{"Flight point", 12, 3, glm::vec4(0.5f, 1, 1, 1), 30, true});
                hs.mapPreview.push_back(Line{"Distance: " + flatDistance(metres), 12, 3, glm::vec4(1), 30, true});
                if (boughtFlights.count(selectedFlightPoint)) {
                    int time = nativeability::flightTime((int32_t)here.x, (int32_t)here.y, selectedFlightPoint[0], selectedFlightPoint[1]);
                    float flightMs = 0;
                    if (const uint8_t *me = spawned.playerRecord(); me && spawned.playerAbility(flightMs) == 0x9c) {
                        int32_t to[2];
                        int64_t from[3];
                        std::memcpy(to, me + 0x1b0, 8), std::memcpy(from, me + 0x1b8, 24);
                        if (to[0] == selectedFlightPoint[0] && to[1] == selectedFlightPoint[1]) // (already on the way: from the take-off)
                            time = (int)((float)nativeability::flightTime((int32_t)(from[0] / 0x10000), (int32_t)(from[1] / 0x10000), to[0], to[1]) - flightMs);
                    }
                    hs.mapPreview.push_back(Line{"Flight time: " + std::to_string(time / 1000) + " seconds", 12, 3, glm::vec4(1), 30, true});
                } else if (const int cost = flightPrice(selectedFlightPoint); cost <= playerGold)
                    hs.mapPreview.push_back(Line{"Purchase for: " + std::to_string(cost) + " gold", 12, 3, glm::vec4(1, 0.9f, 0, 1), 0, true});
                else
                    hs.mapPreview.push_back(Line{"Cost: " + std::to_string(cost) + " gold", 12, 3, glm::vec4(1, 0, 0, 1), 0, true});
            }
            if (hs.pointer) {
                hs.pointerText.clear();
                if (mapOpen && mapCursorKnown) {
                    const glm::dvec2 here(player.position.x + (double)(extractedWorld.originDotsX() >> 16),
                                          player.position.y + (double)(extractedWorld.originDotsY() >> 16));
                    hs.pointerText = flatDistance((int)(glm::length(here - glm::dvec2(mapCursor)) * 0.5));
                }
            }
            std::vector<glm::vec2> answerPoints;
            input.gamepadNavigationPoints.clear();
            hud.draw(hs, hudVertices, [&](std::vector<OverlayVertex> &out) {
                if (menusReady && !menus.interfaceShown()) return;
                const auto pixels = renderer.screenSize();
                hud.drawChat(chat, (int32_t)(int64_t)(time * 1000), out);
                double mouseX, mouseY;
                pointerPosition(&mouseX, &mouseY);
                hud.drawNpcSpeech(npcSpeech.bubbles(), (int32_t)(int64_t)(time * 1000), out,
                    [&](int64_t speaker, const dialoguepanel::Layout &layout, glm::vec2) {
                        if (!captureSpeech.empty() && !speechCaptured && layout.revealed >= 8) {
                            renderer.capture(captureSpeech);
                            speechCaptured = true;
                            logLine("NPC speech capture: " + captureSpeech);
                        }
                        std::optional<float> width;
                        if (const auto actor = speechEnvironment.find(speaker)) width = actor->body.x;
                        if (const auto pitch = npcSpeech.voice(speaker, layout, width, clientRand)) {
                            int64_t at[3]; playerUnits(at);
                            audio.playAt(0x35, at, 1, *pitch);
                        }
                    });
                hud.drawDialogueChoices(answers, {(float)pixels.width, (float)pixels.height},
                                        {(float)mouseX, (float)mouseY}, out, &answerPoints);
                if (generalDialogue.visible()) {
                    const auto drawn = hud.drawGeneralDialogue(generalDialogue, {(float)pixels.width, (float)pixels.height},
                                                               (int32_t)(int64_t)(time * 1000), out);
                    std::optional<float> speakerWidth;
                    if (const auto *speaker = spawned.record(generalDialogue.speaker())) {
                        float width;
                        std::memcpy(&width, speaker + 0x84, sizeof width);
                        speakerWidth = width;
                    }
                    if (const auto pitch = generalDialogue.voice(drawn, speakerWidth, clientRand)) {
                        int64_t at[3];
                        playerUnits(at);
                        audio.playAt(0x35, at, 1, *pitch);
                    }
                }
            });
            std::vector<glm::vec2> afterShop;
            if (hs.identification) {
                const auto layout = hud.identificationLayout();
                if (layout.valid) {
                    const auto points = layout.navigation();
                    afterShop.insert(afterShop.end(), points.begin(), points.end());
                }
            }
            afterShop.insert(afterShop.end(), answerPoints.begin(), answerPoints.end());
            if (menusReady ? menus.interfaceShown() : true) {
                const auto points = hs.inventory && hud.hasPanels() ? hud.inventoryNavigationPoints(*hs.inventory, afterShop) : afterShop;
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                const auto size = renderer.screenSize();
                for (const auto &point : points)
                    input.gamepadNavigationPoints.push_back(
                        {(int32_t)(point.x * width / std::max(1u, size.width)), (int32_t)(point.y * height / std::max(1u, size.height))});
            }
            if (!captureFeat.empty() && !featFixtureCaptured && !featLabels.empty() &&
                (!menusReady || menus.interfaceShown()) && hud.hasInterface()) {
                renderer.capture(captureFeat);
                featFixtureCaptured = true;
            }
            if (menusReady && !menus.interfaceShown()) // F4: the whole interface hidden
                hudVertices.clear();
            else if (hs.gold >= 0 && hud.hasInterface()) { // the money's turning coin (hud-money.md §2)
                static hudcoin::Coin hudCoin;
                const VkExtent2D px = renderer.screenSize();
                hudCoin.draw(renderer, assets, opaque, (float)px.width, (float)px.height, hud.interfaceScale(), worldClockMs);
            }
            if (menusReady)
                menus.draw(hudVertices);
            if (menusReady && !menus.interfaceShown())
                renderer.overlay(hudVertices);
            else
                renderer.overlay(hudVertices, hud.itemDraws());
        }
        Camera camera{eye, target};
        camera.displayedYawDegrees = (float)((double)viewYaw * 57.29577951308232 - 90.0);
        // The map (the game's frame update, the map rendering): its position, its overlay between the
        // corner and the middle, its regions.
        if (useExtracted && !serverMode) {
            if (mapTest >= 0) {
                mapOpen = true;
                flightmasterMode = flightsTest;
                if (shrinesTest)
                    mapSelection.openShrines();
                mapZoom = std::clamp((int)mapTest, 0, worldmap::MAX_ZOOM);
                mapTest = -1;
            }
            const glm::dvec3 playerBlocks((double)player.position.x + (double)(extractedWorld.originDotsX() >> 16),
                                          (double)player.position.y + (double)(extractedWorld.originDotsY() >> 16),
                                          (double)player.position.z - ExtractedWorld::Z_OFFSET);
            if (!mapOpen)
                mapMoved = false;
            if (!mapMoved) {
                targetMapPosition = playerBlocks;
                mapPosition = targetMapPosition;
            } else
                targetMapPosition.z = worldgen::groundHeight((int32_t)std::floor(targetMapPosition.x), (int32_t)std::floor(targetMapPosition.y));
            const double stepMs = dt * 1000.0;
            mapPosition += (targetMapPosition - mapPosition) * (double)gamemath::blendFactor(stepMs, 0.01f);
            const VkExtent2D size = renderer.screenSize();
            const float w = (float)std::max(1u, size.width), h = (float)std::max(1u, size.height);
            const glm::vec2 cornerPosition(1.0f - h * 0.3f / w * minimapScale, 1.0f - minimapScale * 0.2f);
            if (!mapPlaced)
                mapViewCentre = cornerPosition, mapPlaced = true;
            const float f = gamemath::blendFactor(stepMs, 0.01f);
            mapViewCentre += ((mapOpen ? glm::vec2(0.5f, 0.6f) : cornerPosition) - mapViewCentre) * f;
            mapLabelScale += ((mapOpen ? 1.8f : minimapScale) - mapLabelScale) * f;
            // §4.6 halves the radius on a flag the spec reads as Control held (its open question 2).
            // The vanilla screenshots (options mapScale 0.75) show the corner map at half radius
            // and the open map whole, so the flag is taken as "the map is not open".
            const bool halfRadius = !mapOpen;
            const int radius = halfRadius ? worldmap::RADIUS / 2 : worldmap::RADIUS;
            worldMap.update(renderer, mapPosition, mapZoom);
            worldmap::Map::View view;
            view.center = mapPosition;
            view.level = mapZoom;
            view.radius = radius;
            view.pitch = viewPitch, view.yaw = viewYaw;
            view.cursor = mapViewCentre;
            view.viewScale = minimapScale * mapLabelScale; // options mapScale x the eased scale (§4.1)
            view.screen = glm::vec2(w, h);
            // The player's own marker models on the map: its head and hair, the hair in its colour.
            if (!mapPlayerSet) {
                mapPlayerSet = true;
                CubModel head, hair;
                glm::u8vec3 hairColor(255);
                bool haveHead = false, haveHair = false;
                for (const auto &part : character.parts()) {
                    if (part.matrix != 3 && part.matrix != 4)
                        continue;
                    if (auto path = assets.resolve(fs::path("sprites") / (part.sprite + ".cub"))) {
                        if (part.matrix == 3)
                            haveHead = head.load(*path);
                        else
                            haveHair = hair.load(*path), hairColor = part.tint;
                    }
                }
                if (haveHead)
                    mapPlayerLooks = worldMap.looks(renderer, head, haveHair ? &hair : nullptr, hairColor);
                if (auto path = assets.resolve(fs::path("sprites") / "cube.cub")) { // the flight route's dot (the cube sprite)
                    CubModel cube;
                    if (cube.load(*path))
                        worldMap.setCube(renderer, cube);
                }
            }
            view.player = playerBlocks;
            view.worldClockMs = time * 1000.0;
            { // the route: to the selected flight point, or the flight's destination while flying
                float flightMs = 0;
                const uint8_t *me = spawned.playerRecord();
                if (flightSelected)
                    view.route = true, view.routeTarget = glm::ivec2(selectedFlightPoint[0], selectedFlightPoint[1]);
                else if (me && spawned.playerAbility(flightMs) == 0x9c) {
                    int32_t to[2];
                    std::memcpy(to, me + 0x1b0, 8);
                    view.route = true, view.routeTarget = glm::ivec2(to[0], to[1]);
                }
                if (view.route)
                    view.routeTargetZ = (int)worldgen::groundHeight(view.routeTarget.x, view.routeTarget.y, 0);
            }
            { // The world map's creature markers (MapMarks.h): the player, the creatures leading it to a
              // mission, and the tombstone where it last died.
                static std::map<std::tuple<std::string, std::string, int>, int> looksOf; // (models, hair colour) -> Map::looks
                auto looks = [&](const std::string &first, const std::string &hairName, glm::u8vec3 colour) {
                    const auto key = std::make_tuple(first, hairName, (colour.r << 16) | (colour.g << 8) | colour.b);
                    if (auto known = looksOf.find(key); known != looksOf.end())
                        return known->second;
                    int index = -1;
                    CubModel model, hair;
                    const auto modelPath = first.empty() ? std::nullopt : assets.resolve(fs::path("sprites") / (first + ".cub"));
                    const auto hairPath = hairName.empty() ? std::nullopt : assets.resolve(fs::path("sprites") / (hairName + ".cub"));
                    if (modelPath && model.load(*modelPath))
                        index = worldMap.looks(renderer, model, hairPath && hair.load(*hairPath) ? &hair : nullptr, colour);
                    return looksOf[key] = index;
                };
                worldmap::Map::View::Mark me; // (a player: no pulse)
                me.looks = mapPlayerLooks;
                me.at = playerBlocks;
                me.turnDegrees = glm::degrees(heading - character.forward()); // (as the body turns)
                view.marks.push_back(me);
                spawned.forEach([&](const uint8_t *c) {
                    const int hostility = c[worldgen::creature::HOSTILITY], ability = c[worldgen::creature::ABILITY];
                    uint16_t toggles;
                    int32_t destination[2];
                    std::memcpy(&toggles, c + worldgen::creature::TOGGLES, 2), std::memcpy(destination, c + 0x1b0, 8);
                    const bool destinationDone =
                        missions::completed(saveGame.finishedMissions, saveGame.solvedMissions, {destination[0], destination[1]});
                    if (!mapmarks::showsCreature(hostility, ability, toggles, destinationDone))
                        return;
                    // Its head (its chest when it has none) and its hair, from its looks (+0x78).
                    int16_t head, hair, chest;
                    std::memcpy(&head, c + 0x90, 2), std::memcpy(&hair, c + 0x92, 2), std::memcpy(&chest, c + 0x98, 2);
                    std::string first = spawned.spriteName(head);
                    if (first.empty())
                        first = spawned.spriteName(chest);
                    int64_t shown[3]; // where it is drawn (+0xb10), and its turn (+0xb3c, degrees)
                    float turn;
                    std::memcpy(shown, c + 0xb10, 24), std::memcpy(&turn, c + 0xb3c, 4);
                    worldmap::Map::View::Mark k;
                    k.looks = looks(first, spawned.spriteName(hair), glm::u8vec3(c[0x7a], c[0x7b], c[0x7c]));
                    k.at = glm::dvec3((double)shown[0], (double)shown[1], (double)shown[2]) / 65536.0;
                    k.turnDegrees = turn;
                    k.pulses = hostility != 0;
                    view.marks.push_back(k);
                });
                if (saveGame.tombstoneMs > 0) {
                    constexpr int TOMBSTONE_MODEL = 0xde7; // tombstone.cub in the world's model table
                    worldmap::Map::View::Mark k;
                    k.looks = looks(spawned.spriteName(TOMBSTONE_MODEL), "", glm::u8vec3(255));
                    k.at = glm::dvec3((double)saveGame.deathPosition[0], (double)saveGame.deathPosition[1], (double)saveGame.deathPosition[2]);
                    k.pulses = true;
                    view.marks.push_back(k);
                }
            }
            worldMap.collect(view, opaque);
            if (!mapCursorFromOverlay)
                mapCursorKnown = false;
            if (mapOpen && !halfRadius && !mapCursorFromOverlay) { // (no cursor on the minimap, nor at half radius: §4.6)
                double px, py;
                pointerPosition(&px, &py);
                if (pointerTestX >= 0)
                    px = pointerTestX, py = pointerTestY;
                mapCursorKnown = worldMap.pick(view, glm::vec2((float)px, (float)py), mapCursor);
                static bool told = false; // (testing: where the pointer meets the map)
                if (pointerTestX >= 0 && mapCursorKnown && !told && time > 5) {
                    told = true;
                    logLine("Map cursor: " + std::to_string((int64_t)mapCursor.x) + " " + std::to_string((int64_t)mapCursor.y) + " " +
                            std::to_string((int64_t)mapCursor.z) + " (the player at " +
                            std::to_string((int64_t)playerBlocks.x) + " " + std::to_string((int64_t)playerBlocks.y) + ")");
                }
            }
            // The map overlay's markers (MapMarks.h): places, missions, lands, flight points and
            // stars, drawn far to near; then the marker under the pointer.
            mapMarkerViews.clear();
            if (!mapOpen)
                flightmasterMode = false;
            {
                const int unit = worldmap::scaleOf(mapZoom);
                const int clockMs = (int)(time * 1000.0);
                std::set<std::array<int32_t, 2>> marked; // the missions marked on the map (+0xb8)
                for (const SaveGame::Marked &m : saveGame.marked)
                    marked.insert({m.x, m.y});
                auto isKnown = [&](mapmarks::Cell zone) { return missionsTest || marked.count(zone) || saveGame.solvedMissions.count(zone); };
                for (const worldmap::Map::Projected &p : worldMap.markers(view)) {
                    const worldgen::MapRegion::Marker &k = p.marker;
                    mapmarks::Marker v;
                    v.at = p.at;
                    v.depth = p.depth;
                    v.block = glm::ivec3(k.x, k.y, k.z);
                    // A place: shops and inns once their zone is discovered; shrines, lore buildings and
                    // treasure cells once their cell is known (used).
                    if (k.list == 0) {
                        constexpr int SHRINE = 12, LORE_BUILDING = 14, TREASURE = 15;
                        if (k.type == SHRINE || k.type == LORE_BUILDING || k.type == TREASURE) {
                            if (!usedCells.count({worldgen::floorDiv(k.x, 4), worldgen::floorDiv(k.y, 4)}))
                                continue;
                        } else if (worldgen::placeKind(k.px, k.py, k.big) != 0 && !discoveredPlaces.count({k.px, k.py}))
                            continue;
                        v.scale = mapmarks::placeScale(k.type, unit, mapLabelScale);
                        v.icon = mapmarks::placeIcon(k.type);
                        v.shrineTarget = k.type == SHRINE;
                        v.reportsHover = v.reportsCell = k.type == LORE_BUILDING; // (hovering a read lore building shows its lore)
                    } else if (k.list == 1) { // a place's mission
                        const mapmarks::Cell zone{k.px, k.py};
                        bool known = isKnown(zone);
                        constexpr int ARTIFACT_PLACE = 14;
                        const worldgen::Item artifact = k.kind == ARTIFACT_PLACE ? worldgen::placeArtifact(k.px, k.py) : worldgen::Item{};
                        if (k.kind == ARTIFACT_PLACE && !known) { // ten pieces of its realm's lore reveal it
                            const auto realmLore = loreBook.find(artifact.realm);
                            if (mapmarks::revealedByLore(k.kind, known, realmLore == loreBook.end() ? 0 : realmLore->second.size())) {
                                known = true;
                                marked.insert(zone);
                                saveGame.marked.push_back({k.px, k.py, "", ""});
                                saveGame.missionTimers[zone] = mapmarks::LORE_NOTE_MS;
                                logLine("Lore revealed the artifact of place " + std::to_string(k.px) + "," + std::to_string(k.py));
                            }
                        }
                        mapmarks::MissionFacts facts;
                        facts.kind = k.kind, facts.level = k.level, facts.unit = unit, facts.mapScale = mapLabelScale;
                        facts.discovered = discoveredPlaces.count(zone) != 0;
                        facts.known = known;
                        facts.completed = missions::completed(saveGame.finishedMissions, saveGame.solvedMissions, zone);
                        facts.newNote = saveGame.missionTimers.count(zone) != 0;
                        facts.clockMs = clockMs;
                        facts.showRarity = showRarity != 0;
                        const mapmarks::MissionLook look = mapmarks::missionLook(facts);
                        if (!look.shown)
                            continue;
                        v.scale = look.scale;
                        v.colour = look.colour;
                        if (look.named) {
                            v.label = placeNameOf(k.px, k.py, k.type);
                            if (look.stars > 0) // (the font draws U+00B1 as a star)
                                v.label += "\n" + std::string((size_t)look.stars, (char)0xb1);
                        }
                        if (look.icon)
                            v.icon = look.icon;
                        if (look.solved)
                            v.overlay = "solved";
                        if (look.note)
                            v.note = "NEW";
                        v.reportsHover = look.reportsHover;
                        v.item = artifact;
                    } else if (k.list == 3) { // a flight point, while a flight master shows them
                        const mapmarks::Size size = mapmarks::pointSize(unit, mapLabelScale);
                        if (!flightmasterMode || !size.shown)
                            continue;
                        v.scale = size.scale;
                        v.icon = boughtFlights.count({k.x, k.y}) ? "flight-point" : "cross";
                        v.flightTarget = v.reportsHover = v.flightPoint = true;
                    } else { // a land's name
                        if (!discoveredAreas.count({k.px, k.py}))
                            continue;
                        v.scale = mapmarks::landSize(unit, mapLabelScale).scale;
                        v.label = landNameOf({k.px, k.py});
                        v.colour = mapmarks::landColour(k.type);
                    }
                    mapMarkerViews.push_back(v);
                }
                // The stars the player set on the map (the character's map stars).
                const mapmarks::Size starSize = mapmarks::pointSize(unit, mapLabelScale);
                if (starSize.shown)
                    for (const auto &t : saveGame.treasures) {
                        mapmarks::Marker v;
                        v.block = glm::ivec3(t[0], t[1], t[2]);
                        if (!worldMap.project(view, glm::dvec3(v.block), v.at, &v.depth))
                            continue;
                        v.icon = "star";
                        v.scale = starSize.scale;
                        v.reportsHover = true;
                        mapMarkerViews.push_back(v);
                    }
                std::stable_sort(mapMarkerViews.begin(), mapMarkerViews.end(),
                                 [](const mapmarks::Marker &a, const mapmarks::Marker &b) { return a.depth > b.depth; });
                // The quests' sprites (OverworldEvents' MapMarkers), over the game's markers: an
                // orange question mark on an encounter, a grey one on an accepted objective, at 0.6
                // of the stars' size.
                if (starSize.shown)
                    for (const OverworldEvents::Pin &pin : events.pins()) {
                        mapmarks::Marker v;
                        if (!worldMap.project(view, glm::dvec3((double)pin.x, (double)pin.y, (double)pin.z), v.at))
                            continue;
                        v.questSprite = pin.destination ? "quest-destination" : "quest-event";
                        v.scale = starSize.scale * 0.6f;
                        mapMarkerViews.push_back(v);
                    }
            }
        }
        if (serverMode) {
            // A dedicated server draws nothing: it keeps the world running at about
            // 60 updates a second and reports who is playing now and then.
            static double reported = -100;
            if (time - reported > 30) {
                reported = time;
                logLine("Server: " + std::to_string(session.guests()) + " player(s) connected");
            }
            // The shared world is saved as the game's host saves it, once a minute (native-saves.md §5.5).
            static double worldSaved = 0;
            if (spawnedReady && savesWritable && saves.isOpen() && time - worldSaved > 60) {
                worldSaved = time;
                const auto persistentDead = hostWorld.persistentDeadSet();
                saves.world().dead = persistentDead;
                saves.saveWorld(hostWorld.day(), hostWorld.timeOfDay(), persistentDead);
            }
            Sleep(16);
        } else {
            renderer.setWorldDay(day); // the moon's phase
            if (spawnedReady) {
                lighting.timeMs = (float)spawned.timeOfDay();
                float strength, remainingMs;
                if (spawned.playerBuff(0x13, strength, remainingMs))
                    lighting.effect13RemainingMs = remainingMs;
            }
            renderer.frame(camera, lighting, player.position, time, opaque, water);
        }
        auto column = columnAt(player.position.x, player.position.y);
        {
            const float ambientFrameMs = (float)(dt * 1000.0);
            int64_t here[3];
            playerUnits(here);
            const int32_t bx = worldgen::floorDots(here[0]), by = worldgen::floorDots(here[1]), bz = worldgen::floorDots(here[2]);
            // The music (sound.md §3): the place's track, or the shrine's, the spirit world's,
            // a boss's or the success track, with the game's waits and fades.
            constexpr uint8_t SHRINE_FLAG = 0x20; // the block's flag byte (+5)
            bool shrineLoaded = false;
            const bool shrine = (extractedWorld.gameBlock(bx, by, bz, &shrineLoaded)[5] & SHRINE_FLAG) && shrineLoaded;
            soundrules::MusicPlace place = musicPlaceAt(bx, by, shrine);
            float strength = 0, left = 0;
            constexpr int SPIRIT_WORLD_BUFF = 0x13;
            place.spiritWorld = spawned.playerRecord() && spawned.playerBuff(SPIRIT_WORLD_BUFF, strength, left);
            place.boss = spawnedReady && spawned.bossAfterPlayer();
            audio.updateMusic(place, std::min(frameMs, 1000.0));

            // The ambient sounds (§4): the emitters of the 3 x 3 zones around the player, or
            // around where it flies to (ability 0x9c, +0x1b0).
            int32_t centre[2] = {bx, by};
            float flightMs = 0;
            if (const uint8_t *me = spawned.playerRecord(); me && spawned.playerAbility(flightMs) == 0x9c)
                std::memcpy(centre, me + worldgen::creature::HOME_PLACE, sizeof centre);
            soundrules::AmbientFrame frame;
            frame.clockMs = worldClockMs, frame.dtMs = ambientFrameMs, frame.timeOfDayMs = hour * 3600000.0f;
            constexpr int ZONE_SHIFT = 6; // zones of 64 blocks
            const int32_t zx = worldgen::floorDiv(centre[0], ZONE_SHIFT), zy = worldgen::floorDiv(centre[1], ZONE_SHIFT);
            for (int32_t ax = zx - 1; ax <= zx + 1; ax++)
                for (int32_t ay = zy - 1; ay <= zy + 1; ay++) {
                    // Only zones in memory sound. HOOK: the game also skips a zone whose +0x234 is
                    // 4096 or more; the spec does not say what that field is (open question 6).
                    bool loaded = false;
                    extractedWorld.gameBlock((int64_t)ax * 64, (int64_t)ay * 64, 0, &loaded);
                    if (!loaded)
                        continue;
                    auto [cached, fresh] = zoneEmitters.try_emplace({ax, ay});
                    if (fresh) {
                        cached->second = extractedWorld.soundEmitters(ax, ay);
                        if (cached->second.empty())
                            cached->second = worldgen::soundEmitters(ax, ay);
                    }
                    soundrules::ambientZone(cached->second, frame, clientRand,
                                            [&](int number, const int64_t at[3], float volume, float pitch) {
                                                audio.playAt(number, at, volume, pitch);
                                            });
                }
            worldClockMs += frameMs;
            world.advanceClock(frameMs); // (the plants sway by it, vegetation-wind.md §2)
        }
        audio.update();

        framesSinceTitle++;
        if (now - titleTime > std::chrono::seconds(1)) {
            char title[320];
            std::string targetText;
            if (struck >= 0 && struck < (int)actors.count() && actors.visible(struck)) {
                char t[64];
                std::snprintf(t, sizeof t, "  |  target %.0f/%.0f", actors.health(struck), actors.maxHealth(struck));
                targetText = t;
            }
            int h = (int)hour, m = (int)((hour - h) * 60);
            std::snprintf(title, sizeof title,
                          "CubeVulkan  |  HP %.0f/%.0f%s  |  %u fps  |  %02d:%02d  |  %s  |  %.0f, %.0f, %.0f  |  zones %zu (+%zu)  |  draws %u%s",
                          playerHp, playerMaxHp, targetText.c_str(), framesSinceTitle, h, m, biomeName(column.biome), player.position.x,
                          player.position.y, player.position.z, world.zoneCount(),
                          world.pendingCount(), renderer.drawsLastFrame(),
                          shadowsEnabled ? "" : "  |  shadows off");
            glfwSetWindowTitle(window, title);
            if (autoAttack)
                logLine(title);
            framesSinceTitle = 0;
            titleTime = now;
        }
        // --benchmark: once the world has loaded, count frames for N seconds.
        if (benchmark > 0) {
            static double startedAt = -1;
            static unsigned counted = 0;
            if (startedAt < 0 && world.pendingCount() == 0 && time > 3)
                startedAt = time;
            if (startedAt >= 0) {
                counted++;
                if (time - startedAt >= benchmark) {
                    char line[256];
                    std::snprintf(line, sizeof line,
                                  "Benchmark: %.1f fps (%.2f ms per frame) over %.0f s; GPU %.2f ms per frame, of which "
                                  "anti-aliasing and interface %.2f ms (TAA %s, SMAA %s)",
                                  counted / (time - startedAt), 1000 * (time - startedAt) / counted, time - startedAt,
                                  renderer.averageGpuMs(), renderer.averagePostMs(),
                                  renderer.settings.taa ? "on" : "off", renderer.settings.smaa ? "on" : "off");
                    logLine(line);
                    break;
                }
            }
        }
        // The FPS limit (minTimeStep): a frame lasts at least that many milliseconds. CPU usage
        // REDUCED: what the game does differently is not specified (frontend-options.md §1.2);
        // CubeVulkan gives the processor up for a millisecond each frame.
        if (benchmark <= 0) {
            if (const int shortest = gameOptions.shortestFrameMs(); shortest > 0)
                std::this_thread::sleep_until(now + std::chrono::milliseconds(shortest));
            if (gameOptions.cpuPerformance == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // --run-seconds alone: play that long, then quit (and save) as if closed.
        if (capture.empty() && runSeconds > 0 && time > runSeconds)
            break;
        // --capture: wait for the world around the player, then save and quit.
        if (!capture.empty()) {
            settledFrames = world.pendingCount() == 0 && time >= runSeconds ? settledFrames + 1 : 0;
            if (settledFrames == 30)
                renderer.capture(capture);
            if (settledFrames == 32 || time > 120 + runSeconds)
                break;
        }
    }
    if (!captureFeat.empty()) {
        logLine(featFixtureCaptured ? "Feat capture: visible owner-font frame captured" : "Feat capture: no visible frame");
        if (!featFixtureCaptured) exitCode = 1;
    }
    if (lastTalkedSpeaker && lastTalkedSpeaker->native) {
        logLine("NPC speech retained: " + std::to_string(npcSpeech.bubbles().size()) + " bubbles, " +
                std::to_string(npcSpeech.emotes().size()) + " emotes");
        for (const auto &bubble : npcSpeech.bubbles())
            logLine("NPC speech root: page " + std::to_string(bubble.page) + "/" + std::to_string(bubble.pages.size()) +
                " clock " + std::to_string(bubble.elapsedMs) + " visible " + std::to_string(bubble.visible) +
                " at " + std::to_string(bubble.screenPoint.x) + "," + std::to_string(bubble.screenPoint.y));
    }
    if (spawnedReady) // every zone in memory records what changed in it (native-saves.md open question 3)
        hostWorld.recordAllZones();
    if (serverMode && spawnedReady && savesWritable && saves.isOpen()) {
        const auto persistentDead = hostWorld.persistentDeadSet();
        saves.world().dead = persistentDead;
        logLine(saves.saveWorld(hostWorld.day(), hostWorld.timeOfDay(), persistentDead) ? "Save: world written"
                                                                                    : "Save: world not written");
    }
    if (saving) {
        std::string where = saveProgress();
        logLine(where.empty() ? "Save: not written" : "Save: written (" + where + ")");
    }
    for (auto &name : world.missingModels)
        logLine("Missing sprite: " + name + ".cub");
    mods.shutdown();
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    if (const auto ranges = renderer.nativeRanges())
        logLine("Steam scene ranges: scene " + std::to_string(ranges->scene) + " models " + std::to_string(ranges->models) +
                " abilities " + std::to_string(ranges->abilities) + " zone radius " + std::to_string(world.viewZoneCount()));
    logLine("CubeVulkan stopped");
    return returnToTitle && !exitCode ? 2 : exitCode;
}

// A crash writes what failed and where (function names from CubeVulkan.pdb) to
// CubeVulkan.log, so a report needs no debugger.
static LONG WINAPI logCrash(EXCEPTION_POINTERS *info) {
    char line[256];
    std::snprintf(line, sizeof line, "Crash: exception 0x%08lx at %p", info->ExceptionRecord->ExceptionCode,
                  info->ExceptionRecord->ExceptionAddress);
    logLine(line);
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(process, nullptr, TRUE);
    CONTEXT context = *info->ContextRecord;
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Rip, frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp, frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp, frame.AddrStack.Mode = AddrModeFlat;
    for (int depth = 0; depth < 24 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context,
                                                   nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr);
         depth++) {
        alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
        auto *symbol = (SYMBOL_INFO *)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;
        DWORD64 offset = 0;
        IMAGEHLP_LINE64 source{sizeof(IMAGEHLP_LINE64)};
        DWORD column = 0;
        bool named = SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol);
        bool lined = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &column, &source);
        std::snprintf(line, sizeof line, "  %s%s%s:%lu", named ? symbol->Name : "?", lined ? " " : "",
                      lined ? source.FileName : "", lined ? source.LineNumber : 0ul);
        logLine(line);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char **argv) {
    SetUnhandledExceptionFilter(logCrash);
    int result = runSession(argc, argv, false);
    while (result == 2)
        result = runSession(argc, argv, true);
    return result;
}
