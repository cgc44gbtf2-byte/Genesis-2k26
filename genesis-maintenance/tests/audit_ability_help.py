#!/usr/bin/env python3
"""Exact help reachability inventory, NOT a claim of effect verification.

Only files loaded by area.lst count. Never treat spell_null affect identities
as missing active spells. --strict fails while actionable help topics remain.
Reports are generated inventories, not hand-maintained success assertions.
"""
import argparse
import csv
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def help_entries():
    result = {}
    for filename in (ROOT / "area/area.lst").read_text().splitlines():
        if not filename.endswith(".hlp"):
            continue
        text = (ROOT / "area" / filename).read_text()
        for match in re.finditer(r"^\s*(-?\d+)\s+([^~]+)~\n(.*?)~", text, re.M | re.S):
            level, keywords, body = match.groups()
            if keywords.strip() == "$":
                continue
            for topic in re.findall(r"'[^']*'|\"[^\"]*\"|[^\s]+", keywords):
                topic = topic.strip("'\"").casefold()
                result.setdefault(topic, []).append((int(level), filename, body.strip()))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    helps = help_entries()
    rows = []
    for row in csv.DictReader((ROOT / "LEGACY_COVERAGE.csv").open()):
        if row["handler"] == "spell_null":
            continue
        for topic in filter(None, (x.strip() for x in row["commands_or_spells"].split(";"))):
            lookup = ("spell " + topic if row["handler"].startswith("spell_") else topic)
            records = [record for record in helps.get(lookup.casefold(), []) if record[0] <= 0]
            rows.append({
                "source": row["source"], "handler": row["handler"], "topic": lookup,
                "exact_public_help": "yes" if records else "no",
                "first_exact_file": records[0][1] if records else "",
                "effect_verification": "separate positive/negative variant assertions required",
            })
    missing = [row for row in rows if row["exact_public_help"] == "no"]
    with (ROOT / "ABILITY_HELP_COVERAGE.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    summary = {
        "registered_legacy_topics": len(rows),
        "exact_public_help": len(rows) - len(missing),
        "missing_exact_public_help": len(missing),
        "excluded": "spell_null internal affect identities; unrelated engine commands",
        "warning": "Reachable documentation is not functional effect/variant certification.",
    }
    (ROOT / "ABILITY_HELP_SUMMARY.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if args.strict and missing:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
