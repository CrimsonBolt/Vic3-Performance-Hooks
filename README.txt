vic3hook — perf hook for Victoria 3 1.14.5 (personal use only)

USE:     
Unzip into a new folder, can be placed anywhere

         1. Start Victoria 3 through Steam as usual.
         2. LOAD your save, then PAUSE the game.
         3. Double-click "INJECT HOOKS.cmd".
         4. Read the window:
            - "6/6 hooks installed"   = active, unpause and play on.
            - "WRONG GAME VERSION"    = game updated; you are playing pure
              vanilla (nothing was patched — it fails closed).

IMPORTANT: inject while the game is LOADED AND PAUSED. Injecting while the
game is running at speed can deadlock it during patching. If the game ever
freezes right after injecting: close it via Task Manager, restart, and
re-inject while paused.

VANILLA: just don't run the script. Restarting the game also fully reverts.

TUNING:  open vic3hook.ini — set any task's _every_n to 0 (off), 1 (always),
         or N (work on every Nth call). Changes apply within 5 seconds while
         the game runs.

TROUBLE: game misbehaves after tuning -> set every_n=0 in the ini (or delete
         this folder — the game itself is never modified). To re-inject
         after a game restart, run INJECT HOOKS.cmd again.


- This isn't free perf. Tasks that make decisions (AI updates, investments) run less often, so the world drifts slightly from what vanilla would have done — a war declared a few days later, factories built in a different order. Nothing is cheated; it's all the game's own code running on a slower schedule.

- It's deterministic but not vanilla-identical, so multiplayer would need everyone on the same config. Treat it as single-player only. 


Performance numbers

**Results** — ms of sim work per daily tick (built-in profiler, 1,500-tick windows, same campaign, patch 1.14.5):

The performance numbers below are based on pretty much random fresh autosaves (every test was a fresh game on observer mode and some AI tweaks happened between runs), so they don't have perfectly matching hooked and unhooked results.

| year | unhooked | hooked |

|---|---|---|

| 1837 | 9.95ms | 7.87ms (control test) |

| 1860 | 13.14ms | — |

| 1880 | 16.87ms | — |

| 1890 | 26.49ms | — |

| 1891 | ~28ms | 14.70ms | <<<<<<-------------

| 1895 | — | 13.71ms | <<<<<<-------------

- By 1895 with 6 hooks active, the daily sim costs what the **unhooked 1860** game did — roughly 35 years of late-game slowdown reclaimed. 

How does it work?

 Victoria 3's daily simulation is a serial queue of ~266 named "tick tasks". I profiled them with the game's own built-in console command `dump_ticktask_timings`, found the ones whose cost explodes as your country grows, and injected a small DLL that lets a handful of them run every 2nd/5th/10th call instead of every call. The game .exe is never modified — it's runtime injection, and each hook verifies the target function's bytes before patching; on any mismatch it disables itself instead of patching.

**The rule: when a task runs, it runs 100% unmodified.** No number is ever computed differently — some caches and AI decisions just refresh a few days later than vanilla.

Throttled tasks (every Nth call): navy AI prep (2), building spending priorities (10), country AI update (2), autonomous investment (5), spending variables (5), modifier-node recalc (5), pop growth (2), economic migration (2), AI country goals (2), pop-need cache (2), state trades (2), world market (2). 
