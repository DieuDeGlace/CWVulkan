"""Checks Extract/hits.txt (ZoneExtractor hit probe) against combat-log HP changes and the decoded rules."""
import re, sys
g = sys.argv[1]; since = int(sys.argv[2]) if len(sys.argv) > 2 else 0
MULT = {6:.5,22:.1,27:.5,31:.4,32:.6,33:.4,34:.6,38:2,44:2,47:.75,53:3,56:.25,58:2,59:2,69:2,73:.2,105:.75,106:.75,160:2}
hits = []
for l in open(g + '/Extract/hits.txt'):
    m = re.search(r't=(\d+) damage=([\d.]+) critical=(\d).*?charge=([\d.]+) type=(\d+) ability=(\d+).*?attacker id=(\d+).*?combo=(-?\d+) maxcombo=(\d+) attack=([\d.]+) spell=([\d.]+) chargefield=([\d.]+).*?target id=(\d+).*?armor=([\d.]+) resistance=([\d.]+) \| hp=([\d.]+)->([\d.]+).*applied=(\d)', l)
    if m:
        v = m.groups()
        if int(v[0]) >= since: hits.append(dict(t=int(v[0]), dmg=float(v[1]), crit=int(v[2]), charge=float(v[3]), type=int(v[4]), ab=int(v[5]), att=v[6], combo=int(v[7]), maxc=int(v[8]), atk=float(v[9]), spell=float(v[10]), tgt=v[12], armor=float(v[13]), res=float(v[14]), h0=float(v[15]), h1=float(v[16]), applied=int(v[17])))
hp = []
for l in open(g + '/Extract/combat-log.txt'):
    m = re.match(r't=(\d+) target=\w+ id=(\d+).*?hp=([\d.]+)->([\d.]+)', l)
    if m and int(m.group(1)) >= since: hp.append((int(m.group(1)), m.group(2), float(m.group(3)) - float(m.group(4))))
roll_bad = def_bad = def_ok = 0; rolls = []
for h in hits:
    power = h['spell'] if h['type'] in (1, 2) else h['atk']
    base = MULT.get(h['ab'], 1) * power * (1 + 5 * h['charge'] ** 2) * (2 if h['crit'] else 1)
    r = h['dmg'] / base if base else 0; rolls.append(r)
    if not .7499 <= r <= 1.2501: roll_bad += 1; print('ROLL?', h['ab'], h['type'], h['crit'], round(r, 4), h['att'][-4:], h['tgt'][-4:])
    if not h['applied']: continue
    d = h['res'] if h['type'] else h['armor']
    pred = max(0, h['dmg'] - d * (1 - min(1, max(0, h['combo']) / h['maxc'])))
    lost = h['h0'] - h['h1'] if h['h0'] != h['h1'] else next((x[2] for x in hp if x[1] == h['tgt'] and 0 <= x[0] - h['t'] < 400), None)
    if lost is None: continue
    if abs(lost - pred) < .02 or (lost > 0 and h['h1'] == 0 and pred >= h['h0'] - .02): def_ok += 1
    else: def_bad += 1; print('DEF?', h['ab'], 'type', h['type'], 'crit', h['crit'], 'att', h['att'][-4:], 'tgt', h['tgt'][-4:], 'dmg', round(h['dmg'], 3), 'pred', round(pred, 3), 'lost', round(lost, 3))
print(f'{len(hits)} hits; roll outside 0.75-1.25: {roll_bad}; roll range {min(rolls):.4f}-{max(rolls):.4f}; defense matched {def_ok}, mismatched {def_bad}')
print('abilities:', sorted({(h["ab"], h["type"]) for h in hits}), 'crits:', sum(h['crit'] for h in hits), 'player hits:', sum(h['att'] == '1' for h in hits))
