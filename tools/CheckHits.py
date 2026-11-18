"""Compare a Steam hit probe's immediate HP samples with the ordinary defense rules."""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
import struct


ABILITY_MULTIPLIERS = {
    6: .5, 22: .1, 27: .5, 31: .4, 32: .6, 33: .4, 34: .6, 38: 2, 44: 2, 47: .75,
    53: 3, 56: .25, 58: 2, 59: 2, 69: 2, 73: .2, 105: .75, 106: .75, 160: 2,
}


def f32(value):
    return struct.unpack("=f", struct.pack("=f", value))[0]


@dataclass(frozen=True)
class Hit:
    time: int
    attacker: int
    target: int
    ability: int
    category: int
    critical: bool
    roll: tuple[str, bool] | None
    damage: float
    defense: float
    combo: int
    threshold: int
    before: float
    after: float
    applied: bool

    def ordinary_hp(self, before, damage, defense):
        ratio = f32(f32(self.combo) / f32(self.threshold))
        reduction = f32(f32(defense) * f32(1 - min(1, ratio)))
        amount = max(0, f32(f32(damage) - reduction))
        return max(0, f32(f32(before) - amount))

    def consistent(self, observed, precision):
        # The hit probe writes four decimal places for each input. Bound both
        # ends before comparing to either a four- or two-decimal HP reading.
        half = HALF_DECIMAL
        low = self.ordinary_hp(self.before - half, self.damage + half, self.defense - half)
        high = self.ordinary_hp(self.before + half, self.damage - half, self.defense + half)
        return low <= observed + precision and high >= observed - precision


# The direct swing and close-spell dispatch in creature-abilities.md, section 4.
DIRECT_MOVES = {
    *range(2, 9), *range(10, 23), *range(31, 35), 53, 55, 58, 59,
    61, 62, 63, *range(66, 72), 73, *range(75, 80), 95, 96, 97,
    100, 103, 116, 139, 148, 158, 159,
}
HALF_DECIMAL = 0.00005  # logged to four places


def roll_check(head, source, damage):
    ability, category = int(head["ability"]), int(head["type"])
    if "ability" not in source or "charge" not in head:
        return None
    actor_action = int(source["ability"])
    charge = float(head["charge"])
    scale = ABILITY_MULTIPLIERS.get(ability, 1)
    stat = "attack"
    critical = bool(int(head["critical"]))

    if ability in DIRECT_MOVES and actor_action == ability:
        expected_category = 1 if ability in (31, 32) else 2 if ability in (33, 34) else 0
        if category != expected_category:
            return None
        label = "direct"
        if ability in (31, 32, 33, 34, 96, 97):
            stat = "spell"
            charge_scale, base = 0, 1
        else:
            charge_scale, base = 5, int(ability not in (12, 61, 63))
    elif (ability, category, actor_action) == (0, 0, 27):
        # The contact carries one fifth of the boomerang's released charge.
        # The actor's current action/power need not belong to that launch.
        flag = int(head.get("flag", -1))
        if flag not in (0, 1) or charge < 0 or (not flag and charge != 0):
            return None
        label = "boomerang_candidate"
        scale = .5
        charge_scale, base = (1, .1) if flag else (0, 1)
    else:
        return None

    if stat not in source or float(source[stat]) < 0:
        return None
    power = float(source[stat])

    def endpoint(variation, rounding):
        magnitude = max(0, abs(charge) + rounding)
        if label == "boomerang_candidate" and charge_scale:
            # The launch first stored binary32(release * 0.2f). Allow its
            # rounding as well as the four decimal places at contact.
            half_ulp = f32(magnitude) * 2**-24
            magnitude = max(0, magnitude + (half_ulp if rounding > 0 else -half_ulp)) / f32(.2)
        released = f32(magnitude)
        charge_factor = f32(f32(f32(released * released) * charge_scale) + f32(base))
        scaled_power = f32(f32(max(0, power + rounding)) * f32(scale))
        amount = f32(variation * f32(charge_factor * scaled_power))
        if ability == 22:  # Earth Shatter also applies the fixed skill curve.
            divisor = f32(1 + f32(21 / 50))
            curve = f32(1 - f32(1 / divisor))
            amount = f32(amount * f32(curve * .5 + .5))
        return f32(amount * (2 if critical else 1))

    lower = endpoint(.75, -HALF_DECIMAL)
    upper = endpoint(1.25, HALF_DECIMAL)
    return label, (lower <= damage + HALF_DECIMAL and upper >= damage - HALF_DECIMAL)


def fields(text):
    return dict(token.split("=", 1) for token in text.split() if "=" in token)


def parse_hit(line):
    head, source, target, hp = map(fields, line.split(" | "))
    ability, category = int(head["ability"]), int(head["type"])
    damage = float(head["damage"])
    critical = bool(int(head["critical"]))
    before, after = map(float, hp["hp"].split("->"))
    return Hit(int(head["t"]), int(source["id"]), int(target["id"]), ability, category, critical,
               roll_check(head, source, damage), damage, float(target["resistance"] if category else target["armor"]),
               int(source["combo"]), int(source["maxcombo"]), before, after, bool(int(hp["applied"])))


def read_log(path, parse):
    rows = []
    skipped = 0
    with path.open(encoding="utf-8-sig") as stream:
        for line in stream:
            if not line.strip():
                continue
            try:
                rows.append(parse(line))
            except (ValueError, KeyError):
                skipped += 1
    return rows, skipped


def hp_sample(line):
    record = fields(line.split("|", 1)[0])
    before, after = map(float, record["hp"].split("->"))
    return int(record["id"]), (int(record["t"]), before, after)


def compare(folder, since=0):
    hits, skipped = read_log(folder / "hits.txt", parse_hit)
    samples = defaultdict(list)
    times = defaultdict(list)
    for hit in hits:
        times[hit.target].append(hit.time)
    for target_times in times.values():
        target_times.sort()
    supplemental = folder / "combat-log.txt"
    if supplemental.is_file():
        observations, ignored = read_log(supplemental, hp_sample)
        for target, sample in observations:
            samples[target].append(sample)
        for target_samples in samples.values():
            target_samples.sort()
        if ignored:
            print(f"Supplemental rows without usable HP samples: {ignored}")

    selected = [hit for hit in hits if hit.time >= since]
    counts = Counter()
    for hit in selected:
        if hit.roll is None:
            counts["unclassified_roll"] += 1
        else:
            label, compatible = hit.roll
            counts[label + ("_within" if compatible else "_outside")] += 1
        if not hit.applied:
            counts["declined"] += 1
        elif hit.after > hit.before:
            counts["increased"] += 1
        elif hit.after < hit.before:
            if hit.threshold <= 0:
                counts["missing_threshold"] += 1
            elif hit.consistent(hit.after, .00005):
                counts["matched"] += 1
            else:
                counts["unexplained"] += 1
                print(f"DEF? t={hit.time} ability {hit.ability}, type {hit.category}, HP {hit.before} -> {hit.after}")
        else:
            counts["unchanged"] += 1
            nearby = samples.get(hit.target, [])
            first = bisect_left(nearby, (hit.time,))
            end = bisect_left(nearby, (hit.time + 400,))
            if first == end:
                continue
            when, before, after = nearby[first]
            # A later sample can also fall in another hit's 400 ms window,
            # including a hit before the user's time filter. Never reuse it.
            order = times[hit.target]
            competing = bisect_left(order, when + 1) - bisect_left(order, when - 399)
            if end != first + 1 or competing != 1:
                counts["ambiguous"] += 1
            elif abs(hit.before - before) <= .00505 and hit.threshold > 0 and hit.consistent(after, .005):
                counts["nearby_consistent"] += 1
            else:
                counts["nearby_other"] += 1

    print(f"{len(selected)} hits. Direct moves: {counts['direct_within']} within the roll band, "
          f"{counts['direct_outside']} outside.")
    print(f"Possible boomerang contacts: {counts['boomerang_candidate_within']} compatible with the launch band, "
          f"{counts['boomerang_candidate_outside']} outside (assuming the recorded attack power at launch). "
          f"Other unclassified producers: {counts['unclassified_roll']}")
    print(f"Immediate HP losses: {counts['matched']} consistent with ordinary defense, {counts['unexplained']} unexplained, "
          f"{counts['missing_threshold']} without a positive combo threshold")
    print(f"Other calls: {counts['declined']} returned false, {counts['unchanged']} successful with unchanged HP, "
          f"{counts['increased']} successful with increased HP; {skipped} unreadable hit rows")
    if supplemental.is_file():
        print(f"Later HP samples within 400 ms (timing evidence only): {counts['nearby_consistent']} compatible, "
              f"{counts['nearby_other']} other, {counts['ambiguous']} ambiguous hit windows")
    else:
        print("No supplemental combat-log.txt; compared the immediate HP measurements in hits.txt.")
    print("abilities:", sorted({(h.ability, h.category) for h in selected}), "crits:", sum(h.critical for h in selected),
          "player hits:", sum(h.attacker == 1 for h in selected))
    return counts


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", type=Path)
    parser.add_argument("since", type=int, nargs="?", default=0)
    args = parser.parse_args()
    compare(args.game / "Extract", args.since)
