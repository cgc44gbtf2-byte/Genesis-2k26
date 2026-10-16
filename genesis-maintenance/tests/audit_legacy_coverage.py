"""Compiler-active inventory. A test mention NEVER establishes functionality."""
from pathlib import Path
import collections
import csv
import json
import re
import subprocess

root=Path(__file__).resolve().parents[1]
files=["disc","clan","werewolf","wwbreed","wwauspice","wwtribe","dragon",
       "drow","elemental","highland","mage","monk","ninja","sorcerer",
       "magic","reconstructed","more"]
# more.c mixes ordinary engine commands with class powers. Include only the
# named class helpers here; do not present its administrative/object commands
# as class abilities. Preprocessing below still proves these definitions active.
more_class_helpers=set("""
do_mindblast do_malleable do_fleshcraft do_bonecraft do_zuloform do_plasmaform
do_conditioning do_tongue do_mummify do_controlbeast do_shroudnight
do_silencedeath do_weakness do_infirmity do_bloodagony do_spit do_shadowbody
do_arms do_lamprey do_thanatopsis do_spiritgate do_preservation do_zombie
do_spiritform do_spiritguardian do_succulence do_frenzy do_shadowplay
do_shadowstep do_spiriteyes do_sbreath do_celerity do_chiroptean do_empathy
""".split())
token=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*')
def masked(s,strings=True):
    def sub(m):
        t=m[0]
        return re.sub(r"[^\n]"," ",t) if strings or t.startswith("/") else t
    return token.sub(sub,s)

interp=masked((root/"src/interp.c").read_text(),False)
aliases=collections.defaultdict(list)
for name,handler in re.findall(r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',interp):
    aliases[handler].append(name)
const=masked((root/"src/const.c").read_text(),False)
spells=collections.defaultdict(list)
for name,handler in re.findall(r'\{\s*"([^"]+)"\s*,[^,]+,\s*(spell_\w+)\s*,',const):
    spells[handler].append(name)
tests={p.name:p.read_text() for p in (root/"tests").iterdir() if p.suffix in (".c",".py")}
rows=[]
for stem in files:
    path=root/"src"/(stem+".c")
    result=subprocess.run(["cc","-E","-DMERC","-Dunix","-DMEMWATCH",str(path)],
                          capture_output=True,text=True,check=True)
    code=masked(result.stdout)
    for m in re.finditer(r'\bvoid\s+((?:do_|spell_)\w+)\s*\([^;{}]*\)\s*\{',code):
        name=m[1]
        if stem=="more" and name not in more_class_helpers: continue
        start=m.end(); depth=1; end=start
        while depth and end<len(code):
            if code[end]=="{": depth+=1
            elif code[end]=="}": depth-=1
            end+=1
        body=code[start:end-1]
        direct=[p for p,text in tests.items() if re.search(r"\b"+re.escape(name)+r"\b",text)]
        names=aliases[name]+spells[name]
        mentions=[p for p,text in tests.items()
                  if any(re.search(r'["\']'+re.escape(a)+r'(?:[ "\'])',text) for a in names)]
        noop=re.sub(r"\s","",body) in ("","return;")
        rows.append({"source":str(path.relative_to(root)),"handler":name,
            "commands_or_spells":"; ".join(names),
            "compiler_active":"yes","empty_body":"yes" if noop else "no",
            "test_mentions_not_proof":"; ".join(sorted(set(direct+mentions))),
            "variant_effect_verification":"requires explicit per-variant effect assertions"})
assert {r["handler"] for r in rows if r["source"]=="src/more.c"}==more_class_helpers
with (root/"LEGACY_COVERAGE.csv").open("w",newline="") as fp:
    writer=csv.DictWriter(fp,fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
summary={"active_handlers":len(rows),"by_source":dict(collections.Counter(r["source"] for r in rows)),
    "empty_bodies":[r["handler"] for r in rows if r["empty_body"]=="yes"],
    "without_direct_registration":[r["handler"] for r in rows if not r["commands_or_spells"]],
    "without_test_mentions":[r["handler"] for r in rows if not r["test_mentions_not_proof"]],
    "scope":"Compiler-active inventory and test-mention cross-reference; NOT functional certification."}
(root/"LEGACY_COVERAGE_SUMMARY.json").write_text(json.dumps(summary,indent=2)+"\n")
print(json.dumps(summary,indent=2))
