#!/usr/bin/env python3
"""Focused regressions for the nine audited defects; disposable data only."""
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
    match = re.search(r"^" + key + r"\s+([^\n]+)", record, re.M)
    assert match, "Missing saved field: " + key
    return [int(n) for n in match[1].split()]


def main():
    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                        or ctypes.util.find_library("crypt") or "libcrypt.so.1")
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode(), b"Fixprobe")
    assert hashed
    tribes = "Tribes " + "5 " * 20 + "\n"
    wolf = "Breed 5 5 5\n" + tribes
    fixtures = {
        "Adminprobe": (0, "Level 7\n"),
        "Mageprobe": (32, "Spheres " + "5 " * 9 + "\nQuint 0 0 0\n"),
        "Wolfprobe": (2, wolf),
        "Unflagwolf": (2, wolf + "Flag 0\n"),
        "Monkprobe": (256, "Monk 8 8 8 0\nHpMana 1000000 1000000 5000 5000\n"),
        "Blessmonk": (256, "Monk 8 8 8 0\n"),
        "Victimprobe": (0, "Extra " + str(constant("EXTRA_SUMMON")) + "\n"),
        "Protected": (0, "Flag 0\n"),
        "Elementprobe": (512, "Powers " + "10 " * 20 + "\n"),
        "Timerprobe": (0, "Stats 1 1 1 1 1\n"),
        "Orphanprobe": (0, "Xtra 7\n"),
        "Ghostprobe": (0, "Xtra 0\nSlow_timer 10\nLiquify_timer 10\nAtoningwinds_timer 10\n"),
        "Boundprobe": (0, "Xtra 7\nSlow_timer 9999\nLiquify_timer 9999\nAtoningwinds_timer 9999\n"),
        "Statprobe": (0, "Stats 8 8 8 8 8\nAffectData 'weakness' 10 -5 "
                      + str(constant("APPLY_STR")) + " 0\n"),
    }
    with tempfile.TemporaryDirectory(prefix="genesis-fixes-") as temporary:
        work = Path(temporary)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        for name, (kind, fields) in fixtures.items():
            record = ("#PLAYER\nName " + name + "~\nPassword " + hashed.decode()
                      + "~\nLevel 2\nClass " + str(kind)
                      + "\nRoom 3001\nFlag 2\nAffectedBy "
                      + str(constant("AFF_INFRARED"))
                      + "\nHpMana 1000000 1000000 100000 100000\n"
                      + fields + "End\n")
            if name in ("Victimprobe", "Protected", "Adminprobe"):
                record += ("#OBJECT\nVnum 3000\nNest 0\nName auditblade~\n"
                           "ShortDescr an audit blade~\nItemType 5\nValues 0 1 1 3\n"
                           "Questowner " + name + "~\n"
                           "WearFlags 8193\nWearLoc "
                           + str(constant("WEAR_WEAPON")) + "\nEnd\n")
            if name == "Victimprobe":
                record += ("#OBJECT\nVnum 3000\nNest 0\nName auditbarrel~\n"
                           "ShortDescr an audit barrel~\nItemType 17\n"
                           "Questowner Victimprobe~\nEnd\n")
            (work / "player" / name).write_text(record + "#END\n")
        clients = {}
        proc = None
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        log_path = work / "server.log"
        with log_path.open("w") as log:
            def stop():
                nonlocal proc
                for client in clients.values():
                    client.close()
                clients.clear()
                if proc is not None and proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()

            def start():
                nonlocal proc, port
                with socket.socket() as reservation:
                    reservation.bind(("127.0.0.1", 0))
                    port = reservation.getsockname()[1]
                env["PORT"] = str(port)
                proc = subprocess.Popen(["sh", str(work / "run.sh")],
                                        cwd=work / "area", env=env,
                                        stdout=log, stderr=subprocess.STDOUT)
                for _ in range(120):
                    assert proc.poll() is None, "Server exited during startup"
                    try:
                        with socket.create_connection(("127.0.0.1", port), 1):
                            return
                    except OSError:
                        time.sleep(0.2)
                raise AssertionError("Server did not start")

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
                    command(client, "look", b"Temple of Maraden")
                else:
                    command(client, "", b"Welcome to Godwars: Genesis")

            def run(name, text, expected):
                assert proc.poll() is None
                return command(clients[name], text, expected)

            def snapshot(name):
                run(name, "save", b"Saved.")
                return (work / "player" / name).read_text()

            def room(flags):
                clients["Adminprobe"].sendall(f"rset 3001 flags {flags}\r\n".encode())
                run("Adminprobe", "look", b"Temple of Maraden")

            def armed(name):
                record = snapshot(name)
                return bool(re.search(r"^WearLoc\s+" + str(constant("WEAR_WEAPON"))
                                      + r"$", record, re.M))

            port = 0
            try:
                start()
                for name in ("Adminprobe", "Mageprobe", "Wolfprobe", "Unflagwolf",
                             "Monkprobe", "Victimprobe", "Protected"):
                    login(name)
                room(12)
                # All four optional-quint parsing sites reject bad input
                # without changing mana, BODY or health.
                for spell in ("mageinvis", "beginnersluck", "ward", "rift Victimprobe"):
                    before = snapshot("Mageprobe")
                    for value in ("-100", "1junk", "abc", "99999999999999999999999999999"):
                        run("Mageprobe", spell + " " + value, b"valid nonnegative integer")
                    after = snapshot("Mageprobe")
                    assert numbers(before, "Quint") == numbers(after, "Quint")
                    assert numbers(before, "HpMana") == numbers(after, "HpMana")
                print("Mage invalid-input regressions passed", flush=True)

                # No direct, abbreviated, alias or recursive forced inventory
                # and account actions, including the all-target branch.
                for target in ("Victimprobe", "all"):
                    for action in ("gift auditbarrel Wolfprobe", "gif auditbarrel Wolfprobe",
                                   "claim auditbarrel", "clai auditbarrel", "quit", "qui",
                                   "obedience Victimprobe look", "order all quit"):
                        run("Wolfprobe", "obedience " + target + " " + action,
                            b"permits movement")
                assert "Questowner   Victimprobe~" in snapshot("Victimprobe")
                receive(clients["Victimprobe"])
                run("Wolfprobe", "obedience Victimprobe say obeyed", b"I command")
                assert b"You say" in receive(clients["Victimprobe"])
                for caster, target in (("Unflagwolf", "Victimprobe"),
                                       ("Wolfprobe", "Protected"), ("Wolfprobe", "Adminprobe")):
                    run(caster, "obedience " + target + " look", b"protected target")
                room(1036)
                run("Wolfprobe", "obedience Victimprobe look", b"protected target")
                assert armed("Victimprobe")
                run("Wolfprobe", "jamtech Victimprobe", b"prevents you")
                assert armed("Victimprobe")
                room(12)
                for caster, target in (("Unflagwolf", "Victimprobe"),
                                       ("Wolfprobe", "Protected"), ("Wolfprobe", "Adminprobe")):
                    run(caster, "jamtech " + target, b"protected target")
                    assert armed(target)
                for _ in range(30):
                    result = run("Wolfprobe", "jamtech Victimprobe",
                                 (b"stop working", b"lose your concentration"))
                    if b"stop working" in result:
                        break
                else:
                    raise AssertionError("Jam Technology never succeeded on an eligible target")
                assert not armed("Victimprobe")
                print("Obedience and Jam Technology protection/eligible-target regressions passed",
                      flush=True)

                # Formula and 5000-entry gate remain unchanged, but no
                # successful or refused cast can overdraw mana.
                for _ in range(8):
                    run("Adminprobe", "restore Monkprobe", b"Ok")
                    run("Monkprobe", "touch self", (b"heal yourself", b"enough mana"))
                    assert numbers(snapshot("Monkprobe"), "HpMana")[2] >= 0
                login("Blessmonk")
                run("Blessmonk", "touch self", b"heal yourself")
                run("Blessmonk", "almighty group", b"worries disappear")
                before = numbers(snapshot("Blessmonk"), "HpMana")[2]
                run("Blessmonk", "almighty group", b"already blessed")
                after = numbers(snapshot("Blessmonk"), "HpMana")[2]
                assert after >= before  # Regeneration is allowed, another charge is not.
                print("Monk affordability and all-blessed group regressions passed", flush=True)

                before = numbers(snapshot("Wolfprobe"), "Armor")[0]
                run("Wolfprobe", "totemgift", b"protective spirit")
                assert numbers(snapshot("Wolfprobe"), "Armor")[0] == before + 20
                run("Wolfprobe", "totemgift dismiss", b"dismiss")
                assert numbers(snapshot("Wolfprobe"), "Armor")[0] == before
                print("Totemgift sign/removal regression passed", flush=True)

                # Cast all three real slows, persist them, restart the process,
                # then prove the loaded remaining times expire normally.
                login("Elementprobe")
                login("Timerprobe")
                run("Elementprobe", "slow Timerprobe", b"slows down")
                run("Elementprobe", "liquifyearth Timerprobe", b"slowing them down")
                run("Elementprobe", "atoningwinds Timerprobe", b"wind on")
                timers = ("Slow_timer", "Liquify_timer", "Atoningwinds_timer")
                saved = snapshot("Timerprobe")
                assert numbers(saved, "Xtra")[0] & 7 == 7
                remaining = {key: numbers(saved, key)[0] for key in timers}
                assert all(0 < value <= 20 for value in remaining.values())
                stop()
                start()
                login("Timerprobe")
                loaded = snapshot("Timerprobe")
                assert numbers(loaded, "Xtra")[0] & 7 == 7
                assert all(0 < numbers(loaded, key)[0] <= remaining[key] for key in timers)
                for name in ("Orphanprobe", "Ghostprobe", "Boundprobe", "Statprobe"):
                    login(name)
                for name in ("Orphanprobe", "Ghostprobe"):
                    record = snapshot(name)
                    assert not numbers(record, "Xtra")[0] & 7
                    assert all(numbers(record, key)[0] == 0 for key in timers)
                record = snapshot("Boundprobe")
                assert all(0 < numbers(record, key)[0] <= 20 for key in timers)
                score = run("Statprobe", "score", b"Quest Points")
                assert re.search(rb"Str:\s+3/8", score), score
                record = snapshot("Statprobe")
                assert numbers(record, "Stats")[0] == 8
                deadline = time.monotonic() + 25
                while time.monotonic() < deadline:
                    records = [snapshot(name) for name in ("Timerprobe", "Boundprobe")]
                    if all(not numbers(record, "Xtra")[0] & 7 for record in records):
                        break
                    time.sleep(1)
                else:
                    raise AssertionError("Loaded slow flags did not expire")
                assert all(numbers(record, key)[0] == 0 for record in records for key in timers)
                print("Real Elemental casts, save/restart/reload/expiry, orphan repair and loaded attributes passed",
                      flush=True)
            except BaseException:
                log.flush()
                print(log_path.read_text()[-6000:], flush=True)
                raise
            finally:
                stop()
        text = log_path.read_text()
        assert "AddressSanitizer" not in text and "runtime error:" not in text, text[-6000:]
    print("Nine-fix runtime regressions passed")


if __name__ == "__main__":
    main()
