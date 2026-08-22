#!/usr/bin/env python3
"""Prelude creature table reader - schedules, live positions, inventories.

The SaveBinCreatures format is written both to creatures.bin (the shipped
master list) and into every .gam savegame, so the same walk reads both.
Layout is derived from SaveBinCreatures / Thing::SaveBin / Locator::Save /
GameItem::Save.

    creatures.py creatures.bin where Vittoria    schedule only
    creatures.py save3.gam where Vittoria        schedule + where she is now
    creatures.py save3.gam near 1940 100         who is standing around there
    creatures.py save3.gam who-has "Ironwood Dagger"
    creatures.py save3.gam move 2965 496 121     put the party on that tile

Two traps, both of which make a creature look like it is at the origin:

  * The Position in Thing::SaveBin's fixed header is dead. Thing::GetPosition
    returns GetData(INDEX_POSITION), so the live position is the POSITION
    *field*, which is what this reads.
  * creatures.bin holds templates - every POSITION in it really is (0,0,0).
    Creature::PlaceByLocator puts them on the map from their locators, so
    creatures.bin answers "where is she meant to be" and a savegame answers
    "where is she".
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from saveedit import DATA_VECTOR, DATA_STRING, DATA_FLOAT, _skip_item, _u32, Save

MAX_SKILLS, MAX_SPELLS, MAX_EQUIP = 24, 48, 16   # MAX_NUM_SKILLS, MAX_SPELLS, MAX_EQUIPMENT
LOCATOR_SIZE = 21                                # RECT + Start/End/Angle/State/AreaNum bytes
SAVE_TABLE_AT = 204                              # 64 id + hour + totaltime + 2 matrices + viewdim
STATES = ["normal", "facing", "sitting", "lying"]   # LOCATOR_STATE_T


class Creatures:
    """Every creature in a creatures.bin or a savegame, with its locators."""

    def __init__(self, path):
        self.path = path
        self.data = open(path, "rb").read()
        self.base = self._find_table()
        self.all = list(self._walk())

    def _find_table(self):
        """creatures.bin starts at 0, a savegame after its header."""
        for base in (0, SAVE_TABLE_AT):
            try:
                n_fields = struct.unpack_from("<i", self.data, base + 4)[0]
                if not 0 < n_fields < 512:
                    continue
                names_at = base + 8 + 4 * n_fields
                names = self.data[names_at:self.data.index(b"\r", names_at)]
                if len(names.split(b"@")) == n_fields:
                    return base
            except (struct.error, ValueError):
                pass
        raise ValueError("no creature table in %r" % self.path)

    def _walk(self):
        d = self.data
        p = self.base
        n_creatures, n_fields = struct.unpack_from("<2i", d, p)
        p += 8
        types = struct.unpack_from("<%di" % n_fields, d, p)
        p += 4 * n_fields
        names_end = d.index(b"\r", p)
        self.names = [n.decode("latin1") for n in d[p:names_end].split(b"@")]
        p = names_end + 1

        for _ in range(n_creatures):
            p += 40                       # Thing::SaveBin fixed header - see module docstring
            fields = {}
            for name, t in zip(self.names, types):
                if t == DATA_STRING:
                    n = struct.unpack_from("<i", d, p)[0]
                    fields[name] = d[p + 4:p + 4 + n].decode("latin1")
                    p += 4 + n
                elif t == DATA_VECTOR:
                    fields[name] = struct.unpack_from("<3f", d, p)
                    p += 12
                elif t == DATA_FLOAT:
                    fields[name] = struct.unpack_from("<f", d, p)[0]
                    p += 4
                else:
                    fields[name] = struct.unpack_from("<i", d, p)[0]
                    p += 4

            items = []
            n_items = struct.unpack_from("<i", d, p)[0]
            p += 4
            for _ in range(n_items):
                items.append(self._item(p))
                p = _skip_item(d, p)

            p += 4                        # Created
            n_loc = struct.unpack_from("<i", d, p)[0]
            p += 8                        # NumLocators + CurLocator
            locators = []
            for _ in range(n_loc):
                left, top, right, bottom = struct.unpack_from("<4i", d, p)
                start, end, angle, state, area = struct.unpack_from("<5B", d, p + 16)
                p += LOCATOR_SIZE
                locators.append(dict(rect=(left, top, right, bottom), start=start,
                                     end=end, angle=angle, state=state, area=area))

            p += 4 * MAX_SKILLS + MAX_SPELLS + 1    # SkillImproved, KnownSpells, ReadySpell
            equipped = []
            for _ in range(MAX_EQUIP):
                has = struct.unpack_from("<i", d, p)[0]
                p += 4
                if has:
                    equipped.append(self._item(p))
                    p = _skip_item(d, p)

            yield dict(name=fields.get("NAME", ""), id=fields.get("ID"),
                       pos=fields.get("POSITION", (0.0, 0.0, 0.0)),
                       fields=fields, items=items, equipped=equipped, locators=locators)
        self.end = p

    def _item(self, p):
        """GameItem::Save packs id and quantity into one int."""
        compressed = struct.unpack_from("<i", self.data, p + 4)[0]
        return compressed % 1000, compressed // 1000

    def find(self, name):
        return [c for c in self.all if c["name"].lower() == name.lower()]


def item_names(path):
    """{id: name} from the items.txt sitting beside the file we were given."""
    items = os.path.join(os.path.dirname(os.path.abspath(path)), "items.txt")
    if not os.path.exists(items):
        return {}
    out = {}
    for line in open(items, "r", encoding="latin1").read().splitlines()[1:]:
        cols = line.split("@")
        if len(cols) > 1 and cols[1].isdigit():
            out[int(cols[1])] = cols[0]
    return out


def show(creature, names):
    live = creature["pos"]
    where = " at (%.2f, %.2f, %.2f)" % live if any(live) else " (template - no live position)"
    print("%s (id %s)%s" % (creature["name"], creature["id"], where))
    for loc in creature["locators"]:
        left, top, right, bottom = loc["rect"]
        spot = "(%d,%d)" % (left, top) if right - left <= 1 and bottom - top <= 1 \
            else "(%d,%d)-(%d,%d)" % (left, top, right, bottom)
        print("    %02d:00-%02d:00  area %-2d %-22s %s"
              % (loc["start"], loc["end"], loc["area"], spot, STATES[loc["state"]]))
    for iid, qty in sorted(set(creature["items"])):
        print("    stocks  %-28s x%d" % (names.get(iid, "item %d" % iid), qty))
    for iid, _ in sorted(set(creature["equipped"])):
        print("    wields  %s" % names.get(iid, "item %d" % iid))


def main(argv):
    if len(argv) < 2:
        return print(__doc__.strip())
    path, cmd, args = argv[0], argv[1], argv[2:]
    names = item_names(path)

    if cmd == "move":
        x, y, z = int(args[0]), int(args[1]), float(args[2])
        save = Save(path)
        for n, member in enumerate(save.members):
            # Party::Teleport stands them on tile centres, two abreast.
            struct.pack_into("<3f", save.data, member["POSITION"][1],
                             x + n % 2 + 0.5, y + n // 2 + 0.5, z)
        save.write()
        check = Save(path)
        for member in check.members:
            print("%-12s %s" % (check.get(member, "NAME"),
                                tuple(round(v, 2) for v in check.get(member, "POSITION"))))
        return

    cre = Creatures(path)
    if cmd == "where":
        for want in args:
            found = cre.find(want)
            if not found:
                print("no creature named %r" % want)
            for creature in found:
                show(creature, names)
    elif cmd == "near":
        x, y = float(args[0]), float(args[1])
        radius = float(args[2]) if len(args) > 2 else 8.0
        hits = [c for c in cre.all if any(c["pos"])
                and abs(c["pos"][0] - x) <= radius and abs(c["pos"][1] - y) <= radius]
        if not hits:
            print("nobody within %g tiles (a creatures.bin has no live positions)" % radius)
        for creature in sorted(hits, key=lambda c: c["pos"][1]):
            print("%-22s (%.2f, %.2f, %.2f)" % ((creature["name"],) + creature["pos"]))
    elif cmd == "who-has":
        want = args[0]
        ids = [int(want)] if want.isdigit() else \
            [i for i, n in names.items() if n.lower() == want.lower()]
        if not ids:
            return sys.exit("no item named %r in items.txt" % want)
        for creature in cre.all:
            stock = sum(q for i, q in creature["items"] if i in ids)
            worn = [i for i, q in creature["equipped"] if i in ids]
            if stock:
                print("%-22s x%-3d stock" % (creature["name"], stock))
            elif worn:
                print("%-22s     equipped" % creature["name"])
    else:
        sys.exit("unknown command %r" % cmd)


def _selftest(gamedir):
    """The walk is only right if it lands exactly where the next section starts."""
    cre = Creatures(os.path.join(gamedir, "creatures.bin"))
    assert cre.end == len(cre.data), "creatures.bin: ended at %d of %d" % (cre.end, len(cre.data))
    assert cre.find("Vittoria"), "no Vittoria in creatures.bin"
    assert not any(cre.find("Vittoria")[0]["pos"]), "creatures.bin should hold templates"

    saves = [f for f in sorted(os.listdir(gamedir)) if f.endswith(".gam")]
    for name in saves:
        cre = Creatures(os.path.join(gamedir, name))
        # SaveGame writes its own offset before the party section, so the
        # creature walk has to stop on exactly that self-referential marker.
        assert _u32(cre.data, cre.end) == cre.end, \
            "%s: creature walk ended at %d, not on the party marker" % (name, cre.end)
    print("ok: creatures.bin + %d savegames" % len(saves))


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        _selftest(sys.argv[sys.argv.index("--selftest") + 1])
    else:
        main(sys.argv[1:])
