#!/usr/bin/env python3
"""Run fresh engine processes for 50 paired seeds; never touch a live host."""
import argparse
import csv
import json
import os
from pathlib import Path
import re
import statistics
import subprocess
import tempfile

RESULT = re.compile(
    r"^DUEL_RESULT seed=(\d+) opener=(cyborg|wraith) cyborg_hp=(-?\d+) "
    r"wraith_hp=(-?\d+) pulses=(\d+) cyborg_actions=(\d+) wraith_actions=(\d+)$",
    re.M,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    output = args.output.resolve() if args.output else None
    if output:
        output.mkdir(parents=True, exist_ok=True)
    rows = []
    initial = {}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=0"
    env["UBSAN_OPTIONS"] = "halt_on_error=1"
    # Fresh process + disposable working directory isolates global engine state.
    # Paired seeds hold the RNG initialization constant while reversing priority.
    with tempfile.TemporaryDirectory(prefix="genesis-duel-matrix-") as work:
        for index in range(50):
            seed = 1792094400 + 7919 * index
            for opener in ("cyborg", "wraith"):
                run = subprocess.run(
                    [str(binary), str(seed), opener], cwd=work, env=env,
                    capture_output=True, text=True, timeout=30,
                )
                trace = run.stdout
                if output:
                    (output / f"{seed}-{opener}.txt").write_text(trace)
                if run.returncode or "runtime error:" in run.stderr or "ERROR: AddressSanitizer" in run.stderr:
                    raise AssertionError(
                        f"seed={seed} opener={opener} exit={run.returncode}: {run.stderr[-2000:]}"
                    )
                match = RESULT.search(trace)
                assert match, f"Missing result for {seed}/{opener}"
                values = match.groups()
                row = dict(zip(
                    ("seed", "opener", "cyborg_hp", "wraith_hp", "pulses",
                     "cyborg_actions", "wraith_actions"), values,
                ))
                for key in row:
                    if key != "opener":
                        row[key] = int(row[key])
                assert row["seed"] == seed and row["opener"] == opener
                row["seconds"] = row["pulses"] / 4
                row["winner"] = (
                    "cyborg" if row["cyborg_hp"] > 0 and row["wraith_hp"] <= 0
                    else "wraith" if row["wraith_hp"] > 0 and row["cyborg_hp"] <= 0
                    else "unresolved"
                )
                assert row["cyborg_actions"] > 0 and row["wraith_actions"] > 0
                rows.append(row)
                for name in ("Cyborg", "Wraith"):
                    line = re.search(rf"^{name}: HP 100000/100000;.*$", trace, re.M)
                    assert line, f"Missing initial stats for {name}"
                    if name in initial:
                        assert initial[name] == line.group(), "Starting build changed across seeds"
                    initial[name] = line.group()
    groups = {}
    for opener in ("cyborg", "wraith", "all"):
        group = [r for r in rows if opener == "all" or r["opener"] == opener]
        groups[opener] = {
            "fights": len(group),
            "cyborg_wins": sum(r["winner"] == "cyborg" for r in group),
            "wraith_wins": sum(r["winner"] == "wraith" for r in group),
            "unresolved": sum(r["winner"] == "unresolved" for r in group),
            "mean_seconds": statistics.mean(r["seconds"] for r in group),
            "min_seconds": min(r["seconds"] for r in group),
            "max_seconds": max(r["seconds"] for r in group),
            "mean_cyborg_hp": statistics.mean(max(0, r["cyborg_hp"]) for r in group),
            "min_cyborg_hp": min(max(0, r["cyborg_hp"]) for r in group),
            "max_cyborg_hp": max(max(0, r["cyborg_hp"]) for r in group),
            "mean_wraith_hp": statistics.mean(max(0, r["wraith_hp"]) for r in group),
        }
    assert len(rows) == 100 and len({(r["seed"], r["opener"]) for r in rows}) == 100
    report = {"initial_stats": initial, "groups": groups, "fights": rows}
    if output:
        (output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
        with (output / "results.csv").open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    print(json.dumps({"initial_stats": initial, "groups": groups}, indent=2))
    print("PASS: 100 fresh-process fights, 50 paired seeds, both openers; no balance changes.")


if __name__ == "__main__":
    main()
