#!/usr/bin/env python3
"""Original legacy items, persistence, sensing and travel in a disposable world."""
import ctypes
import ctypes.util
import os
from pathlib import Path
import re
import secrets
import shutil
import socket
import subprocess
import tempfile
import time

from class_abilities import command, constant
from smoke_runtime import receive

ROOT = Path(__file__).resolve().parents[1]


def numbers(record, key):
    match = re.search(r"^" + key + r"\s+([^\r\n]+)", record, re.M)
    return [int(n) for n in match[1].split()] if match else [0]


def affected(record, name):
    return bool(re.search(r"^AffectData\s+'" + re.escape(name) + "'", record, re.M))


def main():
    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                        or ctypes.util.find_library("crypt") or "libcrypt.so.1")
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode(), b"Legacyprobe").decode()
    recall = ("#OBJECT\nVnum 1370\nNest 0\nName white milky potion~\n"
              "ShortDescr a milky white potion~\nDescription A milky white potion is here.~\n"
              "ItemType 10\nWearFlags 1\n"
              "Values 14 0 0 0\nSpell 1 'word of recall'\nEnd\n")
    sword = ("#OBJECT\nVnum 2435\nNest 0\nLevel 20\nName sword~\n"
             "ShortDescr a sword~\nDescription A sword lies here.~\n"
             "ItemType 5\nWearFlags 8193\n"
             "Values 0 0 0 3\nSpectype 36\nSpecpower 41\n"
             "Questowner Legacyprobe~\nWearLoc "
             + str(constant("WEAR_WEAPON")) + "\nEnd\n")
    food = ("#OBJECT\nVnum 8116\nNest 0\nName sandwich~\n"
            "ShortDescr a sandwich~\nDescription A sandwich lies here.~\n"
            "ItemType 19\nWearFlags 1\n"
            "Values 5 0 0 1\nEnd\n")
    frenzy = ("#OBJECT\nVnum 1370\nNest 0\nName rage phial~\n"
              "ShortDescr a rage phial~\nDescription A rage phial is here.~\n"
              "ItemType 10\nWearFlags 1\nValues 14 0 0 0\n"
              "Spell 1 'frenzy'\nEnd\n")

    def wand(name, spell, held=False):
        return ("#OBJECT\nVnum 1370\nNest 0\nName " + name + " wand~\n"
                "ShortDescr a testing wand~\nDescription A testing wand is here.~\n"
                "ItemType 3\nWearFlags 16385\nValues 14 10 10 0\nSpell 3 '"
                + spell + "'\nWearLoc " + str(constant("WEAR_HOLD") if held else -1)
                + "\nEnd\n")

    fixtures = {
        "Adminprobe": (7, 990001, 0, wand("insight", "know alignment", True)
                       + wand("blessing", "darkblessing")),
        "Legacyprobe": (2, 990001, 0, sword + recall + food + frenzy * 3),
        "Hiddenprobe": (2, 990001, constant("AFF_HIDE"), ""),
        "Lockedprobe": (2, 990003, 0, recall),
        "Fightprobe": (2, 990002, 0, recall),
        "Planeprobe": (2, 990002, constant("AFF_SHADOWPLANE"), recall),
    }
    area = """#AREADATA
Name Legacy regression~
Builders None~
VNUMs 990001 990006
End
#MOBILES
#990004
evilprobe~
an evil test creature~
An evil test creature waits here.~
A disposable evil test creature.~
2 0 -500 S
1 0 0 1d1+0 1d1+0
0 0
8 8 1
#990005
goodprobe~
a good test creature~
A good test creature waits here.~
A disposable good test creature.~
2 0 500 S
1 0 0 1d1+0 1d1+0
0 0
8 8 1
#0
#ROOMS
#990001
Legacy Haven~
A quiet testing haven.~
0 0 0
D0
A closed door.~
door~
1 -1 990002
D1
A warded door.~
ward~
5 -1 990003
S
#990002
Legacy Hall~
A quiet testing hall.~
0 0 0
D2
A closed door.~
door~
1 -1 990001
S
#990003
Legacy Ward~
A travel-protected testing room.~
0 9216 0
S
#0
#$
"""
    with tempfile.TemporaryDirectory(prefix="genesis-legacy-items-") as tmp:
        work = Path(tmp)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        (work / "area/legacy_test.are").write_text(area)
        listing = (work / "area/area.lst").read_text()
        assert listing.rstrip().endswith("$")
        (work / "area/area.lst").write_text(listing.replace("$", "legacy_test.are\n$"))
        for name, (level, room, flags, items) in fixtures.items():
            record = ("#PLAYER\nName " + name + "~\nPassword " + hashed
                      + "~\nLevel " + str(level) + "\nClass 0\nRoom " + str(room)
                      + "\nHome 990001\nFlag 2\nSex 1\nStats 5 5 5 5 5\nAffectedBy "
                      + str(constant("AFF_INFRARED") | flags)
                      + "\nHpMana 1000000 1000000 "
                      + ("100000" if level == 7 else "1000") + " 100000\n")
            if name == "Fightprobe":
                record += "Fight_timer 30\n"
            (work / "player" / name).write_text(record + "End\n" + items + "#END\n")
        clients = {}
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        env = dict(os.environ, PORT=str(port),
                   ASAN_OPTIONS="detect_leaks=0:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        with (work / "server.log").open("w") as log:
            proc = subprocess.Popen(["sh", str(work / "run.sh")],
                                    cwd=work / "area", env=env,
                                    stdout=log, stderr=subprocess.STDOUT)
            try:
                for _ in range(120):
                    assert proc.poll() is None, "Startup failed"
                    try:
                        with socket.create_connection(("127.0.0.1", port), 1):
                            break
                    except OSError:
                        time.sleep(0.2)
                else:
                    raise AssertionError("Server did not start")

                def run(name, text, expected):
                    assert proc.poll() is None, "Server stopped"
                    return command(clients[name], text, expected)

                def snapshot(name):
                    run(name, "save", b"Saved.")
                    return (work / "player" / name).read_text()

                def login(name):
                    client = socket.create_connection(("127.0.0.1", port), 2)
                    clients[name] = client
                    receive(client)
                    command(client, name, b"password")
                    client.sendall(password.encode() + b"\r\n")
                    receive(client)
                    if name == "Adminprobe":
                        client.sendall(b"n\r\n")
                        receive(client)
                        run(name, "look", b"Legacy Haven")
                    else:
                        run(name, "", b"Welcome to Godwars: Genesis")

                def give(number, keyword):
                    run("Adminprobe", "oload " + str(number), b"You create")
                    run("Adminprobe", "give " + keyword + " Legacyprobe", b"You give")

                login("Adminprobe")
                run("Adminprobe", "close door", b"Ok.")
                run("Adminprobe", "close ward", b"Ok.")
                login("Legacyprobe")
                login("Hiddenprobe")
                baseline = snapshot("Legacyprobe")
                give(8114, "potion")
                run("Legacyprobe", "quaff potion", b"You feel blessed")
                record = snapshot("Legacyprobe")
                assert affected(record, "armor") and affected(record, "bless")
                assert numbers(record, "Armor")[0] == numbers(baseline, "Armor")[0] + 20
                assert numbers(record, "Hitroll")[0] == numbers(baseline, "Hitroll")[0] + 2
                assert numbers(record, "SavingThrow")[0] == numbers(baseline, "SavingThrow")[0] - 2
                give(8114, "potion")
                run("Legacyprobe", "quaff potion", b"already active")
                assert numbers(snapshot("Legacyprobe"), "Armor") == numbers(record, "Armor")
                print("Original mixed potion: Armor/Bless apply once, with correct AC/hit/save modifiers",
                      flush=True)

                give(8113, "potion")
                response = run("Legacyprobe", "quaff potion", b"detect poison in")
                assert b"You can't do that." not in response
                assert affected(snapshot("Legacyprobe"), "detect magic")
                run("Legacyprobe", "inventory", b"Magical")
                before = run("Legacyprobe", "look", b"Legacy Haven")
                assert b"Hiddenprobe" not in before
                give(5211, "potion")
                run("Legacyprobe", "quaff potion", b"alert to hidden")
                after = run("Legacyprobe", "look", b"Legacy Haven")
                assert b"Hiddenprobe" in after
                print("Original sensing potions: magic labels, hidden visibility and poison screening",
                      flush=True)

                run("Legacyprobe", "press sword", b"Giant strength")
                assert affected(snapshot("Legacyprobe"), "giant strength")
                give(7812, "potion")
                before = numbers(snapshot("Legacyprobe"), "HpMana")[2]
                run("Legacyprobe", "quaff potion", b"refreshed")
                after = numbers(snapshot("Legacyprobe"), "HpMana")[2]
                assert before < after <= 100000
                print("Original custom sword and Refresh potion activate bounded resource/stat effects",
                      flush=True)

                run("Legacyprobe", "north", b"closed")
                give(2015, "potion")
                run("Legacyprobe", "quaff potion", b"ordinary closed doors")
                run("Legacyprobe", "north", b"Legacy Hall")
                assert numbers(snapshot("Legacyprobe"), "Room") == [990002]
                run("Legacyprobe", "south", b"Legacy Haven")
                run("Legacyprobe", "east", b"closed")
                assert numbers(snapshot("Legacyprobe"), "Room") == [990001]
                run("Legacyprobe", "north", b"Legacy Hall")
                run("Legacyprobe", "quaff potion", b"recalls you to your haven")
                assert numbers(snapshot("Legacyprobe"), "Room") == [990001]
                print("Original travel potions: ordinary doors pass, wards hold, Recall reaches home",
                      flush=True)

                give(5314, "herbs")
                run("Legacyprobe", "eat herbs", b"different sex")
                record = snapshot("Legacyprobe")
                changed = numbers(record, "Sex")[0]
                assert changed in (0, 2) and affected(record, "change sex")
                run("Legacyprobe", "quit", b"Carroll")
                clients["Legacyprobe"].close()
                login("Legacyprobe")
                record = snapshot("Legacyprobe")
                assert numbers(record, "Sex") == [changed] and affected(record, "change sex")
                assert affected(record, "armor") and affected(record, "bless")
                run("Legacyprobe", "cast 'dispel magic' self", b"remove all magical")
                record = snapshot("Legacyprobe")
                assert numbers(record, "Sex") == [1]
                assert not affected(record, "change sex") and not affected(record, "armor")
                assert numbers(record, "Armor") == numbers(baseline, "Armor")
                assert numbers(record, "Hitroll") == numbers(baseline, "Hitroll")
                assert numbers(record, "SavingThrow") == numbers(baseline, "SavingThrow")
                assert numbers(record, "Stats") == [5] * 5
                print("Timed effects survive real logout/login; dispel restores sex, stats and defenses",
                      flush=True)

                for name in ("Lockedprobe", "Fightprobe", "Planeprobe"):
                    login(name)
                    before = numbers(snapshot(name), "Room")
                    run(name, "quaff potion", b"recall is blocked")
                    assert numbers(snapshot(name), "Room") == before
                print("Recall refuses protected rooms, active fight timers and plane crossing",
                      flush=True)

                for vnum in (990004, 990005):
                    run("Adminprobe", "mload " + str(vnum), b"You have created")
                run("Adminprobe", "zap evilprobe", b"evil (-500)")
                run("Adminprobe", "zap goodprobe", b"good (500)")
                run("Adminprobe", "zap Legacyprobe", b"no NPC alignment")
                run("Adminprobe", "remove insight", b"You stop using")
                run("Adminprobe", "hold blessing", b"You clutch")
                run("Adminprobe", "zap goodprobe", b"only evil NPCs")
                run("Adminprobe", "zap Legacyprobe", b"only evil NPCs")

                def mobstats():
                    text = run("Adminprobe", "mstat evilprobe", b"Hitroll:").decode(
                        "ascii", errors="replace")
                    return (text, int(re.search(r"AC:\s+(-?\d+)", text)[1]),
                            *map(int, re.search(
                                r"Hitroll:\s+(-?\d+)\.\s+Damroll:\s+(-?\d+)", text).groups()))

                before = mobstats()
                run("Legacyprobe","help knowalignment",b"area-defined alignment")
                run("Legacyprobe","help frenzy",b"+1 hitroll")
                run("Legacyprobe","help darkblessing",b"evil NPCs only")
                run("Adminprobe", "zap evilprobe", b"You zap")
                after = mobstats()
                assert "Spell: 'darkblessing'" in after[0]
                assert after[1:] == (before[1] + 10, before[2] + 1, before[3] + 1)
                run("Adminprobe", "zap evilprobe", b"already active")
                assert mobstats()[1:] == after[1:]
                print("Know Alignment and Darkblessing: real wand dispatch, NPC output, "
                      "evil-only buffs and no stacking", flush=True)

                before = snapshot("Legacyprobe")
                run("Legacyprobe", "quaff phial", b"battle rage")
                record = snapshot("Legacyprobe")
                for key in ("Hitroll", "Damroll"):
                    assert numbers(record, key)[0] == numbers(before, key)[0] + 1
                run("Legacyprobe", "quaff phial", b"already active")
                assert numbers(snapshot("Legacyprobe"), "Hitroll") == numbers(record, "Hitroll")
                run("Legacyprobe", "quit", b"Carroll")
                clients["Legacyprobe"].close()
                login("Legacyprobe")
                record = snapshot("Legacyprobe")
                assert affected(record, "frenzy")
                assert numbers(record, "Hitroll")[0] == numbers(before, "Hitroll")[0] + 1
                run("Legacyprobe", "cast 'dispel magic' self", b"remove all magical")
                record = snapshot("Legacyprobe")
                assert not affected(record, "frenzy")
                assert numbers(record, "Hitroll") == numbers(before, "Hitroll")
                assert numbers(record, "Damroll") == numbers(before, "Damroll")
                run("Legacyprobe", "quaff phial", b"battle rage")
                snapshot("Legacyprobe")
                run("Legacyprobe", "quit", b"Carroll")
                clients["Legacyprobe"].close()
                path = work / "player/Legacyprobe"
                shortened, count = re.subn(r"^(AffectData\s+'frenzy')\s+-?\d+",
                                          r"\1 0", path.read_text(), flags=re.M)
                assert count == 2
                path.write_text(shortened)
                login("Legacyprobe")
                deadline = time.monotonic() + 75
                while affected(snapshot("Legacyprobe"), "frenzy"):
                    assert time.monotonic() < deadline, "Frenzy did not naturally expire"
                    time.sleep(1)
                record = snapshot("Legacyprobe")
                assert numbers(record, "Hitroll") == numbers(before, "Hitroll")
                assert numbers(record, "Damroll") == numbers(before, "Damroll")
                assert numbers(record, "Stats") == numbers(before, "Stats")
                print("Frenzy: real potion, duplicate rejection, logout/login, dispel and "
                      "natural expiry restore base modifiers", flush=True)
            except BaseException:
                log.flush()
                print((work / "server.log").read_text()[-6000:], flush=True)
                raise
            finally:
                for client in clients.values():
                    client.close()
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
        text = (work / "server.log").read_text()
        assert "AddressSanitizer" not in text and "runtime error:" not in text, text[-6000:]
    print("Legacy spell item, sensing, persistence and travel regressions passed")


if __name__ == "__main__":
    main()
