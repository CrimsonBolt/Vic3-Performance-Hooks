# Vic3 Performance Hooks

A performance hook for **Victoria 3 (1.14.5)**: a small DLL that lets a handful of expensive engine "tick tasks" run every Nth call instead of every call, reclaiming late-game simulation time. The game executable is **never modified** — this is runtime injection, and each hook verifies the target function's bytes before patching; on any mismatch it disables itself instead of patching (fail closed).

> **The rule: when a task runs, it runs 100% unmodified.** No number is ever computed differently — some caches and AI decisions just refresh a few days later than vanilla.

## Install

Unzip the release into a new folder — it can be placed anywhere.

1. Start Victoria 3 through Steam as usual.
2. **LOAD your save, then PAUSE the game.**
3. Double-click `INJECT HOOKS.cmd`.
4. Read the window:
   - `6/6 hooks installed` = active — unpause and play on.
   - `WRONG GAME VERSION` = the game updated; you are playing pure vanilla (nothing was patched — it fails closed).

**IMPORTANT: inject while the game is LOADED AND PAUSED.** Injecting while the game is running at speed can deadlock it during patching. If the game ever freezes right after injecting: close it via Task Manager, restart, and re-inject while paused.

**Vanilla:** just don't run the script. Restarting the game also fully reverts.

## Tuning

Open `vic3hook.ini` — set any task's `_every_n` to `0` (off), `1` (always), or `N` (work on every Nth call). Changes apply within 5 seconds while the game runs.

**Trouble:** game misbehaves after tuning → set `every_n=0` in the ini (or delete the folder — the game itself is never modified). To re-inject after a game restart, run `INJECT HOOKS.cmd` again.

## How it works

Victoria 3's daily simulation is a serial queue of ~266 named "tick tasks". I profiled them with the game's own built-in console command `dump_ticktask_timings`, found the ones whose cost explodes as your country grows, and injected a small DLL that lets a handful of them run every 2nd/5th/10th call instead of every call.

Throttled tasks (every Nth call):

| task | every Nth | what it is |
|---|---|---|
| PrepareNavyAIData | 2 | navy AI data prep |
| UpdateBuildingSpendingPriorities_Stage2 | 10 | building spending priorities |
| UpdateAI | 2 | country AI update |
| HandleAutonomousInvestment | 5 | AI autonomous investment |
| UpdateSpendingVariables | 5 | spending variables refresh |
| RecalculateModifierNodes | 5 | modifier-graph recalculation (the late-game knee) |

Profiled as the next candidates, not yet shipped: pop growth, economic migration, world market, state trades, pop-need cache, AI country goals.

## Performance

ms of sim work per daily tick (built-in profiler, 1,500-tick windows, same campaign, patch 1.14.5).

The performance numbers below are based on pretty much random fresh autosaves (every test was a fresh game on observer mode and some AI tweaks happened between runs), so they don't have perfectly matching hooked and unhooked results.

| year | unhooked | hooked |
|---|---|---|
| 1837 | 9.95 ms | 7.87 ms (control test) |
| 1860 | 13.14 ms | — |
| 1880 | 16.87 ms | — |
| 1890 | 26.49 ms | — |
| 1891 | ~28 ms | 14.70 ms |
| 1895 | — | 13.71 ms |

By 1895 with 6 hooks active, the daily sim costs what the **unhooked 1860** game did — roughly 35 years of late-game slowdown reclaimed.

## Honest caveats

- This isn't free perf. Tasks that make decisions (AI updates, investments) run less often, so the world drifts slightly from what vanilla would have done — a war declared a few days later, factories built in a different order. Nothing is cheated; it's all the game's own code running on a slower schedule.
- It's deterministic but not vanilla-identical, so multiplayer would need everyone on the same config. Treat it as single-player only.

## Building from source

Requires Visual Studio Build Tools (C++) and CMake 3.21+.

```bat
cmake -B build -A x64
cmake --build build --config Release
```

Targets are valid for **Victoria 3 1.14.5 only**. After a game update, re-derive the addresses and prologues with:

```bat
python scripts\resolve_targets.py "<path-to>\victoria3.exe"
```

and update the table in `src\hooks.cpp`. Hooks fail closed on any other build.

## License

This project's code is released under the [MIT License](LICENSE). [MinHook](https://github.com/TsudaKageyu/minhook) 1.3.3 is vendored under `third_party/MinHook` (BSD-2-Clause, see its `LICENSE.txt`).

Not affiliated with or endorsed by Paradox Interactive. Use at your own risk.
