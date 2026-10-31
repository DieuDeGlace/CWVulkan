#pragma once
// Sound and music (docs/specs/sound.md), played from the owner's install: sounds/*.wav by the
// game's sound numbers and music/*.ogg by track name, mod folders overriding either.
//
// Two layers, as in the game:
//   * the output (§1): a sound plays once from start to end at a volume and a frequency ratio
//     (capped at 2.0), with no limit on how many play at once; one looping music track.
//   * the game's side (§2, §3): sounds are placed relative to the camera's eye (quieter with
//     the square of the distance, dropped beyond 64 blocks, no panning), the world's sound
//     requests are played each frame, and the music follows the place with the game's waits
//     and fades. The rules themselves are in SoundRules.h.
// Without a sound device the game's side still runs (and logs, with setLog), playing nothing.
#include "SoundRules.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Assets;

class Audio {
  public:
    Audio();
    ~Audio();
    // Opens the sound output and reads the sound list the extraction tool wrote
    // (Extract/sounds.txt: one "<number> <file name>" per line). False without a device.
    bool init(const Assets &assets);
    bool ok() const;
    // How many sound numbers the list names (0 when it was not extracted).
    size_t soundsKnown() const;

    // The options (options.cfg's soundVolume, musicVolume and musicLoop; percent).
    void setVolumes(int musicPercent, int soundPercent);
    void setMusicLoop(bool loop);
    // Debug: every sound placed (number, distance, volume, pitch) is described to `log`.
    void setLog(std::function<void(const std::string &)> log);

    // --- Sounds ---
    // The listener: the camera's eye, in world units (1/65536 block).
    void setListener(const int64_t eye[3]);
    // A sound at a world position (0xa0420): attenuated from the listener, dropped beyond 64
    // blocks, scaled by the sound option; an instrument's note ducks the music.
    void playAt(int number, const int64_t position[3], float volume, float pitch);
    // An interface sound: at the listener, full volume, normal pitch (0xa03f0).
    void playInterface(int number);
    // The world's sound requests of a frame, in order (§2.1). An explosion shakes the camera
    // from squared distance to the physical player, regardless of audibility.
    void playRequests(const std::vector<soundrules::RequestRecord> &requests, const int64_t player[3]);
    // The camera shake explosions raised (game +0x1e00); the camera lets it die down.
    float cameraShake() const;
    void setCameraShake(float shake);

    // --- Music ---
    // The success track is wanted for a minute (a mission done, a special item obtained).
    void startSuccess();
    bool successRunning() const;
    // The game update's music step (§3.3, §3.4, §3.5) for the place the player is in: counts
    // the duck and success timers down, chooses the track and moves the music on.
    void updateMusic(const soundrules::MusicPlace &place, double dtMs);
    // The same while a menu screen shows outside the game: the title theme.
    void menuMusic(double dtMs);
    // The track now chosen (for the log).
    const std::string &currentTrack() const;

    // Releases the sounds that have finished (once a frame).
    void update();

  private:
    struct State;
    std::unique_ptr<State> s;
    void playSound(int number, float volume, float pitch); // the output's play (§1.3)
    void musicStep(double dtMs, const soundrules::Track &wanted);
    void feedMusic(); // the music thread's step (§3.2)
};
