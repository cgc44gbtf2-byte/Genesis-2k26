#!/usr/bin/env python3
"""Exercise restored class commands against disposable accounts and area data.

Build src/merc first. No production player files or area files are modified.
"""
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

from smoke_runtime import receive

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "src/merc.h").read_text()


def constant(name):
    match = re.search(r"^#define\s+" + name + r"\s+(\d+)", HEADER, re.M)
    if not match:
        raise AssertionError("Missing numeric constant: " + name)
    return int(match[1])


def command(client, text, expected):
    alternatives = expected if isinstance(expected, tuple) else (expected,)
    client.sendall(text.encode("ascii") + b"\r\n")
    data = bytearray()
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        client.settimeout(0.3)
        try:
            chunk = client.recv(65536)
        except socket.timeout:
            continue
        if not chunk:
            break
        data.extend(chunk)
        if any(value.lower() in data.lower() for value in alternatives):
            return bytes(data)
    raise AssertionError(f"{text!r}: expected {expected!r}, got {bytes(data)[-1500:]!r}")


def main():
    books_only=os.environ.get("GENESIS_BOOKS_ONLY")=="1"
    roster_only=os.environ.get("GENESIS_ROSTER_ONLY")=="1"
    help_only=os.environ.get("GENESIS_HELP_ONLY")=="1"
    if sum((books_only,roster_only,help_only))>1:
        raise ValueError("Choose only one focused native regression mode")
    # Verify every compatibility alias points at the canonical handler, and
    # prevent reintroducing silent duplicate command-name shadowing.
    table = (ROOT / "src/interp.c").read_text()
    table = re.sub(r"/\*.*?\*/|//[^\n]*", "", table, flags=re.S)
    entries = re.findall(r'\{\s*"([^"]+)"\s*,\s*(do_\w+)', table)
    registry = dict(entries)
    assert len(registry) == len(entries), "Duplicate command name"
    for alias, canonical in {
        "frictioncurse": "friction", "sorcheal": "mageheal",
        "sorcbetterbody": "betterbody", "sorcalterweight": "alterweight",
        "sorcinvis": "mageinvis", "sorcenchant": "mageenchant",
        "spiritsence": "spiritsense", "spiritfriend": "spiritguardian",
        "illusionaryform": "formillusion", "changeling": "malleable",
    }.items():
        assert registry[alias] == registry[canonical], alias

    crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                        or ctypes.util.find_library("crypt") or "libcrypt.so.1")
    crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
    crypt.crypt.restype = ctypes.c_char_p
    password = secrets.token_hex(12)
    hashed = crypt.crypt(password.encode("ascii"), b"Classprobe")
    if not hashed:
        raise RuntimeError("Could not hash disposable fixture password")
    powers = "Powers " + " ".join(["7"] * 20) + "\n"
    tribes = "Tribes " + " ".join(["5"] * 20) + "\n"
    fixtures = {
        "Adminprobe": (2, "Level 7\nRank 1\n"),
        "Noneprobe": (0, "Flag 0\nAffectedBy "
                      + str(constant("AFF_SHADOWPLANE")) + "\n"),
        "Mageprobe": (32, "Spheres " + " ".join(["5"] * 9) + "\n"),
        "Magenovice": (32, "Spheres " + " ".join(["0"] * 9) + "\n"),
        "Wolfprobe": (2, "Breed 5 5 5\nAuspice 0 0 0 0 0\n"
                       + "CyborgCore 1 1 1 100 96\n"
                      + tribes + "Rank 5\nAffectedBy "
                      + str(constant("AFF_SHADOWSIGHT") | constant("AFF_INFRARED")) + "\n"),
        "Wolfcub": (2, "Breed 0 0 0\nRank 1\nStats 1 1 1 1 1\nAffectedBy "
                    + str(constant("AFF_SHADOWPLANE")) + "\n"),
        "Vampprobe": (1, powers + "Blood 1000 1000\nBeast 50\n"),
        "Monkprobe": (256, "Monk 8 8 8 0\n"),
        "Monknovice": (256, "Monk 6 0 0 0\n"),
        "Phoenixprobe": (1024, "Powers 8 " + " ".join(["0"] * 19) + "\n"),
        "Phoenixnew": (0, "Stats 5 5 5 5 5\nExp 50000\n"),
        "Cybernew": (0, "Stats 5 5 5 5 5\nExp 60000\nPrimal 5000\n"),
        "Angelnew": (0, "Stats 5 5 5 5 5\nExp 50000\n"),
        "Demonnew": (0, "Stats 5 5 5 5 5\nExp 50000\n"),
        "Angelkit": (4096, "Stats 5 5 5 5 5\nPowers 8 "
                     + " ".join(["0"] * 19) + "\n"),
        "Demonkit": (8192, "Stats 5 5 5 5 5\nPowers 8 "
                     + " ".join(["0"] * 19) + "\n"),
        "Cyberclear": (2048, "CyborgCore 1 1 1 100 96\n"
                           + "CyborgRuntime 1 1000 2 0\n"),
        "Sorcbooknew": (0,"Stats 5 5 5 5 5\nExp 50000\nPrimal 123\n"),
        "Chaosbooknew": (0,"Stats 5 5 5 5 5\nExp 50000\nPrimal 123\n"),
        "Sorcbookold": (4,"Level 3\nStats 5 5 5 5 5\nPrimal 123\n"),
        "Drywolf": (2, "Breed 5 5 5\n" + tribes
                    + "HpMana 1000000 1000000 1 1\n"),
        "Dryvamp": (1, powers + "Beast 0\nHpMana 1000000 1000000 1 1\n"),
        "Dragprobe": (64, "DSkills " + str(constant("DRAG_WINGS"))
                      + "\nPolyaff " + str(constant("POLY_DRAGONFORM"))
                      + "\nAffectedBy " + str(constant("AFF_POLYMORPH"))
                      + "\nVampaff " + str(constant("VAM_DISGUISED")) + "\n"),
    }
    if books_only:
        fixtures={k:v for k,v in fixtures.items() if k in ("Sorcbooknew","Chaosbooknew","Sorcbookold")}
    if roster_only:
        fixtures={k:v for k,v in fixtures.items() if k=="Wolfprobe"}
    if help_only:
        fixtures={k:v for k,v in fixtures.items() if k=="Wolfprobe"}
    with tempfile.TemporaryDirectory(prefix="genesis-abilities-") as work:
        work = Path(work)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src/merc").symlink_to(ROOT / "src/merc")
        (work / "player").mkdir()
        for name, (kind, fields) in fixtures.items():
            record = (
                "#PLAYER\nName " + name + "~\nPassword " + hashed.decode("ascii")
                + "~\nLevel 2\nClass " + str(kind)
                + "\nRoom 3001\nHpMana 1000000 1000000 100000 100000\nFlag 2\n"
                + "AffectedBy " + str(constant("AFF_INFRARED")) + "\n"
                + fields + "End\n"
            )
            if name == "Mageprobe":
                record += ("#OBJECT\nVnum 3000\nNest 0\nName barrel beer~\n"
                           "ShortDescr an audit barrel~\nItemType 17\nEnd\n")
            (work / "player" / name).write_text(record + "#END\n")
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        env = os.environ.copy()
        env.update(PORT=str(port), ASAN_OPTIONS="detect_leaks=0:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        clients = {}
        log_path = work / "server.log"
        with log_path.open("w") as log:
            proc = subprocess.Popen(["sh", str(work / "run.sh")],
                                    cwd=work / "area", env=env,
                                    stdout=log, stderr=subprocess.STDOUT)
            try:
                for _ in range(120):
                    if proc.poll() is not None:
                        raise RuntimeError("Server exited during startup")
                    try:
                        probe = socket.create_connection(("127.0.0.1", port), timeout=1)
                        probe.close()
                        break
                    except OSError:
                        time.sleep(0.25)
                else:
                    raise RuntimeError("Server did not start")
                for name in fixtures:
                    client = socket.create_connection(("127.0.0.1", port), timeout=2)
                    clients[name] = client
                    receive(client)
                    command(client, name, b"password")
                    client.sendall(password.encode("ascii") + b"\r\n")
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

                def number(name, field, index=0):
                    text = snapshot(name)
                    match = re.search(r"^" + field + r"\s+([^\n]+)", text, re.M)
                    return int(match[1].split()[index]) if match else 0

                def room_flags(flags):
                    clients["Adminprobe"].sendall(
                        f"rset 3001 flags {flags}\r\n".encode("ascii"))
                    run("Adminprobe", "look", b"Temple of Maraden")

                if help_only:
                    entries=[]
                    for filename in ("mage_abilities.hlp","legacy_spells.hlp",
                                     "book_magic.hlp","combat_parents.hlp","drow_abilities.hlp",
                                     "ninja_abilities.hlp","werewolf_abilities.hlp",
                                     "monk_abilities.hlp","elemental_abilities.hlp",
                                     "vampire_advanced.hlp","shared_abilities.hlp",
                                     "legacy_class_helpers.hlp","kits.hlp"):
                        entries.extend(re.findall(r"^0 ([^~]+)~\n(.*?)~",
                            (ROOT/"area"/filename).read_text(),re.M|re.S))
                    checked=0
                    class_topics={"VAMPIRE","WEREWOLF","SORCERER","MAGE","DROW",
                                  "HIGHLANDER","DRAGON","NINJA","MONK","ELEMENTAL",
                                  "ANGEL","CYBORG","DEMON","PHOENIX","GARGOYLE",
                                  "KUEI-JIN","DEATHKNIGHT","MOOGLE","WRAITH"}
                    for keywords,body in entries:
                        if keywords.split()[0] in class_topics|{"CLASSBONUSES"}:
                            assert re.search(r"^-{4,}  -{4,}",body,re.M),keywords
                            assert all(len(line)<=79 for line in body.splitlines()),keywords
                    assert class_topics.issubset({k.split()[0] for k,_ in entries})
                    for keywords,body in entries:
                        if keywords=="$": continue
                        expected=body.strip().splitlines()[0].encode("ascii")
                        aliases=re.findall(r"'[^']*'|\"[^\"]*\"|[^\s]+",keywords)
                        for alias in aliases:
                            alias=alias.strip("'\"")
                            result=run("Wolfprobe","help "+alias,expected)
                            assert b"No help on that word" not in result,alias
                            checked+=1
                    assert checked>=40
                    run("Wolfprobe","help elemental",b"Werewolf command: ELEMENTAL")
                    run("Wolfprobe","help abilitystatus",b"Restored commands:")
                    print(f"Native legacy/class table help passed:{len(entries)} "
                          f"entries/{checked} exact topics and aliases, including "
                          "19 class tables, CLASSBONUSES, multiword spells and ARMOR versus ARMORCLASS")
                    return
                # Development preview must not convert or debit a real player.
                if roster_only:
                    before=snapshot("Wolfprobe")
                    listing=run("Wolfprobe","hybrid roster",b"171 named combinations shown")
                    rows=re.findall(r'^H\((\d+), \w+, \w+, "([^"]+)"\)$',
                                    (ROOT/"src/hybrid_roster.def").read_text(),re.M)
                    assert len(rows)==171
                    for ident,name in rows:
                        assert re.search(rb"\b"+str(200000+int(ident)).encode()
                                         +rb"\s+"+re.escape(name.encode())+rb"\s",listing),name
                    for text,needle in (
                        ("hybrid roster kuei-jin",b"18 named combinations shown"),
                        ("hybrid pair vampire werewolf",b"Bloodfang"),
                        ("hybrid pair werewolf vampire",b"Bloodfang"),
                        ("hybrid pair angel demon",b"Nephilim"),
                        ("hybrid pair vampire vampire",b"identical parents"),
                        ("hybrid pair vampire dem",b"Unknown"),
                        ("hybrid pair vampire",b"Preview only"),
                        ("hybrid pair vampire werewolf confirm",b"Preview only"),
                        ("hybrid roster vampire confirm",b"Preview only"),
                        ("help hybridroster",b"hybrid pair"),
                    ):
                        run("Wolfprobe",text,needle)
                    after=snapshot("Wolfprobe")
                    for field in ("Class","Powers","Breed","Tribes","Auspice","Stats",
                                  "Exp","Primal","Rank","HpMana","ParentGrant","CyborgCore"):
                        pattern=r"^"+field+r"\s+([^\r\n]+)"
                        assert re.findall(pattern,before,re.M)==re.findall(pattern,after,re.M),field
                    assert not re.search(r"^Hybrid",after,re.M)
                    print("Native named-roster preview passed:all171 exact ID/name rows, "
                          "filtered18, reverse-order lookup, invalid inputs, help and "
                          "unchanged saved class/progression/resources; no hybrid enrollment")
                    return
                for topic,needle in (
                    ("classes",b"19 standalone"),("cyborg",b"demolitions"),
                    ("angel",b"spiritshare"),("demon",b"offersoul"),
                    ("phoenix",b"emberbolt"),("gargoyle",b"visceratika"),
                    ("kuei-jin",b"dharm"),("deathknight",b"deathknight"),
                    ("moogle",b"moogle"),("wraith",b"wraith"),
                    ("sorcerer",b"sorccreate starter"),("chaosmage",b"colors"),
                    ("mage",b"arete"),("vampire",b"disciplines"),
                    ("werewolf",b"tribe"),("drow",b"faeriefire"),
                    ("dragon",b"dragonform"),("ninja",b"michi"),
                    ("monk",b"monk"),("highlander",b"forge"),
                ):
                    run("Sorcbooknew" if books_only else "Wolfprobe","help "+topic,needle)
                for name,selection,chaos in (
                    ("Sorcbooknew","autoclass sorcerer",False),
                    ("Chaosbooknew","autoclass chaosmage",True),
                    ("Sorcbookold","sorccreate starter",False),
                ):
                    run(name,selection,b"three ready-to-chant spells")
                    saved=snapshot(name)
                    assert re.search(r"^SorcStarter\s+1$",saved,re.M)
                    assert bool(re.search(r"^SorcStyle\s+1$",saved,re.M))==chaos
                    assert number(name,"Primal")==123
                    assert saved.count("#OBJECT")==7
                    speech=run(name,"cast ventriloquate pen a quiet hello",b"illusory voice")
                    assert b"a quiet hello" in speech
                    run(name,"sorccreate starter",b"already received")
                    run(name,"hold primer",b"clutch")
                    run(name,"read primer",b"Getting started")
                    run(name,"turn primer 4",b"flip")
                    run(name,"chant self",b"protective ward")
                    saved=snapshot(name)
                    assert "green sorcery" in saved if not chaos else "sorcery" in saved
                    if chaos:
                        assert re.search(r"^Spectype\s+2048$",saved,re.M)
                    # Verify native nested-object loading, not just text serialization.
                    run(name,"quit",b"Lewis Carroll")
                    clients[name].close()
                    client=socket.create_connection(("127.0.0.1",port),timeout=2)
                    clients[name]=client; receive(client); command(client,name,b"password")
                    client.sendall(password.encode("ascii")+b"\r\n"); receive(client)
                    command(client,"",b"Welcome to Godwars: Genesis")
                    run(name,"sorccreate starter",b"already received")
                    run(name,"turn primer 0",b"flip")
                    listing=run(name,"read primer",b"Writing your own spells")
                    assert b"Spark" in listing and b"Mend" in listing and b"Minor Ward" in listing
                if books_only:
                    assert proc.poll() is None
                    print("Native book mage regression passed: selection, one-time kit, three-page spell inventory, ward effects, serialized Chaos marker and relogin")
                    return
                original_class = number("Wolfprobe", "Class")
                original_exp = number("Wolfprobe", "Exp")
                run("Wolfprobe", "hybrid", b"19 planned parents; 171")
                run("Wolfprobe", "hybrid demon", b"leech")
                run("Wolfprobe", "hybrid angel", b"guardianangel")
                # Match Cyborg's final row, not a common footer possibly left
                # in the socket after an earlier large parent preview.
                cyborg_preview = run("Wolfprobe", "hybrid cyborg", b"darkvision")
                assert b"nanite_factory" in cyborg_preview and b"stuntubes" in cyborg_preview
                assert b"effect not ported" not in cyborg_preview
                assert b"standalone effect; hybrid blocked" in cyborg_preview
                run("Wolfprobe", "hybrid demon confirm", b"Preview only")
                run("Wolfprobe", "hybrid dem", b"Unknown parent")
                assert number("Wolfprobe", "Class") == original_class
                assert number("Wolfprobe", "Exp") == original_exp
                assert "CyborgCore 1 1 1 100 96" in snapshot("Wolfprobe")
                assert "CyborgCore" not in snapshot("Phoenixprobe")
                run("Wolfprobe", "quit", b"Carroll")
                clients["Wolfprobe"].close()
                client = socket.create_connection(("127.0.0.1", port), timeout=2)
                clients["Wolfprobe"] = client
                receive(client)
                command(client, "Wolfprobe", b"password")
                client.sendall(password.encode("ascii") + b"\r\n")
                receive(client)
                run("Wolfprobe", "", b"Welcome to Godwars: Genesis")
                assert "CyborgCore 1 1 1 100 96" in snapshot("Wolfprobe")
                assert number("Wolfprobe", "Class") == original_class

                # Standalone Phoenix registration/training and persisted buffs.
                for name, kind in (("Cybernew", "cyborg"), ("Angelnew", "angel"),
                                   ("Demonnew", "demon")):
                    run(name, "autoclass " + kind, b"new class is active")
                    assert number(name, "Class") == constant("CLASS_" + kind.upper())
                    run(name, kind + " train", b"training advances" if kind == "cyborg"
                        else b"class mastery advances")
                for component in ("liver", "stomach", "neuroprocessor", "battery",
                                  "nanite_factory", "left_arm", "right_arm", "eyes",
                                  "teleoptics", "railgun", "taser"):
                    run("Cybernew", "cyborg install " + component, b"modification complete")
                run("Cybernew", "cyborg convert 1000", b"converted to nanites")
                run("Cybernew", "cyborg bionics 50000", b"bionic training power")
                for system in ("limb_implants", "body_implants", "face_implants"):
                    run("Cybernew", "cyborg " + system + " improve", b"training advances")
                run("Cybernew", "cyborg demolitions improve", b"training advances")
                saved = snapshot("Cybernew")
                assert "CyborgRuntime 1 1000 1 0" in saved
                run("Cybernew", "cyborg taser Cybernew", b"not a legal")
                assert "CyborgRuntime 1 1000 1 0" in snapshot("Cybernew")
                assert "CyborgSystems 1 1 1 1 1 0 0 12500" in snapshot("Cybernew")
                for name in ("Cybernew", "Angelnew", "Demonnew"):
                    run(name, "quit", b"Carroll")
                    clients[name].close()
                    client = socket.create_connection(("127.0.0.1", port), timeout=2)
                    clients[name] = client
                    receive(client)
                    command(client, name, b"password")
                    client.sendall(password.encode("ascii") + b"\r\n")
                    receive(client)
                    run(name, "", b"Welcome to Godwars: Genesis")
                assert "CyborgRuntime 1 1000 1 0" in snapshot("Cybernew")
                assert number("Angelnew", "Powers") == 1
                assert number("Demonnew", "Powers") == 1
                run("Angelnew", "angel aura", b"power takes effect")
                assert "'parent ward'" in snapshot("Angelnew")
                run("Adminprobe", "clearclass Cyberclear", b"Ok.")
                assert number("Cyberclear", "Class") == 0
                assert "CyborgCore" not in snapshot("Cyberclear")
                assert "CyborgRuntime" not in snapshot("Cyberclear")
                # Real login persistence for derived traits and physical powers.
                for name, kind, power in (("Angelkit", "angel", "feet"),
                                          ("Demonkit", "demon", "hooves")):
                    assert "'parent traits'" in snapshot(name)
                    original = number(name, "Damroll")
                    run(name, kind + " " + power, b"power takes effect")
                    assert number(name, "Damroll") == original + 8
                    assert "'parent feet'" in snapshot(name)
                    run(name, "quit", b"Carroll")
                    clients[name].close()
                    client = socket.create_connection(("127.0.0.1", port), timeout=2)
                    clients[name] = client
                    receive(client)
                    command(client, name, b"password")
                    client.sendall(password.encode("ascii") + b"\r\n")
                    receive(client)
                    run(name, "", b"Welcome to Godwars: Genesis")
                    assert number(name, "Damroll") == original + 8
                    assert number(name, "Stats") == 5
                    assert snapshot(name).count("'parent traits'") == 3
                    run("Adminprobe", "slay " + name, b"in cold blood")
                    assert "'parent feet'" not in snapshot(name)
                    assert snapshot(name).count("'parent traits'") == 3
                    assert number(name, "Class") == constant("CLASS_" + kind.upper())
                    assert 0 < number(name, "HpMana") < number(name, "HpMana", 1)
                    run("Adminprobe", "clearclass " + name, b"Ok.")
                    assert number(name, "Class") == 0
                    assert "'parent feet'" not in snapshot(name)
                    assert "'parent traits'" not in snapshot(name)
                print("Expanded Angel/Demon: physical effects survive real login, passive nonstacking, real death/passive restoration and clearclass cleanup passed")
                run("Phoenixnew", "autoclass phoenix", b"now a Phoenix")
                run("Phoenixnew", "phoenix train", b"mastery advances")
                assert number("Phoenixnew", "Class") == constant("CLASS_PHOENIX")
                assert number("Phoenixnew", "Powers") == 1
                run("Phoenixnew", "phoenix ward", b"not mastered")
                run("Phoenixprobe", "phoenix", b"mastery: 8/8")
                original_armor = number("Phoenixprobe", "Armor")
                run("Phoenixprobe", "phoenix ward", b"feather ward protects")
                assert number("Phoenixprobe", "Armor") == original_armor + 20
                run("Phoenixprobe", "phoenix ward", b"still recovering")
                assert number("Phoenixprobe", "Armor") == original_armor + 20
                saved = snapshot("Phoenixprobe")
                assert "'phoenix ward'" in saved
                assert "'phoenix recovery'" in saved
                run("Phoenixprobe", "quit", b"Carroll")
                clients["Phoenixprobe"].close()
                client = socket.create_connection(("127.0.0.1", port), timeout=2)
                clients["Phoenixprobe"] = client
                receive(client)
                command(client, "Phoenixprobe", b"password")
                client.sendall(password.encode("ascii") + b"\r\n")
                receive(client)
                run("Phoenixprobe", "", b"Welcome to Godwars: Genesis")
                run("Phoenixprobe", "phoenix", b"mastery: 8/8")
                assert number("Phoenixprobe", "Class") == constant("CLASS_PHOENIX")
                assert number("Phoenixprobe", "Powers") == 8
                assert number("Phoenixprobe", "Armor") == original_armor + 20

                # Both classes retain the same historical ward spelling.
                run("Wolfprobe", "ward", b"ward yourself")
                assert number("Wolfprobe", "More") & constant("MORE_SHIELDED")
                run("Wolfprobe", "ward", b"stop warding")
                assert not number("Wolfprobe", "More") & constant("MORE_SHIELDED")
                run("Mageprobe", "ward", b"How much quintessense")
                run("Wolfcub", "ward", b"require Homid 5")
                run("Adminprobe", "disable ward", b"Command disabled.")
                run("Wolfprobe", "ward", b"temporarily disabled")
                run("Mageprobe", "ward", b"temporarily disabled")
                run("Adminprobe", "disable ward", b"Command enabled.")
                run("Adminprobe", "disable mageheal", b"Command disabled.")
                run("Mageprobe", "sorcheal Nobodyhere", b"temporarily disabled")
                run("Adminprobe", "disable mageheal", b"Command enabled.")

                # Exact short names win, while ordinary abbreviations still work.
                run("Vampprobe", "step", b"Who do you wish to shadowstep")
                run("Vampprobe", "shadowstep", b"Who do you wish to shadowstep")
                run("Magenovice", "l", b"Temple of Maraden")
                run("Noneprobe", "missingability", b"Huh?")
                run("Wolfprobe", "help abilitystatus", b"Restored commands:")

                run("Magenovice", "analyze", b"require Matter 1")
                run("Mageprobe", "analyze", b"Analyze which carried object")
                run("Mageprobe", "analyze barrel", b"Object")
                run("Mageprobe","magepowers",b"Mage Powers")
                run("Mageprobe","sphere Life",b"Mage Powers")
                for text in ("brittlebones Nobodyhere -1","mutateephemera Nobodyhere junk",
                             "alterweight Nobodyhere 1x","stepsideways -1",
                             "breakdreamshell 999999999999999999999999",
                             "mageheal Mageprobe -1","betterbody Mageprobe str -1",
                             "polymorphother Nobodyhere -1","electricalchaos Nobodyhere -1",
                             "inferno -1"):
                    run("Mageprobe",text,b"valid nonnegative integer")
                run("Mageprobe", "frictioncurse Nobodyhere 0", b"aren't here")
                run("Mageprobe", "sorcheal Nobodyhere", b"aren't here")
                run("Mageprobe", "spiritsence", b"see between planes")
                run("Mageprobe", "spiritsense", b"no longer see between planes")
                run("Wolfprobe", "spiritfriend", b"call your Spirit Guardian")
                run("Vampprobe", "changeling", b"Change to look like whom")
                run("Wolfprobe", "illusionaryform", b"Who")

                menu = run("Wolfprobe", "wwpowers", b"Wendigos")
                assert b"Gift track" in menu and b"Known commands" in menu
                assert b" Aeolus Bodywrack" in menu
                assert re.search(rb"Bone Gnawers\s+Friend Infest",menu)

                run("Dragprobe", "wings", b"grow wings")
                assert number("Dragprobe", "DSkills") & constant("DRAG_WINGSOUT")
                run("Dragprobe", "wings", b"slide back")
                assert not number("Dragprobe", "DSkills") & constant("DRAG_WINGSOUT")
                run("Noneprobe", "wings", b"Huh?")
                run("Wolfcub", "invisibility", b"level 4 power")

                # Cross-plane targeting respects protection, then succeeds in PK.
                room_flags(1036)
                run("Wolfprobe", "reach Wolfcub", b"must permit summoning")
                assert number("Wolfcub", "AffectedBy") & constant("AFF_SHADOWPLANE")
                room_flags(12)
                run("Wolfprobe", "reach Noneprobe", b"must permit summoning")
                assert number("Noneprobe", "AffectedBy") & constant("AFF_SHADOWPLANE")
                run("Wolfprobe", "reach Wolfcub", b"pull")
                assert not number("Wolfcub", "AffectedBy") & constant("AFF_SHADOWPLANE")

                run("Wolfcub", "mastery Wolfprobe say hello", b"Silver Fangs 5")
                run("Wolfprobe", "mastery Wolfcub quit", b"permits movement")
                run("Wolfprobe", "mastery Wolfcub mastery Wolfprobe look",
                    b"permits movement")
                run("Wolfprobe", "mastery Wolfcub say obeyed", b"compel")
                assert b"obeyed" in receive(clients["Wolfcub"])
                run("Wolfprobe", "mastery Wolfcub", b"Syntax:")
                run("Wolfprobe", "mastery Adminprobe look", b"higher authority")

                run("Monknovice", "stunfist Wolfcub", b"Tech 7")
                room_flags(1036)
                mana = number("Monkprobe", "HpMana", 2)
                run("Monkprobe", "stunfist Wolfcub", b"prevents you from attacking")
                assert number("Monkprobe", "HpMana", 2) == mana
                room_flags(12)
                run("Monkprobe", "stunfist Wolfcub", b"punch")
                run("Adminprobe", "peace", b"Ok")
                saved = snapshot("Monkprobe")
                hp = re.search(r"^HpMana\s+(\d+) (\d+) (\d+) (\d+)", saved, re.M)
                assert hp and 94000 <= int(hp[3]) <= 96000, hp
                from reconstructed_abilities import check_reconstruction
                check_reconstruction(run, snapshot, room_flags, clients, work,
                                     password, command)
                # Run Fear after other combat scenarios: a feared caster may
                # legitimately flee during a later combat pulse.
                room_flags(1036)
                run("Wolfprobe", "wrath", b"start to howl")
                assert not number("Vampprobe", "AffectedBy") & constant("AFF_FEAR")
                room_flags(12)
                run("Wolfprobe", "wrath", b"start to howl")
                assert number("Vampprobe", "AffectedBy") & constant("AFF_FEAR")
                run("Vampprobe", "shadowplay", b"deep into the shadows")
                assert number("Vampprobe", "AffectedBy") & constant("AFF_HIDE")
                run("Wolfprobe", "invisibility", b"fade out")
                assert number("Wolfprobe", "AffectedBy") & constant("AFF_INVISIBLE")
                assert proc.poll() is None
            except BaseException:
                log.flush()
                print(log_path.read_text()[-6000:])
                raise
            finally:
                for client in clients.values():
                    client.close()
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
        log_text = log_path.read_text()
        if "AddressSanitizer" in log_text or "runtime error:" in log_text:
            raise AssertionError(log_text[-6000:])
    print("Class ability regressions passed: routing, effects, costs, prerequisites, PK protection")


if __name__ == "__main__":
    main()
