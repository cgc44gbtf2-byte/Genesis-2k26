"""Additional isolated checks invoked by class_abilities.py."""
import re
import time

from smoke_runtime import receive


def check_reconstruction(run, snapshot, room_flags, clients, work, password, command):
    def affected(name, skill):
        return "AffectData   '" + skill + "'" in snapshot(name)

    def peace():
        run("Adminprobe", "peace", b"Ok")

    def logout(name):
        # Combat timers are real game rules; do not bypass them in the server.
        deadline = time.monotonic() + 45
        while True:
            result = run(name, "quit", (b"brillig", b"fight timer"))
            if b"fight timer" not in result.lower():
                return
            if time.monotonic() > deadline:
                raise AssertionError("Combat timer did not expire")
            time.sleep(1)

    def land(caster, text, target, skill, success):
        # The legacy saving throw always has a minimum resistance chance.
        # Exercise legitimate resistance without making a single random roll
        # the pass/fail criterion. With fixture WIS 1 it is 14% per attempt.
        for _ in range(16):
            result = run(caster, text, (success, b"resists your gift"))
            peace()
            if success.lower() in result.lower():
                assert affected(target, skill), skill
                return
        raise AssertionError("Gift resisted in every bounded attempt: " + skill)

    run("Wolfcub", "disquiet Wolfprobe", b"Homid 4")
    run("Wolfcub", "quicksand", b"Red Talons 4")
    run("Wolfcub", "lycaon Wolfprobe", b"Red Talons 5")
    run("Wolfcub", "totemgift", b"Metis 5")
    run("Wolfcub", "scream", b"Get of Fenris 4")
    run("Noneprobe", "horridreality Wolfprobe", b"Huh?")
    run("Noneprobe", "maskempathy", b"Huh?")
    run("Drywolf", "disquiet Vampprobe", b"not have enough mana")
    run("Drywolf", "lycaon Vampprobe", b"not have enough mana")
    run("Drywolf", "totemgift", b"not have enough mana")
    run("Drywolf", "scream", b"not have enough mana")
    run("Dryvamp", "horridreality Drywolf", b"not have enough mana")
    run("Dryvamp", "maskempathy", b"not have enough mana")

    room_flags(1036)
    run("Wolfprobe", "disquiet Wolfcub", b"protected")
    run("Wolfprobe", "lycaon Wolfcub", b"protected")
    run("Wolfprobe", "scream", b"safe room")
    run("Vampprobe", "horridreality Wolfcub", b"protected")
    room_flags(12)
    run("Wolfprobe", "quicksand", b"solid ground")
    run("Wolfprobe", "lycaon Dragprobe", b"existing form")

    run("Wolfprobe", "totemgift", b"protective spirit totem")
    assert affected("Wolfprobe", "totemgift")
    run("Wolfprobe", "totemgift", b"already present")
    run("Wolfprobe", "totemgift dismiss", b"dismiss")
    assert not affected("Wolfprobe", "totemgift")
    run("Wolfprobe", "totemgift", b"protective spirit totem")

    run("Vampprobe", "maskempathy", b"emotional mask")
    assert affected("Vampprobe", "maskempathy")
    run("Mageprobe", "readaura Vampprobe", b"veiled")
    run("Wolfprobe", "disquiet Vampprobe", b"resists your gift")
    peace()
    assert not affected("Vampprobe", "disquiet")
    run("Vampprobe", "maskempathy", b"lower your emotional mask")
    assert not affected("Vampprobe", "maskempathy")

    land("Wolfprobe", "disquiet Wolfcub", "Wolfcub", "disquiet", b"shatter")
    run("Wolfprobe", "disquiet Wolfcub", b"already affected")
    land("Vampprobe", "horridreality Wolfcub", "Wolfcub", "horridreality", b"trap")
    run("Vampprobe", "horridreality Wolfcub", b"already affected")

    land("Wolfprobe", "lycaon Wolfcub", "Wolfcub", "lycaon", b"bind")
    cursed = snapshot("Wolfcub")
    assert re.search(r"^Morph\s+a spirit-cursed wolf~", cursed, re.M), cursed[-1500:]
    assert re.search(r"^Class\s+2$", cursed, re.M)
    run("Wolfcub", "morph", b"cannot change form")
    run("Wolfcub", "beastform", b"cannot change form")
    run("Wolfcub", "mummify", b"cannot change form")
    run("Wolfcub", "spiritform", b"cannot change form")
    run("Wolfcub", "formillusion", b"cannot change form")
    run("Wolfprobe", "lycaon Wolfcub", b"already affected")

    # Real save/reload must retain the curse and later remove its form bit,
    # polymorph flag, appearance and stat penalties together.
    logout("Wolfcub")
    clients["Wolfcub"].close()
    import socket
    endpoint = clients["Wolfprobe"].getpeername()
    client = socket.create_connection(endpoint, timeout=2)
    clients["Wolfcub"] = client
    receive(client)
    command(client, "Wolfcub", b"password")
    client.sendall(password.encode("ascii") + b"\r\n")
    receive(client)
    command(client, "", b"Welcome to Godwars: Genesis")
    assert affected("Wolfcub", "lycaon")
    run("Adminprobe", "cast 'remove curse' Wolfcub", b"Ok")
    cured = snapshot("Wolfcub")
    assert "AffectData   'lycaon'" not in cured
    assert not re.search(r"^Morph\s+a spirit-cursed wolf~", cured, re.M)
    poly = re.search(r"^Polyaff\s+(\d+)", cured, re.M)
    assert not poly or int(poly[1]) == 0

    # Area spells have one charge per cast, skip protected targets, and share
    # a persisted cooldown. Check a valid result even if an individual saves.
    clients["Adminprobe"].sendall(b"rset 3001 sector 2\r\n")
    run("Adminprobe", "look", b"Temple of Maraden")
    room_flags(4)
    run("Drywolf", "quicksand", b"not have enough mana")
    run("Wolfprobe", "quicksand", b"spirit quicksand")
    peace()
    assert affected("Wolfprobe", "gift recovery")
    assert not affected("Noneprobe", "quicksand")  # non-PK, opposite plane
    assert not affected("Adminprobe", "quicksand")
    run("Wolfprobe", "scream", b"recovering")
    # Self-cast removes the effect deterministically through the real handler.
    run("Wolfprobe", "cast 'dispel magic' self", b"remove all magical")
    assert not affected("Wolfprobe", "gift recovery")
    run("Wolfprobe", "scream", b"Scream of Gaia")
    peace()
    assert affected("Wolfprobe", "gift recovery")
    run("Wolfprobe", "quicksand", b"recovering")
    run("Vampprobe", "maskempathy", b"emotional mask")
    land("Wolfprobe", "lycaon Wolfcub", "Wolfcub", "lycaon", b"bind")

    # Natural expiry: shorten only disposable serialized records, reload, and
    # wait for the server's normal character-update tick (no production hook).
    peace()
    for name in ("Wolfprobe", "Vampprobe", "Wolfcub"):
        snapshot(name)
        logout(name)
        clients[name].close()
        path = work / "player" / name
        record = path.read_text()
        record = re.sub(
            r"^(AffectData\s+'(?:totemgift|maskempathy|disquiet|horridreality|"
            r"gift recovery|scream|quicksand|lycaon)')\s+-?\d+",
            r"\1 0", record, flags=re.M)
        path.write_text(record)
        client = socket.create_connection(endpoint, timeout=2)
        clients[name] = client
        receive(client)
        command(client, name, b"password")
        client.sendall(password.encode("ascii") + b"\r\n")
        receive(client)
        command(client, "", b"Welcome to Godwars: Genesis")
    deadline = time.monotonic() + 75
    names = ("totemgift", "maskempathy", "disquiet", "horridreality",
             "gift recovery", "scream", "quicksand", "lycaon")
    while True:
        records = [snapshot(name) for name in ("Wolfprobe", "Vampprobe", "Wolfcub")]
        if not any("AffectData   '" + skill + "'" in text
                   for text in records for skill in names):
            break
        if time.monotonic() > deadline:
            raise AssertionError("Reconstructed effects did not expire")
        time.sleep(1)
    expired = snapshot("Wolfcub")
    poly = re.search(r"^Polyaff\s+(\d+)", expired, re.M)
    assert not poly or int(poly[1]) == 0
    assert not re.search(r"^Morph\s+a spirit-cursed wolf~", expired, re.M)
    assert re.search(r"^Class\s+2$", expired, re.M)
    clients["Adminprobe"].sendall(b"rset 3001 sector 0\r\n")
    run("Adminprobe", "look", b"Temple of Maraden")
    room_flags(12)
    print("Reconstructed gifts passed: prerequisites, safety, effects, cooldown, reload, cure, expiry")
