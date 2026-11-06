# Vic3 Performance Hooks

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Game: Victoria 3](https://img.shields.io/badge/game-Victoria%203%201.13.11%20%2F%201.14.5%20%2F%201.15.0-8e44ad.svg)](#supported-game-versions)
[![Platform: Windows x64](https://img.shields.io/badge/platform-Windows%20x64-2ea44f.svg)](#quick-start)
[![Download - Releases](https://img.shields.io/badge/download-GitHub%20Releases-brightgreen.svg)](https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest)

**Reclaim late-game simulation speed in Victoria 3** by letting a handful of expensive engine tick-tasks run every Nth call instead of every call — with no changes to game files and no changes to how the game computes anything.

> [!TIP]
> ### ➤ [Download the latest release here](https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest)
> Unzip it anywhere — no build, no setup. Then follow [Quick start](#quick-start) below.
> The kit checks for updates automatically on every launch.

> [!IMPORTANT]
> **When a task runs, it runs 100% unmodified.** No number is ever computed differently — some caches and AI decisions just refresh a few days later than vanilla.

> [!NOTE]
> **Supported game versions:** Victoria 3 **1.15.0** ("Ivan-Chai", current release is a beta for it), **1.14.5** ("Ice Tea") and **1.13.11** ("Matcha"). After a game update the kit fails safe: nothing is patched and you simply stay at vanilla speed until an updated release lands.

---

## Contents

- [Quick start](#quick-start)
- [What it throttles](#what-it-throttles)
- [What to expect](#what-to-expect)
- [Results](#results)
- [Using it](#using-it)
- [Updating](#updating)
- [Tuning](#tuning)
- [Troubleshooting](#troubleshooting)
- [Supported game versions](#supported-game-versions)
- [Technical notes](#technical-notes)
- [Building from source](#building-from-source)
- [License](#license)

---

## Quick start

1. Unzip the release into a new folder — it can be placed anywhere.
2. Start Victoria 3 through Steam as usual.
3. **Load your save, then pause the game.**
4. Double-click `INJECT HOOKS.cmd`, read the window, unpause and play.

> [!WARNING]
> **Inject only while the game is loaded and paused.** Injecting while the game runs at speed can deadlock it during patching. If the game freezes right after injecting: close it via Task Manager, restart, and re-inject while paused.

> [!NOTE]
> **Going back to vanilla:** just don't run the script. Restarting the game also fully reverts.

---

## What it throttles

The current release ships all eight hooks enabled at these cadences:

| Task | What it is | Cadence |
|---|---|---|
| `UpdateAI` | country AI update | every 2nd tick |
| `UpdateBuildingSpendingPriorities_Stage2` | building spending priorities | every 2nd tick |
| `HandleAutonomousInvestment` | AI autonomous investment | every 2nd tick |
| `PrepareNavyAIData` | navy AI data prep | every 2nd tick |
| `UpdateSpendingVariables` | spending-variables refresh | every 2nd tick |
| `ProcessEconomicMigration` | pop migration processing | every 2nd tick |
| `UpdateAICountryGoalsAdjustments` | AI country-goal adjustment | every 2nd tick |
| `RecalculateModifierNodes` | modifier-graph recalculation — the single biggest late-game cost | every 5th tick |

`RecalculateModifierNodes` gets the gentlest cadence, proven across long sessions on the previous patches. If diplomatic deals ever misbehave on your setup, set it to `0` in the ini — instant passthrough, no restart.

`ProcessEconomicMigration` and `UpdateAICountryGoalsAdjustments` are safe to skip by design: migration is a queue (a skipped day just delays it, nothing is lost) and goal adjustments carry a progress cursor (a skipped call simply continues next time).

All throttled tasks run every tick in vanilla; the default profile throttles them to **every other tick** (RecalculateModifierNodes to every 5th). More aggressive values in `vic3hook.ini` buy more speed at the cost of larger gameplay differences.

**The weekly "vanilla day":** every 7th day, *all* throttled tasks run at full fidelity for that one day. The engine schedules some systems on fixed day-phases, and a fixed skip pattern can phase-lock against them — the vanilla day re-synchronizes everything, which keeps war resolution and AI construction behaving normally.

**Optional engine switch** (ini, default off): `render_frame_if_needed=0` trades a bit of frame smoothness for even faster ticks, same effect as the game's own `-disable_renderframeifneeded` launch flag. Simulation results are unaffected. Turning it back off needs a game restart.

---

## What to expect

- **This isn't free perf.** Tasks that make decisions (AI updates, investments) run less often, so the world develops differently than vanilla — a war declared a few days later, factories built in a different order. Nothing is cheated; it's all the game's own code running on a slower schedule. The weekly vanilla day keeps the differences mild at the default settings.
- **Single-player focused.** It's deterministic but not vanilla-identical, so multiplayer would need everyone on the same config. Treat it as single-player only.
- **Workshop mods:** this hooks the engine's tick tasks, not game data, so it should work alongside regular Workshop mods — but I haven't tested compatibility with any particular mod.

---

## Results

Milliseconds of simulation work per daily tick (built-in profiler, 1,500-tick windows, patch 1.14.5). **Vanilla baseline** from one complete hands-off observer campaign, 1840–1930 — **hooked-run testing with the current cadences is in progress** and will be added.

| Year | Vanilla |
|:---:|---:|
| 1840 | 6.95 ms |
| 1850 | 11.19 ms |
| 1860 | 13.57 ms |
| 1870 | 12.88 ms |
| 1880 | 13.17 ms |
| 1890 | 14.29 ms |
| 1900 | 17.09 ms |
| 1910 | 20.19 ms |
| 1920 | 25.56 ms |
| 1930 | **31.09 ms** |

> [!NOTE]
> **Vanilla more than quadruples over a campaign — the 1900s onward is where it compounds (+3, +5.4, +5.5 ms per decade).**

### Methodology

- Hands-off **observer mode**, no player input, one continuous session per run
- **Medium** graphics settings, **FPS capped at 60**, maximum zoom-out level
- Fixed **RNG seed** per campaign (so runs are reproducible and comparable)
- Game version 1.14.5 (rev `9f8281a3`); measured with the game's own `dump_ticktask_timings` console command
- Test machine: AMD Ryzen 7 9800X3D (8C/16T), RTX 5070 Ti, 96 GB RAM, Windows 11

---

## Using it

**Requires:** Victoria 3 **1.15.0**, **1.14.5**, or **1.13.11**; Windows x64. See [Supported game versions](#supported-game-versions).

1. Start Victoria 3 through Steam as usual.
2. **Load your save, then pause the game.**
3. Double-click `INJECT HOOKS.cmd`.
4. Read the window:

| Message | Meaning |
|---|---|
| `N/M hooks installed` | Active — unpause and play on. (The current release ships all 8 enabled. A note about skipped older-version candidate checks is normal on newer game builds.) |
| `WRONG GAME VERSION` | No hook matched the game build — nothing was patched, you're on pure vanilla. |
| `PARTIAL INSTALL` | Only some hooks have a working target on this game build — the throttling IS active, but the build may not be fully supported; please report it. |

## Updating

On every launch the kit checks this repo's latest GitHub release (HTTPS, 5-second timeout, **silent if offline**). Nothing is ever downloaded or changed without your explicit choice: when a newer version is available you are shown the release and asked —

- **U** — update now: the new kit is downloaded from GitHub and applied in the same window, before injection. **Your `vic3hook.ini` customizations are preserved** (values kept, newly added hook keys merged in with their defaults).
- **M** — open the [releases page](https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest) in your browser to download manually.
- **Enter** — not now; continue with the installed version.

To skip the check entirely for a run, double-click with `-SkipUpdate` appended, or call the script directly:

```powershell
powershell -File inject_running.ps1 -SkipUpdate
```

---

## Tuning

Open `vic3hook.ini` and set any task's `_every_n` value:

| Value | Behavior |
|:---:|---|
| `0` | Off — task runs every call (fully vanilla) |
| `1` | Always run (passthrough) |
| `N` | Work on every Nth call |

`vanilla_day_every_n_days` controls the scheduled full-fidelity day (default `7` = once a week; `0` disables it). Don't disable it unless you know what you're trading away — it's what keeps day-phased AI systems in sync.

> [!TIP]
> Changes apply **within 5 seconds** while the game runs — no restart needed. Turning a hook on from `0` installs it live; turning one off (`N` → `0`) makes it pass through immediately.

---

## Troubleshooting

- **Game misbehaves after tuning** → set that task's `every_n=0` in the ini, or delete the folder — the game itself is never modified.
- **Re-injecting after a game restart** → run `INJECT HOOKS.cmd` again (load save, pause, inject).

---

## Supported game versions

- **Victoria 3 1.15.0 ("Ivan-Chai")** — supported since release `1.15.0.1` (beta). Live-tested with all eight hooks active; per-task timing measurements on this patch are still being gathered.
- **Victoria 3 1.14.5 ("Ice Tea")** and **1.13.11 ("Matcha")** — fully supported.
- Any other build: every hook's check fails and nothing is patched — you stay at vanilla speed, and the injector says so.

---

## Technical notes

How it works, for the curious:

- Victoria 3's daily simulation is a serial queue of ~266 named **tick tasks**. Profiling them with the game's own console command (`dump_ticktask_timings`) shows a handful whose cost explodes as your country grows — one modifier-graph recalculation alone approaches 2.4 ms per tick by 1890.
- The kit injects a small DLL that lets those tasks run on a slower cadence:

```text
game tick ──► hooked task ──► skip?  ── yes ──► return (no work, no numbers touched)
                          └── no  ──► original game code runs, unmodified
```

- The game executable is **never modified** — it's runtime injection.
- Every hook verifies the target function's bytes before patching; on any mismatch it **disables itself instead of patching** (fail closed). If the game updates, the worst case is a vanilla-speed session and a log entry.
- Per-version hook targets ship as a data file (`vic3hook.targets`, current builds) or as built-in candidate tables (1.14.5, 1.13.11). Each candidate is byte-checked before install; the first table whose checks pass wins.
- History note on `RecalculateModifierNodes`: an early test once blamed throttling it for one-shot diplomatic deals not applying; a later autopsy reproduced that deal failure with hooks completely disabled on a corrupted vanilla save, and the finding was retracted. The mechanism-level concern (stale modifier data during a throttled window) is why it ships at the gentlest cadence and why the ini keeps a one-key route back to vanilla.

---

## Building from source

Requires Visual Studio Build Tools (C++) and CMake 3.21+:

```bat
cmake -B build -A x64
cmake --build build --config Release
```

The build **assembles `kit\` automatically** (DLL + injector + ini + `INJECT HOOKS.cmd` + MinHook's license) — zip its contents for a release, or copy the folder anywhere and use it directly.

**Packaging for a game version:** drop the game's `victoria3.exe` anywhere and run

```bat
python scripts\make_kit.py --game-version 1.15.0 --exe "<path>\victoria3.exe"
```

It resolves the hook targets from that exe (RTTI walk), writes `targets/<version>.tsv`, and stamps `dist/vic3hook-<version>/` plus a release zip. The kit's `vic3hook.targets` sidecar is read by the DLL at injection — **supporting a new game version needs no rebuild**, only a new sidecar. (The DLL also carries built-in candidate tables for older versions as a fallback when the sidecar is missing.) Every candidate is prologue-verified before patching, fail-closed.

**Releasing:** tag the release with the kit version (e.g. `1.15.0.2`) — the updater reads the version from the tag, so releases carry the zip asset only. The kit's own `version.txt` (inside the zip) is what an installed kit reads its local version from.

Target tables are **per game version**. After a game update, re-derive the addresses and prologues from the new exe and ship the new table:

```bat
python scripts\resolve_targets.py "<path-to>\victoria3.exe"
```

The resolver reads the image base and the tick-task layout from the exe itself (no hardcoded build constants) and follows vtable adjustor thunks — when a row reports a thunk target, hook the thunk target, matching the shipped table. Hooks fail closed on any other build.

---

## License

This project's code is released under the [MIT License](LICENSE).
[MinHook](https://github.com/TsudaKageyu/minhook) 1.3.3 is vendored under `third_party/MinHook` (BSD-2-Clause — see its `LICENSE.txt`, also bundled in release kits as `MINHOOK-LICENSE.txt`).

*Not affiliated with or endorsed by Paradox Interactive. Use at your own risk.*
