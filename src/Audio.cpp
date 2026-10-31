// Sound and music (docs/specs/sound.md). See Audio.h.
#include "Audio.h"

#include "Assets.h"

#include <miniaudio.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <list>
#include <map>
#include <sstream>

using namespace soundrules;

namespace {
// The output's lowest frequency ratio (the game's voices are made with the default minimum).
constexpr float MIN_PITCH = 1.0f / 1024.0f;
// Where the extraction tool writes the sound list, under the install.
const char *const SOUND_LIST = "Extract/sounds.txt";

std::string hex(int n) {
    char text[16];
    std::snprintf(text, sizeof text, "0x%02x", n);
    return text;
}
} // namespace

struct Audio::State {
    // The output.
    ma_engine engine{};
    bool ready = false;
    std::list<ma_sound> voices; // sounds still playing (a list: a voice must not move)
    std::map<int, std::wstring> files; // sound number -> its file (§1.2)

    // The options.
    int soundOption = 60, musicOption = 45;
    bool musicLoop = false;
    std::function<void(const std::string &)> log;

    // The game's side.
    int64_t listener[3] = {0, 0, 0};
    float duckMs = 0;      // game +0x204c
    float successMs = 0;   // game +0x2050
    float shake = 0;       // game +0x1e00
    MusicState music;

    // The music streamer (§3.1) and what the music thread last started (§3.2).
    const Assets *assets = nullptr;
    ma_sound track{};
    bool trackOpen = false;
    std::string started;

    void stopTrack() {
        if (trackOpen)
            ma_sound_uninit(&track);
        trackOpen = false;
    }
};

Audio::Audio() : s(std::make_unique<State>()) {}

Audio::~Audio() {
    if (s->ready) {
        s->stopTrack();
        for (ma_sound &v : s->voices)
            ma_sound_uninit(&v);
        ma_engine_uninit(&s->engine);
    }
}

bool Audio::init(const Assets &assets) {
    s->assets = &assets;
    // The sound list (§1.2): the game's own numbering, read from the owner's install.
    std::ifstream list(assets.gameDir() / SOUND_LIST);
    for (std::string line; std::getline(list, line);) {
        std::istringstream in(line);
        int number;
        std::string name;
        if (!(in >> number >> std::ws) || !std::getline(in, name) || name.empty())
            continue;
        while (!name.empty() && (name.back() == '\r' || name.back() == ' '))
            name.pop_back();
        if (auto path = assets.resolve(fs::path("sounds") / pathOfUtf8(name)))
            s->files[number] = path->wstring();
    }
    s->ready = ma_engine_init(nullptr, &s->engine) == MA_SUCCESS;
    return s->ready;
}

bool Audio::ok() const { return s->ready; }
size_t Audio::soundsKnown() const { return s->files.size(); }

void Audio::setVolumes(int musicPercent, int soundPercent) {
    s->musicOption = musicPercent, s->soundOption = soundPercent;
    // Changing the option sets the music volume at once (0x89c10).
    s->music.volume = (float)musicPercent * OPTION_SCALE;
}

void Audio::setMusicLoop(bool loop) { s->musicLoop = loop; }
void Audio::setLog(std::function<void(const std::string &)> log) { s->log = std::move(log); }

// --- The output (§1) ------------------------------------------------------------------------------

void Audio::playSound(int number, float volume, float pitch) {
    if (!s->ready || volume <= 0)
        return;
    const auto file = s->files.find(number);
    if (file == s->files.end()) // not in the list: plays nothing
        return;
    ma_sound &voice = s->voices.emplace_back();
    const ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
    if (ma_sound_init_from_file_w(&s->engine, file->second.c_str(), flags, nullptr, nullptr, &voice) != MA_SUCCESS) {
        s->voices.pop_back();
        return;
    }
    ma_sound_set_volume(&voice, volume);
    ma_sound_set_pitch(&voice, std::max(MIN_PITCH, cappedPitch(pitch)));
    ma_sound_start(&voice);
}

void Audio::update() {
    if (!s->ready)
        return;
    for (auto v = s->voices.begin(); v != s->voices.end();) {
        if (ma_sound_at_end(&*v)) {
            ma_sound_uninit(&*v);
            v = s->voices.erase(v);
        } else
            ++v;
    }
}

// --- Placing sounds (§2) ----------------------------------------------------------------------------

void Audio::setListener(const int64_t eye[3]) { std::copy(eye, eye + 3, s->listener); }

void Audio::playAt(int number, const int64_t position[3], float volume, float pitch) {
    const float d2 = distanceSquared(position, s->listener);
    const Heard heard = attenuate(d2);
    if (s->log) {
        std::string line = "Sound " + hex(number) + " at " + std::to_string((int)std::sqrt(d2)) + " blocks";
        line += heard.audible ? ": volume " + std::to_string(finalVolume(s->soundOption, volume, heard.attenuation)) +
                                    ", pitch " + std::to_string(cappedPitch(pitch))
                              : ": too far";
        s->log(line);
    }
    if (!heard.audible)
        return;
    if (isInstrument(number))
        s->duckMs = std::max(s->duckMs, DUCK_MS);
    playSound(number, finalVolume(s->soundOption, volume, heard.attenuation), pitch);
}

void Audio::playInterface(int number) { playAt(number, s->listener, 1.0f, 1.0f); }

void Audio::playRequests(const std::vector<RequestRecord> &requests, const int64_t player[3]) {
    for (const RequestRecord &record : requests) {
        const Request r = readRequest(record);
        playAt(r.number, r.position, r.volume, r.pitch);
        if (isExplosion(r.number))
            s->shake = std::max(s->shake, explosionShake(distanceSquared(r.position, player)));
    }
}

float Audio::cameraShake() const { return s->shake; }
void Audio::setCameraShake(float shake) { s->shake = shake; }

// --- Music (§3) -------------------------------------------------------------------------------------

void Audio::startSuccess() { s->successMs = SUCCESS_MS; }
bool Audio::successRunning() const { return s->successMs > 0; }
const std::string &Audio::currentTrack() const { return s->music.current; }

void Audio::updateMusic(const MusicPlace &place, double dtMs) {
    dtMs = std::min(dtMs, 1000.0);
    s->successMs = (float)std::max(0.0, (double)s->successMs - dtMs);
    s->duckMs = (float)std::max(0.0, (double)s->duckMs - dtMs);
    MusicPlace here = place;
    here.success = s->successMs > 0;
    musicStep(dtMs, chooseTrack(here, s->music.current));
}

void Audio::menuMusic(double dtMs) {
    MusicPlace menu;
    menu.menu = true;
    musicStep(dtMs, chooseTrack(menu, s->music.current));
}

void Audio::musicStep(double dtMs, const Track &wanted) {
    const bool playing = s->trackOpen && ma_sound_is_playing(&s->track);
    const std::string before = s->music.current;
    MusicOptions options;
    options.volume = s->musicOption, options.loop = s->musicLoop;
    stepMusic(s->music, wanted, playing, std::min(dtMs, 1000.0), options, s->successMs > 0, s->duckMs);
    if (s->log && s->music.current != before)
        s->log("Music: " + s->music.current);
    feedMusic();
}

void Audio::feedMusic() {
    // Quiet music is stopped; a track not yet started starts, looping (the music thread).
    if (s->music.volume < MUSIC_STOP_VOLUME)
        s->stopTrack();
    else if (s->trackOpen)
        ma_sound_set_volume(&s->track, s->music.volume);
    if (s->music.current == s->started)
        return;
    s->started = s->music.current;
    s->stopTrack();
    if (!s->ready || !s->assets || s->music.current.empty())
        return;
    const auto path = s->assets->resolve(fs::path("music") / pathOfUtf8(s->music.current + ".ogg"));
    if (!path)
        return;
    const ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
    if (ma_sound_init_from_file_w(&s->engine, path->wstring().c_str(), flags, nullptr, nullptr, &s->track) != MA_SUCCESS)
        return;
    s->trackOpen = true;
    ma_sound_set_looping(&s->track, MA_TRUE);
    ma_sound_set_volume(&s->track, s->music.volume);
    ma_sound_start(&s->track);
}
