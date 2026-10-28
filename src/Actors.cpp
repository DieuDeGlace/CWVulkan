#include "Actors.h"
#include "Combat.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace {
constexpr float PI = 3.14159265f;
// Monsters, as measured in the game on 2026-10-22 (ZoneExtractor's approach test,
// docs/creatures.md): a level 1 monster noticed the player at 27-28 blocks, chased at
// 11.5 blocks/s (the player's top speed), started its attack within 3.1 blocks and
// stood about 2 blocks off, swung back to back, gave up at 40-44 blocks (two trials)
// and walked away at 5.8 blocks/s. Townspeople's greeting distances are CubeVulkan's.
constexpr float AGGRO = 27.5f, GIVE_UP = 42, REACH = 3.1f, HOLD = 2, GREET = 5, GREET_END = 7;
constexpr float CHASE_SPEED = 11.5f, RETURN_SPEED = 5.8f;
constexpr double ATTACK_LENGTH = 1.05; // a swing (ability 76) when its clip was not recorded
constexpr float WANDER_RADIUS = 10, TURN_RATE = 6; // blocks; radians per second
constexpr float MAX_STEP_UP = 1.2f;                // blocks a walker climbs in one step

float wrap(float a) {
    while (a > PI)
        a -= 2 * PI;
    while (a < -PI)
        a += 2 * PI;
    return a;
}

// Index into `clip` at `value`, where `keys` grows monotonically (times or distance).
size_t at(const std::vector<double> &keys, double value) {
    auto it = std::upper_bound(keys.begin(), keys.end(), value);
    return it == keys.begin() ? 0 : size_t(it - keys.begin()) - 1;
}
} // namespace

std::vector<glm::mat4> Actors::bodyFrame(const CreatureRecordings::Creature &c, size_t frame) const {
    // Recorded parts are relative to the creature but still turned to its facing
    // at that moment; undo the facing so clips can be played in any direction.
    glm::mat4 unturn = glm::rotate(glm::mat4(1), -c.angles[frame], glm::vec3(0, 0, 1));
    std::vector<glm::mat4> out = c.poses[frame];
    for (auto &m : out)
        if (m[3][3] != 0)
            m = unturn * m;
    return out;
}

void Actors::build(const CreatureRecordings &recordings, bool none) {
    source = &recordings;
    actors.clear();
    const auto &all = recordings.all();
    for (size_t k = 0; k < all.size(); k++) {
        const auto &c = all[k];
        size_t n = c.times.size();
        if (!n || c.angles.size() != n)
            continue;
        if (none) // (the game's own creatures, townsfolk included, take their place)
            continue;
        Actor a;
        a.recording = k;
        switch (c.hostility) {
        case 1: a.kind = Kind::Monster; break;
        case 2: a.kind = Kind::Water; break;
        case 3: a.kind = Kind::Townsperson; break;
        case 4: a.kind = Kind::Companion; break;
        default: a.kind = Kind::Fixed; break;
        }
        // Horizontal speed between recorded frames.
        std::vector<float> speed(n, 0);
        for (size_t i = 0; i + 1 < n; i++) {
            double dt = c.times[i + 1] - c.times[i];
            if (dt > 1e-4)
                speed[i] = float(glm::length(glm::vec2(c.positions[i + 1] - c.positions[i])) / dt);
        }
        if (n > 1)
            speed[n - 1] = speed[n - 2];
        // Longest run of frames passing `test`, as a clip.
        auto cut = [&](auto test) {
            Clip best, cur;
            for (size_t i = 0; i <= n; i++) {
                if (i < n && test(i)) {
                    cur.frames.push_back(i);
                } else {
                    if (cur.frames.size() > best.frames.size())
                        best = cur;
                    cur = {};
                }
            }
            for (size_t j = 0; j < best.frames.size(); j++) {
                size_t f = best.frames[j];
                best.times.push_back(c.times[f] - c.times[best.frames[0]]);
                double d = j ? best.covered.back() + speed[best.frames[j - 1]] *
                                                         (c.times[f] - c.times[best.frames[j - 1]])
                             : 0;
                best.covered.push_back(d);
            }
            if (!best.frames.empty()) {
                best.duration = best.times.back();
                best.distance = best.covered.back();
            }
            return best;
        };
        bool moves = a.kind != Kind::Fixed;
        a.idle = cut([&](size_t i) { return speed[i] < .3f && c.states[i] == 0; });
        if (a.idle.frames.empty() || !moves)
            a.idle = cut([&](size_t) { return true; }); // The whole recording.
        if (moves) {
            a.walk = cut([&](size_t i) { return speed[i] > .8f && c.states[i] == 0; });
            a.move = cut([&](size_t i) { return c.states[i] != 0; });
        }
        if (!a.walk.empty() && a.walk.distance > .5) {
            a.speed = float(a.walk.distance / std::max(a.walk.duration, 1e-3));
            float sx = 0, sy = 0;
            for (size_t j = 0; j + 1 < a.walk.frames.size(); j++) {
                size_t f = a.walk.frames[j];
                glm::vec2 d = glm::vec2(c.positions[f + 1] - c.positions[f]);
                if (glm::length(d) < 1e-4f)
                    continue;
                float rel = std::atan2(d.y, d.x) - c.angles[f];
                sx += std::cos(rel);
                sy += std::sin(rel);
            }
            a.forward = std::atan2(sy, sx);
        } else
            a.walk = {};
        // The game's numbers; captures from before stats were recorded get a
        // level-1 monster's (the logged Radishling's).
        bool known = c.maxHp > 0;
        a.maxHp = a.hp = known ? c.maxHp : 244;
        a.attack = known ? c.attack : 24.8f;
        a.spell = known && c.spell > 0 ? c.spell : a.attack;
        a.armor = known ? c.armor : 4.2f;
        a.resistance = known && c.resistance > 0 ? c.resistance : a.armor;
        a.critical = known ? c.critical : .03f;
        a.flags2 = c.flags2.value_or(0);
        a.baseHp = a.maxHp, a.baseAttack = a.attack, a.baseSpell = a.spell, a.baseArmor = a.armor,
        a.baseResistance = a.resistance;
        // Its attack's ability (the most frequent one recorded) and its right-hand weapon.
        {
            std::map<int, int> seen;
            for (int id : c.abilities)
                if (id) seen[id]++;
            for (auto &[id, n] : seen)
                if (!a.ability || n > seen[a.ability]) a.ability = id;
            bool weapon = c.worn.size() > 7 && c.worn[7].category == 3;
            a.maxCombo = combat::maxCombo(weapon ? c.worn[7].id : -1, c.flags2.value_or(0));
        }
        a.home = a.position = c.positions[0];
        a.heading = c.angles[0] + a.forward;
        a.grounded = a.kind != Kind::Water && a.kind != Kind::Fixed;
        a.lift = NAN; // Measured against the floor on the first update.
        enter(a, State::Idle, 2 + std::uniform_real_distribution<double>(0, 6)(random));
        actors.push_back(std::move(a));
    }
}

void Actors::enter(Actor &a, State s, double length) {
    if (a.state != s)
        a.animTime = 0;
    a.state = s;
    a.stateTime = 0;
    a.stateLength = length;
}

void Actors::update(double dt, glm::vec3 player, const Floor &floor, combat::Defender playerDefense, float &playerDamage,
                    double *playerStun) {
    std::vector<float> damage(1, 0.f);
    std::vector<double> stuns(1, 0.);
    update(dt, std::vector<glm::vec3>{player}, floor, std::vector<combat::Defender>{playerDefense}, damage, &stuns);
    playerDamage += damage[0];
    if (playerStun)
        *playerStun = stuns[0];
}

void Actors::update(double dt, const std::vector<glm::vec3> &players, const Floor &floor,
                    const std::vector<combat::Defender> &defenses, std::vector<float> &damage,
                    std::vector<double> *stuns) {
    if (players.empty())
        return;
    damage.assign(players.size(), 0.f);
    if (stuns)
        stuns->assign(players.size(), 0.);
    // The player nearest to a point (creatures react to whoever is closest).
    auto nearest = [&](glm::vec3 at) {
        size_t best = 0;
        for (size_t p = 1; p < players.size(); p++)
            if (glm::length(glm::vec2(players[p] - at)) < glm::length(glm::vec2(players[best] - at)))
                best = p;
        return best;
    };
    std::uniform_real_distribution<float> unit(0, 1);
    // Walkers keep a little room between each other (and the player).
    constexpr float ROOM = 1.6f;
    for (size_t i = 0; i < actors.size(); i++) {
        Actor &a = actors[i];
        glm::vec3 player = players[nearest(a.position)];
        if (a.kind == Kind::Fixed || a.walk.empty() || glm::length(glm::vec2(player - a.position)) > 40)
            continue;
        glm::vec2 push(0);
        for (size_t j = 0; j < actors.size(); j++) {
            glm::vec2 d = glm::vec2(a.position - actors[j].position);
            float l = glm::length(d);
            if (j != i && l < ROOM && l > 1e-3f)
                push += d / l * (ROOM - l);
        }
        glm::vec2 fromPlayer = glm::vec2(a.position - player);
        float lp = glm::length(fromPlayer);
        if (lp < 1.2f && lp > 1e-3f)
            push += fromPlayer / lp * (1.2f - lp);
        a.position += glm::vec3(push * std::min(1.f, 4 * (float)dt), 0);
    }
    for (auto &a : actors) {
        if (a.dead) {
            a.deathTime += dt; // The game never brings a dead creature back (docs/creatures.md).
            continue;
        }
        a.sinceHurt += dt;
        if (a.sinceHurt > 5)
            a.hp = std::min(a.maxHp, a.hp + a.maxHp * .02f * (float)dt);
        const size_t target = nearest(a.position);
        const glm::vec3 player = players[target];
        a.cooldown -= dt;
        if ((a.sinceHit += dt) > combat::COMBO_RESET) // the game resets a combo 4 s after its last hit
            a.combo = 0;
        a.stateTime += dt;
        a.animTime += dt;
        if (a.grounded && std::isnan(a.lift)) {
            float h;
            if (floor(a.position.x, a.position.y, a.position.z + .5f, h))
                a.lift = a.position.z - h;
            else
                a.grounded = false; // Over unextracted ground: stay where recorded.
        }
        glm::vec2 toPlayer = glm::vec2(player - a.position);
        float distance = glm::length(toPlayer);
        bool canWalk = !a.walk.empty() && a.kind != Kind::Fixed;

        if (const combat::Active *p = a.effects.find(combat::POISON); p && combat::poisonTick(p->left, p->left - dt))
            poisonHits.push_back({size_t(&a - actors.data()), combat::taken(p->value, a.armor, 0, a.maxCombo)});
        a.effects.update(dt);
        if (a.stunned > 0) { // stunned: it neither acts nor moves
            if ((a.stunned -= dt) <= 0)
                a.sinceStun = 0;
            continue;
        }
        a.sinceStun += dt;

        // Reactions, by the game's hostility type.
        switch (a.kind) {
        case Kind::Monster:
            if (distance < AGGRO && canWalk && a.state != State::Chase && a.state != State::Attack)
                a.returning = false, enter(a, State::Chase, 1e9);
            else if (a.state == State::Chase && distance > GIVE_UP) {
                a.target = a.home;
                a.returning = true;
                enter(a, State::Wander, 60);
            }
            if ((a.state == State::Chase || a.state == State::Attack) && distance < REACH && a.cooldown <= 0) {
                combat::Attacker attacker{a.attack, a.spell, a.critical, a.ability, a.combo, a.maxCombo, 0, &a.effects};
                combat::Hit h = combat::hit(attacker, defenses[std::min(target, defenses.size() - 1)], random);
                float hit = h.damage;
                damage[target] += hit;
                a.hp = std::min(a.maxHp, a.hp + h.healed);
                if (hit > 0 && stuns &&
                    (float)combat::nativeRand(random) / 32767 < combat::stunChance(a.ability, 0, false, a.flags2, 0))
                    (*stuns)[target] = combat::stunSeconds(true);
                if (hit > 0) // a landed hit counts toward its combo
                    a.combo++, a.sinceHit = 0;
                a.cooldown = a.move.empty() ? ATTACK_LENGTH : a.move.duration; // swings follow each other
                if (!a.move.empty())
                    enter(a, State::Attack, a.move.duration); // The game's recorded attack.
            }
            break;
        case Kind::Townsperson:
        case Kind::Companion:
            if (distance < GREET && a.state != State::Face)
                enter(a, State::Face, 1e9);
            else if (a.state == State::Face && distance > GREET_END)
                enter(a, State::Idle, 2 + 4 * unit(random));
            break;
        default: break;
        }

        // What each state does.
        glm::vec2 goal(a.position);
        bool walking = false;
        switch (a.state) {
        case State::Idle:
            if (a.stateTime > a.stateLength && canWalk) {
                float angle = unit(random) * 2 * PI, r = WANDER_RADIUS * std::sqrt(unit(random));
                a.target = a.home + glm::vec3(std::cos(angle) * r, std::sin(angle) * r, 0);
                enter(a, State::Wander, 15);
            }
            break;
        case State::Wander:
            goal = glm::vec2(a.target);
            walking = true;
            if (glm::length(goal - glm::vec2(a.position)) < .5f || a.stateTime > a.stateLength) {
                a.returning = false;
                enter(a, State::Idle, 3 + 6 * unit(random));
            }
            break;
        case State::Chase:
            goal = glm::vec2(player);
            walking = distance > HOLD;
            break;
        case State::Attack:
        case State::Face:
            goal = glm::vec2(player);
            if (a.state == State::Attack && a.stateTime > a.stateLength)
                enter(a, State::Chase, 1e9);
            break;
        default: break;
        }

        // Turn toward the goal (or the player when facing them).
        glm::vec2 dir = goal - glm::vec2(a.position);
        if (glm::length(dir) > .05f) {
            float want = std::atan2(dir.y, dir.x);
            float turn = wrap(want - a.heading);
            float limit = TURN_RATE * (float)dt;
            a.heading = wrap(a.heading + std::clamp(turn, -limit, limit));
        }
        if (!walking || !canWalk)
            continue;
        float speed = a.state == State::Chase ? CHASE_SPEED : a.returning ? RETURN_SPEED : a.speed;
        float step = std::min(speed * (float)dt, glm::length(dir));
        glm::vec2 next = glm::vec2(a.position) + glm::vec2(std::cos(a.heading), std::sin(a.heading)) * step;
        float z = a.position.z;
        if (a.grounded) {
            float h;
            if (!floor(next.x, next.y, a.position.z - a.lift + MAX_STEP_UP, h) ||
                std::abs(h + a.lift - a.position.z) > MAX_STEP_UP + .2f) {
                // A wall, cliff or the edge of the extracted world: give up on this goal.
                if (a.state == State::Wander)
                    enter(a, State::Idle, 2 + 3 * unit(random));
                continue;
            }
            z = h + a.lift;
        }
        a.position = glm::vec3(next, z);
        a.walked += step;
    }
}

void Actors::pose(size_t i, std::vector<glm::mat4> &out) const {
    const Actor &a = actors[i];
    const auto &c = source->all()[a.recording];
    const Clip *clip = &a.idle;
    double key = 0;
    bool walking = (a.state == State::Wander || a.state == State::Chase) && !a.walk.empty();
    if (a.state == State::Attack && !a.move.empty()) {
        clip = &a.move;
        key = std::min(a.animTime, a.move.duration);
    } else if (walking) {
        // Step through the walk by distance covered, so feet do not slide.
        clip = &a.walk;
        key = std::fmod(a.walked, std::max(a.walk.distance, 1e-3));
    } else
        key = std::fmod(a.animTime, std::max(a.idle.duration + 1.0 / 30, 1e-3));
    size_t j = at(walking ? clip->covered : clip->times, key);
    out = bodyFrame(c, clip->frames[std::min(j, clip->frames.size() - 1)]);
    glm::vec3 at = a.position;
    glm::mat4 place = glm::rotate(glm::translate(glm::mat4(1), at), a.heading - a.forward, glm::vec3(0, 0, 1));
    for (auto &m : out)
        if (m[3][3] != 0)
            m = place * m;
}

std::string Actors::summary() const {
    std::map<std::string, int> kinds;
    int walkers = 0, attackers = 0;
    for (auto &a : actors) {
        static const char *names[] = {"monsters", "water creatures", "townspeople", "companions", "fixed"};
        kinds[names[(int)a.kind]]++;
        walkers += !a.walk.empty();
        attackers += !a.move.empty();
    }
    std::string s = std::to_string(actors.size()) + " actors (";
    bool first = true;
    for (auto &[k, n] : kinds) {
        s += (first ? "" : ", ") + std::to_string(n) + " " + k;
        first = false;
    }
    return s + "); " + std::to_string(walkers) + " have a recorded walk, " + std::to_string(attackers) +
           " a recorded attack or action";
}

bool Actors::hurt(size_t i, float amount) {
    Actor &a = actors[i];
    if (!hittable(i))
        return false;
    a.hp -= amount;
    a.sinceHurt = 0;
    if (a.state != State::Chase && a.state != State::Attack && !a.walk.empty())
        enter(a, State::Chase, 1e9); // Struck monsters come for the player.
    if (a.hp > 0)
        return false;
    a.hp = 0;
    a.dead = true;
    a.deathTime = 0;
    return true;
}

int64_t Actors::creatureId(size_t i) const {
    const Actor &a = actors[i];
    return source && a.recording < source->all().size() ? (int64_t)source->all()[a.recording].id : 0;
}

void Actors::alreadyDead(size_t i) {
    Actor &a = actors[i];
    a.dead = true, a.hp = 0;
    a.deathTime = 1e9; // long gone: no burst, no body
}

bool Actors::stun(size_t i, double seconds) {
    Actor &a = actors[i];
    if (a.dead || a.stunned > 0 || a.sinceStun < 3)
        return false;
    a.stunned = seconds;
    return true;
}

void Actors::poison(size_t i, float value) {
    if (!actors[i].dead)
        actors[i].effects.add(combat::POISON, value, 3);
}

void Actors::strength(size_t i, float factor) {
    Actor &a = actors[i];
    float fraction = a.maxHp > 0 ? a.hp / a.maxHp : 1; // keep how hurt it is
    a.maxHp = a.baseHp * factor;
    a.attack = a.baseAttack * factor;
    a.spell = a.baseSpell * factor;
    a.armor = a.baseArmor * factor;
    a.resistance = a.baseResistance * factor;
    a.hp = a.dead ? a.hp : a.maxHp * fraction;
}

Actors::Sync Actors::sync(size_t i) const {
    const Actor &a = actors[i];
    Sync s;
    s.position = a.position, s.heading = a.heading, s.state = (int)a.state;
    s.stateTime = (float)a.stateTime, s.animTime = (float)a.animTime, s.walked = (float)a.walked;
    s.hp = a.hp, s.dead = a.dead, s.deathTime = (float)a.deathTime;
    return s;
}

void Actors::apply(size_t i, const Sync &s) {
    Actor &a = actors[i];
    if (!a.netDriven)
        a.position = s.position; // first word from the host: jump there
    a.netDriven = true;
    a.netTarget = s.position;
    a.heading = s.heading;
    State state = (State)std::clamp(s.state, 0, (int)State::Attack);
    if (state != a.state)
        a.animTime = s.animTime; // a new move starts in step with the host
    a.state = state;
    a.stateTime = s.stateTime;
    if (a.hp > s.hp + .01f)
        a.sinceHurt = 0; // the plate's flash follows hits seen on the host
    a.hp = s.hp;
    a.dead = s.dead;
    a.deathTime = s.deathTime;
}

void Actors::advance(double dt) {
    for (auto &a : actors) {
        if (!a.netDriven)
            continue;
        // Between the host's updates: ease toward where it is, keep the clips running.
        glm::vec3 before = a.position;
        a.position += (a.netTarget - a.position) * std::min(1.f, (float)dt * 10);
        a.walked += glm::length(glm::vec2(a.position - before));
        a.animTime += dt;
        a.stateTime += dt;
        a.sinceHurt += dt;
        if (a.dead)
            a.deathTime += dt;
    }
}
