#!/usr/bin/env python3
"""Exercise a disposable character against the compiled server in isolated data."""
import os
import ctypes
import ctypes.util
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def receive(sock):
    chunks = []
    sock.settimeout(3)
    while True:
        try:
            data = sock.recv(65536)
        except socket.timeout:
            break
        if not data:
            break
        chunks.append(data)
        sock.settimeout(0.3)
    return b"".join(chunks)


def send(sock, command, expected=None):
    sock.sendall(command.encode("ascii") + b"\r\n")
    data = receive(sock)
    if expected is not None and expected.lower() not in data.lower():
        raise AssertionError(f"Expected {expected!r}; got {data[:1000]!r}")
    return data


def main():
    with tempfile.TemporaryDirectory(prefix="genesis-smoke-") as work:
        work = Path(work)
        shutil.copytree(ROOT / "area", work / "area")
        shutil.copy2(ROOT / "run.sh", work / "run.sh")
        (work / "src").mkdir()
        (work / "src" / "merc").symlink_to(ROOT / "src" / "merc")
        # Reserve a free local port; the server needs an unprivileged port.
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=0:abort_on_error=1"
        env["UBSAN_OPTIONS"] = "print_stacktrace=1:halt_on_error=1"
        env["PORT"] = str(port)
        password = secrets.token_hex(12)
        log_path = work / "server.log"
        with log_path.open("w") as log:
            proc = subprocess.Popen(
                ["sh", str(work / "run.sh")],
                cwd=work / "area", env=env, stdout=log, stderr=subprocess.STDOUT,
            )
            try:
                for _ in range(80):
                    if proc.poll() is not None:
                        raise RuntimeError(f"Server exited during boot: {proc.returncode}")
                    try:
                        client = socket.create_connection(("127.0.0.1", port), timeout=1)
                        break
                    except OSError:
                        time.sleep(0.25)
                else:
                    raise RuntimeError("Server did not accept a connection")
                with client:
                    assert b"Godwars: Genesis" in receive(client)
                    send(client, "Maintprobe", b"tombstone")
                    send(client, "y", b"password")
                    send(client, password, b"retype password")
                    send(client, password, b"sex")
                    send(client, "m", b"ANSI")
                    send(client, "n")
                    send(client, "", b"Welcome to Godwars: Genesis")
                    for command in (
                        "look", "exits", "inventory", "equipment", "score",
                        "who", "level", "train notaskill", "channels",
                    ):
                        output = send(client, command)
                        if proc.poll() is not None or not output:
                            raise RuntimeError(f"Server stopped responding to {command!r}")
                    send(client, "save", b"must awaken")
                    send(client, "quit")
                # Sleepers deliberately are not persisted. Create an isolated,
                # awakened fixture to exercise eligible character save/load.
                crypt = ctypes.CDLL(os.environ.get("CRYPT_LIBRARY")
                                    or ctypes.util.find_library("crypt") or "libcrypt.so.1")
                crypt.crypt.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
                crypt.crypt.restype = ctypes.c_char_p
                hashed = crypt.crypt(password.encode("ascii"), b"Maintprobe")
                if not hashed:
                    raise RuntimeError("Could not hash disposable fixture password")
                player_file = work / "player" / "Maintprobe"
                player_file.write_text(
                    "#PLAYER\nName Maintprobe~\nPassword " + hashed.decode("ascii")
                    + "~\nLevel 2\nRoom 3001\nHpMana 100 100 100 100\nEnd\n#END\n"
                )
                with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                    receive(client)
                    send(client, "Maintprobe", b"password")
                    send(client, password)
                    send(client, "", b"Welcome to Godwars: Genesis")
                    send(client, "look")
                    send(client, "save", b"Saved.")
                    assert b"#END" in player_file.read_bytes()
                    assert (work / "backup" / "Maintprobe").is_file()
                    # A missing output directory must not crash the server.
                    for dirname in ("player", "backup"):
                        directory = work / dirname
                        unavailable = work / (dirname + "-unavailable")
                        directory.rename(unavailable)
                        try:
                            response = send(client, "save", b"failed")
                            assert b"Saved." not in response
                            time.sleep(0.3)
                            if proc.poll() is not None:
                                raise RuntimeError(f"Server crashed when {dirname}/ was unavailable")
                        finally:
                            unavailable.rename(directory)
                        send(client, "save", b"Saved.")
                    send(client, "quit")
                # Reload the server-generated file, not just the minimal fixture.
                with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                    receive(client)
                    send(client, "Maintprobe", b"password")
                    send(client, password)
                    send(client, "", b"Welcome to Godwars: Genesis")
                    send(client, "score")
                    send(client, "quit")
                if proc.poll() is not None:
                    raise RuntimeError("Server exited after save/reload check")
            except Exception:
                log.flush()
                print(log_path.read_text())
                raise
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
        log_text = log_path.read_text()
        if "AddressSanitizer" in log_text or "runtime error:" in log_text:
            raise AssertionError(log_text)
        print("Runtime smoke passed: new character, commands, save/reload, missing save directories, quit")


if __name__ == "__main__":
    main()
