#!/usr/bin/env python3
"""Find crossed skill families in native getters/arrays.

This is a source wiring check, not proof of every command's runtime effect.
Runtime progression benefits are asserted by test_class_benefits and the
existing class/spell/variant regression suites.
"""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
FAMILIES = {
    "tribe": "TRIBE_", "breed": "BREED_", "auspice": "AUSPICE_",
    "disc": "DISC_", "monk": "MONK_", "ninja": "NINJA_", "highland": "HIGH_",
}
ARRAYS = {
    "tribes": "TRIBE_", "breed": "BREED_", "auspice": "AUSPICE_",
    "monk": "MONK_", "ninja": "NINJA_", "highland": "HIGH_",
}
GETTER_LIMITS = {
    "ninja": {"NINJA_SORA", "NINJA_CHIKYU", "NINJA_NINGENNO"},
    "monk": {"MONK_MANTRA", "MONK_TECH", "MONK_ART"},
    "highland": {"HIGH_WEAPON", "HIGH_ATTACK", "HIGH_DEFENSE", "HIGH_ABILITY"},
}
LEXEMES = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                     r"'(?:\\.|[^'\\])*'", re.S)
GETTERS = re.compile(
    r"\bget_(tribe|breed|auspice|disc|monk|ninja|highland)\s*"
    r"\([^,\n]+,\s*([A-Z][A-Z0-9_]+)\s*\)"
)
SLOTS = re.compile(
    r"(?:->|\.)\s*(tribes|breed|auspice|monk|ninja|highland)"
    r"\s*\[\s*([A-Z][A-Z0-9_]+)\s*\]"
)


def check(text):
    masked = LEXEMES.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), text)
    issues = []
    count = 0
    for pattern, families in ((GETTERS, FAMILIES), (SLOTS, ARRAYS)):
        for match in pattern.finditer(masked):
            family, constant = match.groups()
            count += 1
            wrong = not constant.startswith(families[family])
            if pattern is GETTERS and family in GETTER_LIMITS:
                wrong |= constant not in GETTER_LIMITS[family]
            if wrong:
                issues.append({
                    "line": masked.count("\n", 0, match.start()) + 1,
                    "reference": match.group(), "expected_family": families[family],
                })
    return count, issues


def main():
    # Keep the scanner itself sensitive to both regressions fixed in this pass.
    assert len(check("get_tribe(ch,AUSPICE_PHILODOX);")[1]) == 1
    assert len(check("get_disc(ch,AUSPICE_THEURGE);")[1]) == 1
    assert len(check("get_ninja(ch,NINJA_KI);")[1]) == 1
    assert len(check("pc->tribes[AUSPICE_PHILODOX];")[1]) == 1
    assert not check('/* get_disc(ch,AUSPICE_THEURGE); */\n'
                     '"get_tribe(ch,AUSPICE_PHILODOX)";')[1]
    files = references = 0
    issues = []
    for path in sorted((ROOT / "src").iterdir()):
        if path.suffix not in {".c", ".inc"}:
            continue
        files += 1
        count, found = check(path.read_text())
        references += count
        issues.extend({"file": str(path.relative_to(ROOT)), **entry} for entry in found)
    print(json.dumps({
        "source_files_checked": files, "constant_skill_references_checked": references,
        "wrong_family_or_getter_slot": issues,
        "scope": "Literal native family references, not dynamic indices or runtime effect certification",
    }, indent=2))
    raise SystemExit(bool(issues))


if __name__ == "__main__":
    main()
