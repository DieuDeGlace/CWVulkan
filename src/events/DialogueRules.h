#pragma once
// Villager dialogue for town-referred events: five lines per situation, one
// chosen at random each time (never the same line twice in a row from the
// same villager). Offer and reminder lines are specific to the event kind and
// take the distance to the objective in blocks (%.0f). Every line stays well
// under the native speech bubble length used by the existing messages.
#include "EventRules.h"
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace encounters::dialogue {
constexpr std::size_t Lines = 5;
enum class Topic : unsigned { Offer, Reminder, Reward, Thanks, Over, Count };
using Pool = const wchar_t *const[Lines];

inline const Pool &offers(Kind kind) {
    static Pool battle = {
        L"Adventurers are fighting for their lives about %.0f blocks from here. Lend them your "
        L"blade! The spot is marked on your map. Come back when it's over for your reward.",
        L"Did you hear that? Steel on steel, %.0f blocks out. A band of adventurers is "
        L"outnumbered. Hurry, it's on your map, and I'll have a reward when you return.",
        L"Travelers walked straight into trouble about %.0f blocks away. They won't last without "
        L"help. Check your map, then report back to me.",
        L"A skirmish is raging %.0f blocks from town. If those adventurers fall, the monsters "
        L"come here next. Go help them, then find me for your reward.",
        L"You look like you can handle yourself. A fight broke out %.0f blocks away and our "
        L"friends are losing. It's marked on your map. Return when the dust settles.",
    };
    static Pool rescue = {
        L"One of ours was dragged off and locked in a cage about %.0f blocks from here. Please "
        L"set them free! It's marked on your map. Come back and I'll make it worth your while.",
        L"A traveler went missing on the road. The tracks lead %.0f blocks out, to a camp with a "
        L"cage. Free them and return to me for a reward. It's on your map.",
        L"Someone has been taken captive %.0f blocks from here, and their guards won't open that "
        L"cage for anyone. Check your map, then come back to me afterward.",
        L"Please help! My friend is trapped about %.0f blocks away. I'm no fighter, but you are. "
        L"The spot is on your map. I'll reward you once they're safe.",
        L"We heard cries for help %.0f blocks from town. Someone is caged out there. Break them "
        L"free and return here. The place is marked on your map.",
    };
    static Pool ambush = {
        L"A pack of monsters is gathering about %.0f blocks from here. Strike before they reach "
        L"the town! They're marked on your map. Come back once they're gone.",
        L"Our scouts spotted an ambush waiting on the road, %.0f blocks out. Clear them out, "
        L"adventurer. It's on your map, and there's a reward when you return.",
        L"Something has been stalking our herders %.0f blocks from town. Hunt down the whole "
        L"group. They're marked on your map; report back here.",
        L"Trouble's brewing %.0f blocks away. A band of enemies is camped there, waiting. Drive "
        L"them off and come back for your reward. Check your map.",
        L"If nobody deals with that group %.0f blocks from here, we'll be next. Go now, they're "
        L"on your map. I'll be waiting with your reward.",
    };
    return kind == Kind::Rescue ? rescue : kind == Kind::Battle ? battle : ambush;
}
inline const Pool &reminders(Kind kind) {
    static Pool battle = {
        L"The battle is still raging about %.0f blocks away. Hurry!",
        L"Those adventurers won't hold out forever. It's %.0f blocks from here, on your map.",
        L"Still here? The fight is %.0f blocks from town. Go!",
        L"Check your map. The battle is about %.0f blocks out.",
        L"Come back once the fighting is done. It's %.0f blocks away.",
    };
    static Pool rescue = {
        L"They're still locked in that cage, about %.0f blocks away. Please hurry.",
        L"Every moment counts. The captive is %.0f blocks from here, on your map.",
        L"Have you freed them yet? The cage is %.0f blocks from town.",
        L"Check your map. The prisoner is about %.0f blocks out.",
        L"Bring them home safe. It's %.0f blocks from here.",
    };
    static Pool ambush = {
        L"Those monsters are still out there, about %.0f blocks away.",
        L"The group hasn't moved. It's %.0f blocks from here, on your map.",
        L"Still waiting on you, adventurer. They're %.0f blocks from town.",
        L"Check your map. The enemies are about %.0f blocks out.",
        L"Clear out every last one of them. It's %.0f blocks away.",
    };
    return kind == Kind::Rescue ? rescue : kind == Kind::Battle ? battle : ambush;
}
inline const Pool &shared(Topic topic) {
    static Pool reward = {
        L"Thank you. This equipment is your reward.",
        L"You did it! Take this, you've earned it.",
        L"The whole town owes you. Please accept this gear.",
        L"Word travels fast around here. This is for your trouble.",
        L"I knew you could do it. This should serve you well.",
    };
    static Pool thanks = {
        L"Thank you for helping. Safe travels!",
        L"The roads feel safer already. Take care out there.",
        L"You're always welcome in our town.",
        L"Come back soon. There's always trouble somewhere.",
        L"Thanks again, adventurer. May your blade stay sharp.",
    };
    static Pool over = {
        L"That encounter is over. We will have more news another time.",
        L"It's too late for that one, I'm afraid. Check back with us later.",
        L"That trouble has passed. We'll let you know if more comes up.",
        L"Nothing more to be done there. Come back another day.",
        L"That's behind us now. Ask again later; there's always something.",
    };
    return topic == Topic::Reward ? reward : topic == Topic::Thanks ? thanks : over;
}
inline const Pool &pool(Topic topic, Kind kind) {
    return topic == Topic::Offer      ? offers(kind)
           : topic == Topic::Reminder ? reminders(kind)
                                      : shared(topic);
}

// Picks a line index from a random value, avoiding the speaker's last line
// for the same topic and kind.
class Picker {
    std::unordered_map<uint64_t, std::size_t> last;

  public:
    std::size_t pick(uint64_t speaker, Topic topic, Kind kind, uint64_t random) {
        uint64_t key = speaker * 31 + unsigned(topic) * 7 + unsigned(kind);
        auto previous = last.find(key);
        std::size_t index = random % Lines;
        if (previous != last.end() && index == previous->second)
            index = (index + 1 + (random / Lines) % (Lines - 1)) % Lines;
        if (last.size() > 4096)
            last.clear();
        last[key] = index;
        return index;
    }
};
} // namespace encounters::dialogue
