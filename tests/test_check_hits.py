import io
from contextlib import redirect_stdout
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.CheckHits import compare, roll_check


def hit(time, before, after, *, category=0, combo=-10, applied=1):
    return (f"t={time} damage=12.0000 critical=0 flag=0 charge=0.0000 type={category} ability=0 | "
            f"attacker id=-71 race=1 ability=0 combo={combo} maxcombo=20 attack=12.0000 spell=12.0000 | "
            f"target id=-4 armor=8.0000 resistance=4.0000 | hp={before:.4f}->{after:.4f} applied={applied}\n")


class HitComparisonTests(unittest.TestCase):
    def compare(self, hits, samples=None, since=0):
        with TemporaryDirectory() as temp:
            folder = Path(temp)
            (folder / "hits.txt").write_text("".join(hits), encoding="utf-8")
            if samples is not None:
                (folder / "combat-log.txt").write_text(samples, encoding="utf-8")
            with redirect_stdout(io.StringIO()):
                return compare(folder, since)

    def test_signed_combo_defense_and_overkill_without_supplemental_log(self):
        counts = self.compare([hit(10, 20, 20), hit(20, 20, 14, category=2),
                               hit(30, 5, 0, combo=30), hit(40, 20, 16, category=2),
                               hit(50, 20, 20, applied=0)])
        self.assertEqual((counts["matched"], counts["unexplained"], counts["unchanged"], counts["declined"]),
                         (2, 1, 1, 1))

    def test_later_samples_remain_separate_from_direct_losses(self):
        samples = "t=120 target=player id=-4 hp=20.00->14.00 | player ability=0\n"
        counts = self.compare([hit(100, 20, 20, category=2)], samples)
        self.assertEqual((counts["matched"], counts["unchanged"], counts["nearby_consistent"]), (0, 1, 1))
        counts = self.compare([hit(99, 20, 20), hit(100, 20, 20, category=2)], samples, since=100)
        self.assertEqual((counts["ambiguous"], counts["nearby_consistent"]), (1, 0))

    def test_second_sample_and_exclusive_time_boundary(self):
        row = " target=player id=-4 hp=20.00->14.00 | player ability=0\n"
        self.assertEqual(self.compare([hit(100, 20, 20)], "t=120" + row + "t=499" + row)["ambiguous"], 1)
        self.assertEqual(self.compare([hit(100, 20, 20, category=2)], "t=500" + row)["nearby_consistent"], 0)

    def test_direct_move_uses_its_own_power_and_charge_rules(self):
        source = {"ability": "96", "spell": "20", "attack": "3"}
        head = {"ability": "96", "type": "0", "critical": "1", "charge": "1", "flag": "1"}
        self.assertEqual(roll_check(head, source, 50), ("direct", True))
        self.assertEqual(roll_check(head, source, 51), ("direct", False))
        source["ability"] = head["ability"] = "12"
        head["charge"] = "0"
        self.assertEqual(roll_check(head, source, 0), ("direct", True))
        self.assertEqual(roll_check(head, source, 1), ("direct", False))
        head["type"] = "2"
        self.assertIsNone(roll_check(head, source, 0))

    def test_projectile_damage_is_only_a_candidate_at_contact(self):
        source = {"ability": "27", "attack": "20", "chargefield": "0.0000"}
        head = {"ability": "0", "type": "0", "critical": "0", "charge": "0.1000", "flag": "1"}
        # At launch: half of 20 times (0.1 + 0.5 squared), before variation.
        self.assertEqual(roll_check(head, source, 3.5), ("boomerang_candidate", True))
        self.assertEqual(roll_check(head, source, 9), ("boomerang_candidate", False))
        head["flag"] = "0"
        self.assertIsNone(roll_check(head, source, 10))
        head["charge"] = "0"
        self.assertEqual(roll_check(head, source, 10), ("boomerang_candidate", True))
        source["ability"] = "0"
        self.assertIsNone(roll_check(head, source, 10))

    def test_empty_selected_range(self):
        self.assertFalse(self.compare([hit(10, 20, 14)], since=11))
