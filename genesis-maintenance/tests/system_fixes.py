#!/usr/bin/env python3
"""Isolated live tests for spell costs, saved combat indices and weapon procs."""
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


def field(record, key):
    match = re.search(r"^" + key + r"\s+([^\r\n]+)", record, re.M)
    assert match, "Missing field: " + key
    return [int(n) for n in match[1].split()]


def main():
    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                        or ctypes.util.find_library("crypt") or "libcrypt.so.1")
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode(), b"Spellprobe").decode()
    assert hashed
    bad_stance = "Stance 999 " + "200 " * 10 + "999\n"
    fixtures = {
        "Adminprobe": "Level 7\n",
        "Casterprobe": "Stats 8 8 8 8 8\n",
        "Targetprobe": "Stats 8 8 8 8 8\n",
        "Weaponprobe": "Stats 8 8 8 8 8\nStyle 3 0 0 200 0 0 0\n",
        "Procprobe": "Stats 8 8 8 8 8\nStyle 3 0 0 200 0 0 0\n",
        "Badfileprobe": "Style 999 0 0 0 0 0 0\n" + bad_stance,
    }
    with tempfile.TemporaryDirectory(prefix="genesis-systems-") as temporary:
        work = Path(temporary)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        for name, fields in fixtures.items():
            record = ("#PLAYER\nName " + name + "~\nPassword " + hashed
                      + "~\nLevel 2\nClass 0\nRoom 3001\nFlag 2\nAffectedBy "
                      + str(constant("AFF_INFRARED"))
                      + "\nHpMana 1000000 1000000 100000 100000\n"
                      + fields.replace("Level 7\n", "") + "End\n")
            if name == "Adminprobe":
                record = record.replace("Level 2\n", "Level 7\n")
            if name in ("Weaponprobe", "Procprobe"):
                spell = 999 if name == "Weaponprobe" else 34
                record += ("#OBJECT\nVnum 3000\nNest 0\nName auditblade~\n"
                           "ShortDescr an audit blade~\nItemType 5\n"
                           "Level 45\nValues " + str(spell) + " 1 1 3\n"
                           "Questowner " + name + "~\n"
                           "WearFlags 8193\nWearLoc "
                           + str(constant("WEAR_WEAPON")) + "\nEnd\n")
            (work / "player" / name).write_text(record + "#END\n")
        clients = {}
        port = 0
        proc = None
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        log_path = work / "server.log"
        with log_path.open("w") as log:
            try:
                with socket.socket() as reservation:
                    reservation.bind(("127.0.0.1", 0))
                    port = reservation.getsockname()[1]
                env["PORT"] = str(port)
                proc = subprocess.Popen(["sh", str(work / "run.sh")],
                                        cwd=work / "area", env=env,
                                        stdout=log, stderr=subprocess.STDOUT)
                for _ in range(120):
                    assert proc.poll() is None, "Server stopped during startup"
                    try:
                        with socket.create_connection(("127.0.0.1", port), 1):
                            break
                    except OSError:
                        time.sleep(0.2)
                else:
                    raise AssertionError("Server did not start")

                def run(name, text, expected):
                    assert proc.poll() is None, "Server stopped during test"
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
                        run(name, "look", b"Temple of Maraden")
                    else:
                        run(name, "", b"Welcome to Godwars: Genesis")

                for name in fixtures:
                    login(name)
                clients["Adminprobe"].sendall(b"rset 3001 flags 12\r\n")
                run("Adminprobe", "look", b"Temple of Maraden")

                record = snapshot("Badfileprobe")
                assert field(record, "Style")[0] == 1
                assert field(record, "Stance")[0] == field(record, "Stance")[11] == 0
                assert field(record, "Stance")[1:11] == [200] * 10
                run("Badfileprobe", "skill", b"not using a fighting stance")
                print("Corrupt saved stance/style selections repaired and displayed safely",
                      flush=True)

                before = field(snapshot("Casterprobe"), "HpMana")[2]
                run("Casterprobe", "cast 'control weather' sideways", b"Syntax:")
                assert field(snapshot("Casterprobe"), "HpMana")[2] == before
                run("Casterprobe", "cast 'control weather' better", b"clearer skies")
                run("Casterprobe", "cast 'control weather' worse", b"toward rain")
                after = field(snapshot("Casterprobe"), "HpMana")[2]
                assert before - 50 <= after <= before
                print("Live weather spell validation and both weather directions passed",
                      flush=True)

                before = field(snapshot("Casterprobe"), "HpMana")[2]
                run("Casterprobe", "cast 'energy drain' Casterprobe", b"own energy")
                assert field(snapshot("Casterprobe"), "HpMana")[2] >= before
                target_before = field(snapshot("Targetprobe"), "HpMana")[0]
                for _ in range(12):
                    result = run("Casterprobe", "cast 'energy drain' Targetprobe",
                                 b"glow")
                    run("Adminprobe", "peace", b"Ok")
                    target_after = field(snapshot("Targetprobe"), "HpMana")[0]
                    if target_after < target_before:
                        break
                else:
                    raise AssertionError("Energy Drain never damaged an eligible target")
                print("Live Energy Drain damages eligible targets; self-drain is free to reject",
                      flush=True)

                weapon = snapshot("Weaponprobe")
                assert re.search(r"^Values\s+999 1 1 3", weapon, re.M), weapon[-1000:]
                run("Weaponprobe", "kill Targetprobe", b"Targetprobe")
                run("Adminprobe", "peace", b"Ok")
                run("Weaponprobe", "score", b"Quest Points")
                assert proc.poll() is None
                print("Malformed equipped weapon proc is skipped without crashing",
                      flush=True)

                # Valid existing-area metadata for Energy Drain must still fire.
                assert re.search(r"^Values\s+34 1 1 3",
                                 snapshot("Procprobe"), re.M)
                for _ in range(12):
                    before = field(snapshot("Targetprobe"), "HpMana")[2]
                    run("Procprobe", "kill Targetprobe", b"Targetprobe")
                    run("Adminprobe", "peace", b"Ok")
                    after = field(snapshot("Targetprobe"), "HpMana")[2]
                    if after < before:
                        break
                else:
                    raise AssertionError("Valid Energy Drain weapon never drained mana")
                print("Valid weapon Energy Drain proc still activates", flush=True)
            except BaseException:
                log.flush()
                print(log_path.read_text()[-6000:], flush=True)
                raise
            finally:
                for client in clients.values():
                    client.close()
                if proc is not None and proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
        text = log_path.read_text()
        assert "AddressSanitizer" not in text and "runtime error:" not in text, text[-6000:]
    print("Spell, skill, stance and weapon-proc runtime regressions passed")


if __name__ == "__main__":
    main()
