#!/usr/bin/env python3
"""Prelude savegame editor - party member stats.

Layout is derived from World::SaveGame / Party::Save / Thing::SaveBin.
The party section is found via the self-referential uint32 the game writes
before each section (at file offset P it stores the value P).

Run with no args for the GUI, or:
    saveedit.py save1.gam                 list party stats
    saveedit.py save1.gam Argus STRENGTH 20
"""
import struct, sys, os, shutil

MAX_PARTY, MAX_SKILLS, MAX_SPELLS, MAX_EQUIP = 6, 24, 48, 16
DATA_INT, DATA_FLOAT, DATA_VECTOR, DATA_STRING = 1, 2, 3, 4


def _u32(d, p):
    return struct.unpack_from("<I", d, p)[0]


def _skip_item(d, p):
    """GameItem::Save is 68 fixed bytes plus nested contents."""
    n = struct.unpack_from("<i", d, p + 64)[0]
    p += 68
    for _ in range(n):
        p = _skip_item(d, p)
    return p


class Save:
    def __init__(self, path):
        self.path = path
        self.data = bytearray(open(path, "rb").read())
        d = self.data

        # header: 64 name + hour + totaltime + 2 matrices + viewdim, then
        # SaveBinCreatures' shared field table
        n_fields = struct.unpack_from("<i", d, 208)[0]
        self.types = struct.unpack_from("<%di" % n_fields, d, 212)
        names_at = 212 + 4 * n_fields
        names_end = d.index(b"\r", names_at)
        self.names = [n.decode("latin1") for n in d[names_at:names_end].split(b"@")]

        self.members = []
        p = self._find_party(names_end + 1)
        p += 4 + 4 * MAX_PARTY            # section marker + RestActions
        n_members = struct.unpack_from("<i", d, p)[0]
        p += 8                            # NumMembers + CurArea
        for _ in range(n_members):
            p = self._read_member(p)

    def _find_party(self, start):
        d = self.data
        for p in range(start, len(d) - 4):
            if _u32(d, p) == p and 1 <= struct.unpack_from("<i", d, p + 28)[0] <= MAX_PARTY:
                return p
        raise ValueError("party section not found - not a Prelude savegame?")

    def _read_member(self, p):
        """Record each field's file offset; values are patched in place."""
        d = self.data
        p += 40                           # Thing::SaveBin fixed header
        fields = {}
        for name, t in zip(self.names, self.types):
            fields[name] = (t, p)
            if t == DATA_STRING:
                p += 4 + struct.unpack_from("<i", d, p)[0]
            elif t == DATA_VECTOR:
                p += 12
            else:
                p += 4
        n_items = struct.unpack_from("<i", d, p)[0]
        p += 4
        for _ in range(n_items):
            p = _skip_item(d, p)
        p += 4 + 4 * MAX_SKILLS + MAX_SPELLS + 1   # Created, SkillImproved, spells
        for _ in range(MAX_EQUIP):
            has = struct.unpack_from("<i", d, p)[0]
            p += 4
            if has:
                p = _skip_item(d, p)
        p += 4 * MAX_SKILLS               # LastSkillImproves
        self.members.append(fields)
        return p

    def get(self, fields, name):
        t, p = fields[name]
        if t == DATA_INT:
            return struct.unpack_from("<i", self.data, p)[0]
        if t == DATA_FLOAT:
            return round(struct.unpack_from("<f", self.data, p)[0], 4)
        if t == DATA_STRING:
            n = struct.unpack_from("<i", self.data, p)[0]
            return self.data[p + 4:p + 4 + n].decode("latin1")
        return struct.unpack_from("<3f", self.data, p)

    def set(self, fields, name, value):
        t, p = fields[name]
        if t == DATA_INT:
            struct.pack_into("<i", self.data, p, int(value))
        elif t == DATA_FLOAT:
            struct.pack_into("<f", self.data, p, float(value))
        else:
            raise ValueError("only numeric fields are editable")

    def write(self):
        if not os.path.exists(self.path + ".bak"):
            shutil.copy2(self.path, self.path + ".bak")
        open(self.path, "wb").write(self.data)


def numeric(save, fields):
    return [n for n in save.names if fields[n][0] in (DATA_INT, DATA_FLOAT)]


# ---- GUI ------------------------------------------------------------------

def gui(path=None):
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox

    root = tk.Tk()
    root.title("Prelude Save Editor")
    if not path:
        path = filedialog.askopenfilename(
            title="Open Prelude savegame", filetypes=[("Savegames", "*.gam *.sav"), ("All", "*.*")],
            initialdir=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "saves"))
        if not path:
            return
    save = Save(path)
    root.title("Prelude Save Editor - " + os.path.basename(path))

    nb = ttk.Notebook(root)
    nb.pack(fill="both", expand=True)
    entries = []                          # (fields, name, StringVar)

    for fields in save.members:
        frame = ttk.Frame(nb, padding=6)
        nb.add(frame, text=save.get(fields, "NAME"))
        for i, name in enumerate(numeric(save, fields)):
            col, row = divmod(i, 32)
            ttk.Label(frame, text=name).grid(row=row, column=col * 2, sticky="w", padx=(8, 2))
            var = tk.StringVar(value=str(save.get(fields, name)))
            ttk.Entry(frame, textvariable=var, width=9).grid(row=row, column=col * 2 + 1, sticky="w")
            entries.append((fields, name, var))

    def do_save():
        try:
            for fields, name, var in entries:
                save.set(fields, name, var.get())
        except ValueError as e:
            messagebox.showerror("Bad value", str(e))
            return
        save.write()
        messagebox.showinfo("Saved", "Written to %s\n(backup: %s.bak)" % (path, path))

    ttk.Button(root, text="Save", command=do_save).pack(pady=4)
    root.mainloop()


# ---- CLI ------------------------------------------------------------------

def main(argv):
    if not argv:
        return gui()
    save = Save(argv[0])
    if len(argv) == 1:
        for fields in save.members:
            print("[%s]" % save.get(fields, "NAME"))
            for name in numeric(save, fields):
                print("  %-20s %s" % (name, save.get(fields, name)))
        return
    who, field, value = argv[1], argv[2].upper(), argv[3]
    for fields in save.members:
        if save.get(fields, "NAME").lower() == who.lower():
            save.set(fields, field, value)
            save.write()
            print("%s %s = %s" % (who, field, save.get(fields, field)))
            return
    sys.exit("no party member named %r" % who)


def _selftest():
    """Round-trip against the shipped saves: parse, patch, re-parse."""
    import tempfile
    src = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "saves", "save1.gam")
    tmp = os.path.join(tempfile.mkdtemp(), "t.gam")
    shutil.copy2(src, tmp)
    s = Save(tmp)
    assert s.members, "no party members parsed"
    assert all(save_name(s, f) for f in s.members), "unnamed member"
    s.set(s.members[0], "STRENGTH", 42)
    s.write()
    s2 = Save(tmp)
    assert s2.get(s2.members[0], "STRENGTH") == 42
    assert len(s2.data) == len(s.data), "file size changed"
    assert save_name(s2, s2.members[0]) == save_name(s, s.members[0])
    print("ok:", [save_name(s2, f) for f in s2.members])


def save_name(s, fields):
    return s.get(fields, "NAME")


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        _selftest()
    else:
        main(sys.argv[1:])
