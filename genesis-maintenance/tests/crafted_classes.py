#!/usr/bin/env python3
"""Native socket/enrollment/save/relogin checks; all data and accounts disposable."""
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
from class_abilities import constant, command
from smoke_runtime import receive

ROOT = Path(__file__).resolve().parents[1]

def main():
    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY") or ctypes.util.find_library("crypt"))
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode(), b"Craftprobe").decode()
    fixtures = {
        "Mognew": (0, ""),
        "Knightnew": (0, ""),
        "Mogkit": (constant("CLASS_MOOGLE"), "CraftedClass 1 1018 8 10000\n"),
        "Knightkit": (constant("CLASS_DEATHKNIGHT"), "CraftedClass 1 1017 8 10000\n"),
        "Mogbad": (constant("CLASS_MOOGLE"), "CraftedClass 1 1017 8 10000\n"),
    }
    for name, macro, parent, tracks in (("Garg", "CLASS_GARGOYLE",1015,1),
            ("Kuei","CLASS_KUEIJIN",1016,18),("Wraith","CLASS_WRAITH",1019,11)):
        fixtures[name+"new"] = (0,"")
        levels = " ".join(["5"]*tracks+["0"]*(20-tracks))
        fixtures[name+"kit"] = (constant(macro),f"LineageClass 1 {parent} 10000 {5 if parent==1016 else 0} {levels}\n")
    fixtures["Kueibad"] = (constant("CLASS_KUEIJIN"),"LineageClass 1 1019 10000 0 "+" ".join(["0"]*20)+"\n")
    with tempfile.TemporaryDirectory(prefix="genesis-crafted-") as tmp:
        work = Path(tmp)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        for name, (kind, fields) in fixtures.items():
            (work / "player" / name).write_text(
                f"#PLAYER\nName {name}~\nPassword {hashed}~\nLevel 2\nClass {kind}\n"
                "Room 3001\nStats 5 5 5 5 5\nExp 100000000\n"
                "HpMana 50000 1000000 100000 100000\nFlag 2\n"
                f"AffectedBy {constant('AFF_INFRARED')}\n{fields}End\n#END\n")
        with socket.socket() as reserve:
            reserve.bind(("127.0.0.1", 0))
            port = reserve.getsockname()[1]
        env = os.environ.copy()
        env.update(PORT=str(port), ASAN_OPTIONS="detect_leaks=0:abort_on_error=1", UBSAN_OPTIONS="halt_on_error=1")
        clients = {}
        with (work / "server.log").open("w") as log:
            proc = subprocess.Popen(["sh", str(work / "run.sh")], cwd=work / "area",
                                    env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                for _ in range(80):
                    if proc.poll() is not None:
                        raise RuntimeError((work / "server.log").read_text()[-3000:])
                    try:
                        probe = socket.create_connection(("127.0.0.1", port), timeout=1)
                        probe.close()
                        break
                    except OSError:
                        time.sleep(.25)
                else:
                    raise RuntimeError("Native server failed to boot")
                def login(name):
                    client = socket.create_connection(("127.0.0.1", port), timeout=2)
                    receive(client)
                    command(client, name, b"password")
                    client.sendall(password.encode() + b"\r\n")
                    receive(client)
                    command(client, "", b"Welcome to Godwars: Genesis")
                    clients[name] = client
                def run(name, text, expected):
                    assert proc.poll() is None
                    return command(clients[name], text, expected)
                def record(name):
                    run(name, "save", b"Saved.")
                    return (work / "player" / name).read_text()
                def state(name):
                    m = re.search(r"^CraftedClass\s+1\s+(\d+)\s+(\d+)\s+(\d+)", record(name), re.M)
                    assert m, name
                    return tuple(map(int, m.groups()))
                def lineage(name):
                    m = re.search(r"^LineageClass\s+1\s+(.+)",record(name),re.M)
                    assert m,name
                    values = tuple(map(int,m[1].split()))
                    assert len(values)==23
                    return values
                for name in fixtures:
                    login(name)
                for name, family, parent in (("Mognew","moogle",1018),("Knightnew","deathknight",1017)):
                    run(name, f"autoclass {family}", b"Your original Genesis class is active")
                    assert state(name) == (parent,0,1000)
                    run(name, f"{family} train", b"mastery improves")
                    assert state(name) == (parent,1,1000)
                    run(name, f"{family} focus 1000", b"one to one")
                    assert state(name) == (parent,1,2000)
                run("Mogkit","moogle pom_ward",b"power takes effect")
                assert state("Mogkit") == (1018,8,9900)
                run("Knightkit","deathknight graveward",b"power takes effect")
                assert state("Knightkit") == (1017,8,9850)
                run("Mogbad","moogle status",b"Mastery 0/8")
                assert state("Mogbad") == (1018,0,0)
                before = record("Mogkit")
                run("Mogkit","hybrid moogle",b"Moogle")
                assert state("Mogkit") == (1018,8,9900)
                run("Mogkit","quit",b"Twas brillig")
                clients.pop("Mogkit").close()
                login("Mogkit")
                run("Mogkit","moogle status",b"Mastery 8/8")
                assert state("Mogkit") == (1018,8,9900)
                assert re.search(r"^Class\s+131072$", record("Mogkit"), re.M)
                run("Mogkit","who",b"Moogle")
                for name, family, parent, track in (("Gargnew","gargoyle",1015,"visceratika"),
                        ("Kueinew","kueijin",1016,"yin"),("Wraithnew","wraith",1019,"argos")):
                    run(name,f"autoclass {family}",b"Your original Genesis class is active")
                    assert lineage(name)[:3] == (parent,1000,0)
                    run(name,f"{family} train {track}",b"discipline improves")
                    assert lineage(name)[3] == 1
                    run(name,f"{family} focus 1000",b"one to one")
                    assert lineage(name)[1] == 2000
                run("Gargkit","gargoyle wings",b"ancestral power takes effect")
                assert lineage("Gargkit")[1] == 9900
                run("Kueikit","kueijin akuma",b"ancestral power takes effect")
                assert lineage("Kueikit")[:3] == (1016,9200,5)
                run("Wraithkit","wraith dirge",b"ancestral power takes effect")
                assert lineage("Wraithkit")[1] == 9850
                run("Kueibad","kueijin status",b"Dharma 0/5")
                assert lineage("Kueibad") == (1016,0,0)+(0,)*20
                run("Kueikit","quit",b"Twas brillig")
                clients.pop("Kueikit").close()
                login("Kueikit")
                run("Kueikit","kueijin status",b"Dharma 5/5")
                assert lineage("Kueikit")[:3] == (1016,9200,5)
                run("Wraithkit","who",b"Wraith")
                assert "CraftedClass" in before
                print("All five native classes: actual enrollment, training/focus, ability/Akuma dispatch, real saves/relogin, mismatched-owner resets, who labels and read-only hybrid preview passed", flush=True)
            finally:
                for client in clients.values():
                    client.close()
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=5)
                text = (work / "server.log").read_text(errors="replace")
                for marker in ("AddressSanitizer", "runtime error:", "Segmentation fault"):
                    assert marker not in text, text[-3000:]

if __name__ == "__main__":
    main()
