"""Independent supplied-roster oracle; no game/player files are changed."""
import hashlib
import itertools
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SYMBOLS = ("VAMPIRE WEREWOLF SORCERER DROW HIGHLANDER MAGE DRAGON NINJA "
           "MONK ELEMENTAL ANGEL CYBORG DEMON PHOENIX GARGOYLE KUEIJIN "
           "DEATHKNIGHT MOOGLE WRAITH").split()
EXPECTED = "cec96e66aaaf44b794439d2377d9b620b2b399ec5f2481304e31aec89dcc861e"

def main():
    header=(ROOT/"src/parents.h").read_text()
    parent_ids={k:int(v) for k,v in re.findall(
        r"^#define\s+GEN_PARENT_(\w+)\s+(\d+)\s*$",header,re.M)}
    assert [parent_ids[k] for k in SYMBOLS]==list(range(1001,1020))
    rows=re.findall(r'^H\((\d+), (\w+), (\w+), "([^"]+)"\)$',
                    (ROOT/"src/hybrid_roster.def").read_text(),re.M)
    assert len(rows)==171
    assert [int(i) for i,a,b,n in rows]==list(range(1,172))
    pairs=[(parent_ids[a],parent_ids[b]) for i,a,b,n in rows]
    assert all(a<b for a,b in pairs)
    assert set(pairs)==set(itertools.combinations(range(1001,1020),2))
    assert len({n.casefold() for i,a,b,n in rows})==171
    canonical="".join(f"{200000+int(i)}|{parent_ids[a]}|{parent_ids[b]}|{n}\n"
                      for i,a,b,n in rows)
    # Hash computed independently from the uploaded list, not the C table.
    assert hashlib.sha256(canonical.encode("ascii")).hexdigest()==EXPECTED
    saved_classes={k:int(v) for k,v in re.findall(
        r"^#define\s+CLASS_(\w+)\s+(\d+)\s*$",(ROOT/"src/merc.h").read_text(),re.M)}
    old_order=("VAMPIRE WEREWOLF SORCERER DROW HIGHLANDER MAGE DRAGON NINJA "
               "MONK ELEM PHOENIX CYBORG ANGEL DEMON GARGOYLE KUEIJIN "
               "DEATHKNIGHT MOOGLE WRAITH").split()
    assert saved_classes["NONE"]==0
    assert [saved_classes[k] for k in old_order]==[1<<i for i in range(19)]
    print("Supplied roster oracle passed:171 exact ID/parent/name mappings; "
          "all19 legacy class bits and stable parent IDs unchanged")

if __name__=="__main__":
    main()
