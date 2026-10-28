#pragma once
// Each class's moves, per specialization, as the game plays them: the ability id
// the game used for each control. The ids come from the recordings in
// Extract/characters: each take pressed the controls in a fixed order, and
// every ability is matched to its key by when it started (docs/saves.md).
// 0 = that move was not recorded for this specialization.
struct ClassMoves {
    int attack[2] = {0, 0}; // basic attack (left mouse); two ids alternate as a combo
    int special = 0;        // special attack (right mouse)
    int releaseSpecial = 0; // release of a held charge (bow salvo)
    int skill = 0;          // class skill (Shift)
    int tap = 0;            // tap P: smash, retreat, teleport...
    int ultimate = 0;       // R
    int dodge = 0;          // dodge roll (middle mouse)
    float reach = 2.6f;     // blocks: a melee swing, or how far arrows and spells fly
    // Mana (MP, 0..1) and stamina (0..1), as the game's skill list and the extractor's
    // fight log (Extract/combat-log-*.txt) show them. A cost of -1 spends all MP
    // and needs some; a skill is refused while its cost cannot be paid.
    float specialMp = 0, tapMp = 0;            // water special .3 (logged), teleport "100 MANA"
    float tapStamina = 0, skillStamina = .25f; // smash / retreat / poison vial "100 STAMINA"
    float dodgeStamina = .25f;                 // "25 STAMINA"
    bool mpRegenerates = false;                // mage: +0.1 MP per second (logged)
    float mpPerHit = .2f;                      // others: +0.2 per landed basic hit, -0.05/s (logged, ranger)
};

// cls: the game's class ids (1 warrior, 2 ranger, 3 mage, 4 rogue); spec 0 or 1.
inline ClassMoves classMoves(int cls, int spec) {
    ClassMoves m;
    switch (cls) {
    case 1: // warrior: berserker / guardian
        m.attack[0] = m.attack[1] = 58;
        m.special = 60; // crush
        m.skill = 95;   // cyclone
        if (spec == 0)
            m.tap = 55, m.tapStamina = 1; // smash
        else
            m.ultimate = 149;
        break;
    case 2: // ranger: sniper / scout
        m.attack[0] = m.attack[1] = 23;
        m.special = 26; // salvo
        m.releaseSpecial = 56;
        m.specialMp = -1; // the salvo spends all MP (logged)
        if (spec == 0)
            m.skill = 80; // sneak
        else
            m.tap = 22, m.tapStamina = 1; // retreat
        m.reach = 18;
        break;
    case 3: // mage: fire / water
        if (spec == 0) {
            m.attack[0] = m.attack[1] = 31;
            m.special = 32; // fire vortex
            m.ultimate = 47;
        } else {
            m.attack[0] = m.attack[1] = 33;
            m.special = 34;
            m.ultimate = 144;
        }
        m.skill = 83; // float
        m.tap = 51, m.tapMp = 1; // teleport
        m.specialMp = .3f;
        m.mpRegenerates = true, m.mpPerHit = 0;
        m.reach = 12;
        break;
    case 4: // rogue: assassin / ninja
        m.attack[0] = 20, m.attack[1] = 19;
        if (spec == 0)
            m.skill = 80; // sneak
        else
            m.special = 18, m.specialMp = -1; // intercept: "ALL MP"
        m.dodge = 160;
        break;
    }
    return m;
}
