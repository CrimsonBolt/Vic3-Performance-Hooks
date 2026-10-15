# Vic3 Performance Hooks

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Game: Victoria 3](https://img.shields.io/badge/game-Victoria%203%201.14.5-8e44ad.svg)](#building-from-source)
[![Platform: Windows x64](https://img.shields.io/badge/platform-Windows%20x64-2ea44f.svg)](#install)
[![Download - Releases](https://img.shields.io/badge/download-GitHub%20Releases-brightgreen.svg)](https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest)

**Reclaim late-game simulation speed in Victoria 3** by letting a handful of expensive engine tick-tasks run every Nth call instead of every call — with no changes to game files and no changes to how the game computes anything.

> [!TIP]
> ### ➤ [Download the latest release here](https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest)
> Unzip it anywhere — no build, no install. Then see [Install](#install) for the 3-step usage.
> The kit checks for updates automatically on every launch.

> [!IMPORTANT]
> **When a task runs, it runs 100% unmodified.** No number is ever computed differently — some caches and AI decisions just refresh a few days later than vanilla.

---

## Contents

- [How it works](#how-it-works)
- [Gameplay impacts](#gameplay-impacts)
- [Results](#results)
- [Install](#install)
- [Tuning](#tuning)
- [Troubleshooting](#troubleshooting)
- [Building from source](#building-from-source)
- [License](#license)

---

## How it works

Victoria 3's daily simulation is a serial queue of ~266 named **tick tasks**. Profiling them with the game's own built-in console command (`dump_ticktask_timings`) shows a handful whose cost explodes as your country grows — one modifier-graph recalculation alone approaches 2.4 ms per tick by 1890.

This project injects a small DLL that lets those tasks run on a slower cadence:

```text
game tick ──► hooked task ──► skip?  ── yes ──► return (no work, no numbers touched)
                          └── no  ──► original game code runs, unmodified
```

- The game executable is **never modified** — it's runtime injection.
- Every hook verifies the target function's bytes before patching; on any mismatch it **disables itself instead of patching** (fail closed).
- If the game updates, the worst case is a vanilla-speed session and a log entry.

**Shipped hooks** (each throttled to every other tick):

| Task | What it is |
|---|---|
| `UpdateBuildingSpendingPriorities_Stage2` | building spending priorities |
| `HandleAutonomousInvestment` | AI autonomous investment |
| `PrepareNavyAIData` | navy AI data prep |
| `UpdateAI` | country AI update |
| `UpdateSpendingVariables` | spending-variables refresh |

`RecalculateModifierNodes` — the single biggest late-game cost — is **deliberately not throttled**: skipping it defers modifier invalidations, which silently breaks one-shot decisions (diplomatic deals that show 100% valid but never apply). Isolated and confirmed via same-save A/B testing; leave it at `0` in the ini.

All six run every tick in vanilla; the default profile throttles each to **every other tick** — the gentlest setting. More aggressive values in `vic3hook.ini` buy more speed at the cost of larger gameplay differences.

**The weekly "vanilla day":** every 7th day, *all* throttled tasks run at full fidelity for that one day. The engine schedules some systems on fixed day-phases (every 4th day, monthly), and a fixed skip pattern can phase-lock against them — the vanilla day re-synchronizes everything, which keeps war resolution and AI construction behaving normally. Same-seed testing confirmed both the problem (wars could freeze under constant skipping) and the fix (zero freezes with the weekly resync, verified across decades). Tunable via `vanilla_day_every_n_days` in the ini.

Profiled as the next candidates, not yet shipped: economic migration, job satisfaction.

---

## Gameplay impacts

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

## Install

**Requires:** Victoria 3 **1.14.5** (rev `9f8281a3`), Windows x64. Unzip the release into a new folder — it can be placed anywhere.

1. Start Victoria 3 through Steam as usual.
2. **Load your save, then pause the game.**
3. Double-click `INJECT HOOKS.cmd`.
4. Read the window:

| Message | Meaning |
|---|---|
| `6/6 hooks installed` | Active — unpause and play on. |
| `WRONG GAME VERSION` | No hook matched the game build — nothing was patched, you're on pure vanilla. |
| `PARTIAL VERSION MISMATCH` | Some hooks installed and are active, others failed their byte check — the game is **not** vanilla. Restart without injecting for pure vanilla. |

> [!WARNING]
> **Inject only while the game is loaded and paused.** Injecting while the game runs at speed can deadlock it during patching. If the game freezes right after injecting: close it via Task Manager, restart, and re-inject while paused.

> [!NOTE]
> **Going back to vanilla:** just don't run the script. Restarting the game also fully reverts.

## Updating

On every launch the kit checks this repo's latest GitHub release (HTTPS, 5-second timeout, **silent if offline**). If a newer version is published, the new kit is downloaded and applied automatically when the injector window closes — **your `vic3hook.ini` customizations are preserved** (values kept, newly added hook keys merged in with their defaults). To skip the check for a run, double-click with `-SkipUpdate` appended, or call the script directly:

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
> Changes apply **within 5 seconds** while the game runs — no restart needed. Turning a hook on from `0` installs it live (byte-verified, same as at injection); turning one off (`N` → `0`) makes it pass through immediately.

---

## Troubleshooting

- **Game misbehaves after tuning** → set that task's `every_n=0` in the ini, or delete the folder — the game itself is never modified.
- **Re-injecting after a game restart** → run `INJECT HOOKS.cmd` again (load save, pause, inject).

---

## Building from source

Requires Visual Studio Build Tools (C++) and CMake 3.21+:

```bat
cmake -B build -A x64
cmake --build build --config Release
```

The build **assembles `kit\` automatically** (DLL + injector + ini + `INJECT HOOKS.cmd` + MinHook's license) — zip its contents for a release, or copy the folder anywhere and use it directly.

Targets are valid for **Victoria 3 1.14.5 only**. After a game update, re-derive the addresses and prologues, then update the table in `src\hooks.cpp`:

```bat
python scripts\resolve_targets.py "<path-to>\victoria3.exe"
```

The resolver reads the image base and the tick-task layout from the exe itself (no hardcoded build constants) and follows vtable adjustor thunks — when a row reports a thunk target, hook the thunk target, matching the shipped table. Hooks fail closed on any other build.

---

## License

This project's code is released under the [MIT License](LICENSE).
[MinHook](https://github.com/TsudaKageyu/minhook) 1.3.3 is vendored under `third_party/MinHook` (BSD-2-Clause — see its `LICENSE.txt`, also bundled in release kits as `MINHOOK-LICENSE.txt`).

*Not affiliated with or endorsed by Paradox Interactive. Use at your own risk.*
