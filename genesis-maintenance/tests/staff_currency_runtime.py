#!/usr/bin/env python3
"""Real login/pset/save/reload in disposable data; never touches live players."""
import ctypes
import ctypes.util
import os
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import tempfile
import time

from class_abilities import command, constant
from smoke_runtime import receive

ROOT = Path(__file__).resolve().parents[1]


def main():
    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                        or ctypes.util.find_library("crypt") or "libcrypt.so.1")
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode("ascii"), b"Currencyprobe")
    if not hashed:
        raise RuntimeError("Test password hashing failed")
    cases = [
        ("Cyberprobe", "cyborg", [("nanites", 1234), ("bionics", 234567),
                                 ("humanity", 150), ("bombs", 3)]),
        ("Demonprobe", "demon", [("souls", 76543)]),
        ("Ninjaprobe", "ninja", [("ki", 300)]),
        ("Mageprobe", "mage hermes", [("quint", 10000)]),
        ("Moogleprobe", "moogle", [("pomenergy", 10000)]),
        ("Knightprobe", "deathknight", [("soulenergy", 10000)]),
        ("Gargprobe", "gargoyle", [("stoneenergy", 10000)]),
        ("Kueiprobe", "kueijin", [("chi", 10000)]),
        ("Wraithprobe", "wraith", [("pathos", 10000)]),
    ]
    try:
        with socket.socket(socket.AF_INET6, socket.SOCK_STREAM):
            ipv6 = True
    except OSError:
        ipv6 = False
    with tempfile.TemporaryDirectory(prefix="genesis-staff-currencies-") as directory:
        work = Path(directory)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        for name in ["Adminprobe"] + [row[0] for row in cases]:
            record = (
                "#PLAYER\nName " + name + "~\nPassword " + hashed.decode("ascii")
                + "~\nLevel " + ("7" if name == "Adminprobe" else "2")
                + "\nClass 0\nRoom 3001\nHpMana 1000000 1000000 100000 100000\n"
                + "Stats 5 5 5 5 5\nExp 100000000\nPrimal 100000\nFlag 2\n"
                + "AffectedBy " + str(constant("AFF_INFRARED")) + "\nEnd\n#END\n"
            )
            (work / "player" / name).write_text(record)
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        env = os.environ.copy()
        env.update(PORT=str(port), ASAN_OPTIONS="detect_leaks=0:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        clients = []
        with (work / "server.log").open("w") as log:
            server = subprocess.Popen(["sh", str(work / "run.sh")],
                                      cwd=work / "area", env=env,
                                      stdout=log, stderr=subprocess.STDOUT)
            try:
                for _ in range(120):
                    if server.poll() is not None:
                        raise RuntimeError("Test server exited at startup")
                    try:
                        with socket.create_connection(("127.0.0.1", port), timeout=1):
                            break
                    except OSError:
                        time.sleep(0.25)
                else:
                    raise RuntimeError("Test server did not listen")

                def login(name, use_ipv6=False):
                    s = socket.create_connection(("::1" if use_ipv6 else "127.0.0.1", port), timeout=3)
                    clients.append(s)
                    receive(s)
                    command(s, name, b"password")
                    s.sendall(password.encode("ascii") + b"\r\n")
                    receive(s)
                    if name == "Adminprobe":
                        s.sendall(b"n\r\n")
                        receive(s)
                        command(s, "look", b"Temple of Maraden")
                    else:
                        command(s, "", b"Welcome to Godwars: Genesis")
                    return s

                def rows(name):
                    return {line.split()[0]: line.split()[1:]
                            for line in (work / "player" / name).read_text().splitlines()
                            if line.strip()}

                admin = login("Adminprobe", ipv6)
                command(admin, "help pset", b"STAFF PSET | absolute player balances")
                for index, (name, klass, resources) in enumerate(cases):
                    player = login(name, ipv6 and bool(index % 2))
                    command(player, "autoclass " + klass, (b"class is active", b"now a"))
                    if index == 0:
                        command(player, "cyborg install battery", b"modification complete")
                    for field, amount in resources:
                        command(admin, f"pset {name} {field} {amount}", b"Normal save/autosave")
                    if index == 0:
                        for field, amount in [("xp", 1234567), ("cps", 98765),
                                              ("qp", 321), ("gold", 456), ("primal", 54321)]:
                            command(admin, f"pset {name} {field} {amount}", b"Normal save/autosave")
                        command(player, f"pset {name} xp 999", b"Huh?")
                    command(admin, f"pset {name} currencies", b"PSET CURRENCIES")
                    command(player, "save", b"Saved.")
                    player.sendall(b"quit\r\n")
                    receive(player)
                    player.close()
                    first = rows(name)
                    player = login(name, ipv6)
                    command(player, "save", b"Saved.")
                    second = rows(name)
                    if index == 0:
                        for key, value in [("Exp", 1234567), ("Quest", 321),
                                           ("Gold", 456), ("Primal", 54321)]:
                            assert int(first[key][0]) == value == int(second[key][0]), key
                        assert first["Classpoints"] == second["Classpoints"]
                        assert int(second["Classpoints"][0]) == 98765
                        assert first["CyborgRuntime"] == second["CyborgRuntime"]
                        assert int(second["CyborgRuntime"][1]) == 1234
                        assert first["CyborgSystems"] == second["CyborgSystems"]
                        assert int(second["CyborgSystems"][-1]) == 234567
                        assert int(second["CyborgSystems"][5]) == 3
                        assert first["CyborgCore"] == second["CyborgCore"]
                    elif index == 1:
                        assert first["ParentLegacy"] == second["ParentLegacy"]
                        assert int(second["ParentLegacy"][1]) == 76543
                    elif index == 2:
                        assert int(first["Ninja"][3]) == 300 == int(second["Ninja"][3])
                    elif index == 3:
                        assert first["Quint"] == second["Quint"] and "10000" in second["Quint"]
                    else:
                        key = "CraftedClass" if index < 6 else "LineageClass"
                        assert first[key] == second[key], key
                        assert int(second[key][3 if key == "CraftedClass" else 2]) == 10000
                    player.sendall(b"quit\r\n")
                    receive(player)
                    player.close()
                command(admin, "shutdown", b"Shutdown by")
                server.wait(timeout=10)
                assert server.returncode == 0
            except Exception:
                log.flush()
                print((work / "server.log").read_text())
                raise
            finally:
                for s in clients:
                    s.close()
                if server.poll() is None:
                    server.terminate()
                    try:
                        server.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        server.kill()
                        server.wait()
        text = (work / "server.log").read_text()
        assert "AddressSanitizer" not in text and "runtime error:" not in text
        print("Native staff currencies passed: nine class families, real pset/permission checks, "
              "save/relogin persistence and clean shutdown; "
              + ("IPv6 and IPv4 TCP sessions" if ipv6 else "IPv4 TCP (kernel has no IPv6)"))


if __name__ == "__main__":
    main()
