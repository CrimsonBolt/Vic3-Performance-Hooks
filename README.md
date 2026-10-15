# Vic3 Performance Hooks

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Game: Victoria 3](https://img.shields.io/badge/game-Victoria%203%201.14.5-8e44ad.svg)](#building-from-source)
[![Platform: Windows x64](https://img.shields.io/badge/platform-Windows%20x64-2ea44f.svg)](#install)

**Reclaim late-game simulation speed in Victoria 3** by letting a handful of expensive engine tick-tasks run every Nth call instead of every call — with no changes to game files and no changes to how the game computes anything.

> [!IMPORTANT]
> **When a task runs, it runs 100% unmodified.** No number is ever computed differently — some caches and AI decisions just refresh a few days later than vanilla.

---

## Contents

- [How it works](#how-it-works)
- [Gameplay impacts](#gameplay-impacts)
- [Known issues](#known-issues)
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

**Shipped hooks** (each runs every Nth call):

| Task | Every Nth | What it is |
|---|:---:|---|
| `RecalculateModifierNodes` | 5 | modifier-graph recalculation — the single biggest late-game cost |
| `UpdateBuildingSpendingPriorities_Stage2` | 10 | building spending priorities |
| `PrepareNavyAIData` | 2 | navy AI data prep |
| `UpdateAI` | 2 | country AI update |
| `HandleAutonomousInvestment` | 5 | AI autonomous investment |
| `UpdateSpendingVariables` | 5 | spending-variables refresh |

Profiled as the next candidates, not yet shipped: pop growth, economic migration, world market, state trades, pop-need cache, AI country goals.

---

## Gameplay impacts

- **This isn't free perf.** Tasks that make decisions (AI updates, investments) run less often, so the world develops differently than vanilla — a war declared a few days later, factories built in a different order. Nothing is cheated; it's all the game's own code running on a slower schedule. Over a full campaign the differences add up (see [Known issues](#known-issues)).
- **Single-player focused.** It's deterministic but not vanilla-identical, so multiplayer would need everyone on the same config. Treat it as single-player only.
- **Workshop mods:** this hooks the engine's tick tasks, not game data, so it should work alongside regular Workshop mods — but I haven't tested compatibility with any particular mod.

## Known issues

- **In long campaigns, some wars may never end.** Being fixed — an update with corrected hook cadences is in the works. Until then, setting `RecalculateModifierNodes_every_n=2` (or `0`) in `vic3hook.ini` avoids it for new campaigns.
- **At the default settings, the AI builds a somewhat smaller economy over a full campaign.** Being fixed in the same update; lowering `HandleAutonomousInvestment_every_n` and `UpdateBuildingSpendingPriorities_Stage2_every_n` to `2`–`3` (or `0`) in `vic3hook.ini` reduces it in the meantime.

---

## Results

Milliseconds of simulation work per daily tick — built-in profiler, 1,500-tick windows, patch 1.14.5.

| Year | Unhooked | Hooked |
|:---:|---:|---:|
| 1837 | 9.95 ms | 7.87 ms *(control test)* |
| 1860 | 13.14 ms | — |
| 1880 | 16.87 ms | — |
| 1890 | **26.49 ms** | — |
| 1891 | ~28 ms *(extrapolated)* | **14.70 ms** |
| 1895 | — | **13.71 ms** |

> [!NOTE]
> **By 1895, the daily sim costs what the unhooked 1860 game did — roughly 35 years of late-game slowdown reclaimed.**

*Methodology note: the numbers are based on pretty much random fresh autosaves (every test was a fresh game on observer mode, and some AI tweaks happened between runs), so they don't have perfectly matching hooked and unhooked results.*

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
