vic3hook — perf hook for Victoria 3 1.14.5 (personal use only)

USE:     1. Start Victoria 3 through Steam as usual.
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
