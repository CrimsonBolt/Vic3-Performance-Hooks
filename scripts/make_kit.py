#!/usr/bin/env python3
"""make_kit.py - version-driven kit packaging for vic3hook.

Pipeline: game exe -> resolve_targets.py (RTTI walk) -> targets/<ver>.tsv
-> stamped dist kit folder + zip. One command per game version; nothing is
hardcoded per build in the DLL (it also carries a built-in 1.14.5 + 1.13.11
table as fallback when the sidecar file is absent).

Usage:
  python scripts/make_kit.py --game-version 1.13.11 --exe "<path>/victoria3.exe" [--kit-rev 1]

Output: dist/vic3hook-<ver>/ (runnable kit incl. vic3hook.targets sidecar)
        dist/Vic3-Performance-Hooks-<ver>.zip (release asset)
"""
import argparse, re, subprocess, shutil, sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
HOOKS = ["PrepareNavyAIData", "UpdateBuildingSpendingPriorities_Stage2",
         "UpdateAI", "HandleAutonomousInvestment", "UpdateSpendingVariables",
         "RecalculateModifierNodes", "ProcessEconomicMigration",
         "UpdateAICountryGoalsAdjustments"]

ROW = re.compile(r"^(\S+)\s+group [AB]\s+run rva (0x[0-9a-fA-F]+)\s+\{ ([0-9a-fxA-F, ]+) \}")

def resolve(exe: Path):
    out = subprocess.run([sys.executable, str(REPO / "scripts" / "resolve_targets.py"), str(exe)],
                         capture_output=True, text=True, check=True).stdout
    rows = {}
    for line in out.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        name, rva, bytes_ = m.group(1), int(m.group(2), 16), m.group(3)
        pro = bytes(int(b, 16) for b in bytes_.replace("0x", "").replace(" ", "").split(",") if b)
        if len(pro) >= 16:
            rows[name] = (rva, pro[:16])   # first resolver row per name wins
    missing = [h for h in HOOKS if h not in rows]
    if missing:
        sys.exit(f"resolver did not produce a row for: {missing}")
    return rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-version", required=True, help="e.g. 1.13.11")
    ap.add_argument("--exe", required=True, type=Path)
    ap.add_argument("--kit-rev", type=int, default=1)
    ap.add_argument("--skip-resolve", action="store_true",
                    help="reuse targets/<ver>.tsv if it already exists")
    a = ap.parse_args()

    tsv_path = REPO / "targets" / f"{a.game_version}.tsv"
    rows = None
    if a.skip_resolve and tsv_path.exists():
        print(f"using existing {tsv_path}")
    else:
        if not a.exe.exists():
            sys.exit(f"exe not found: {a.exe}")
        print(f"resolving targets from {a.exe} ...")
        rows = resolve(a.exe)
        tsv_path.parent.mkdir(exist_ok=True)
        with open(tsv_path, "w", newline="") as f:
            f.write(f"# vic3hook target table - game build {a.game_version}\n")
            f.write("# columns: hook_name<TAB>rva<TAB>prologue_hex(16 bytes)\n")
            for h in HOOKS:
                rva, pro = rows[h]
                f.write(f"{h}\t{rva:#010x}\t{pro.hex()}\n")
        print(f"wrote {tsv_path}")

    kit_ver = f"{a.game_version}.{a.kit_rev}"
    dist = REPO / "dist"
    kit = dist / f"vic3hook-{a.game_version}"
    if kit.exists():
        shutil.rmtree(kit)
    kit.mkdir(parents=True)
    dll = REPO / "build" / "Release" / "vic3hook.dll"
    if not dll.exists():
        sys.exit("build/Release/vic3hook.dll missing - build first")
    shutil.copy(dll, kit / "vic3hook.dll")
    shutil.copy(tsv_path, kit / "vic3hook.targets")
    for f in ["inject_running.ps1", "INJECT HOOKS.cmd", "MINHOOK-LICENSE.txt"]:
        src = (REPO / "kit" / f)
        if not src.exists():
            sys.exit(f"kit template missing: {src}")
        shutil.copy(src, kit / f)
    ini = (REPO / "kit" / "vic3hook.ini").read_text(encoding="utf-8")
    ini = re.sub(r"^;.*", f"; vic3hook cadence config (Victoria 3 {a.game_version}) - kit {kit_ver}",
                 ini, count=1)
    (kit / "vic3hook.ini").write_text(ini, encoding="utf-8", newline="")
    (kit / "version.txt").write_text(kit_ver + "\n", encoding="utf-8", newline="")

    zip_path = dist / f"Vic3-Performance-Hooks-{a.game_version}.zip"
    if zip_path.exists():
        zip_path.unlink()
    shutil.make_archive(str(dist / f"Vic3-Performance-Hooks-{a.game_version}"), "zip", str(kit))
    print(f"kit:   {kit}")
    print(f"zip:   {zip_path} ({zip_path.stat().st_size} bytes)")
    print(f"release hint: tag {kit_ver}, upload the zip; set make_latest=false "
          f"if a newer game-version line should stay 'latest'.")

if __name__ == "__main__":
    main()
