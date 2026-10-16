vic3hook - Victoria 3 performance hooks
=======================================

Reclaims late-game simulation speed by letting a handful of expensive engine
tick-tasks run every other tick instead of every tick. No game files are
modified; everything is verified at runtime and fails closed (worst case on
a game update: a vanilla-speed session and a log line).

Usage (3 steps)
---------------
1. Start Victoria 3 through Steam as usual, LOAD your save, then PAUSE.
2. Double-click "INJECT HOOKS.cmd".
3. Read the window - it tells you exactly what happened:
   - "5/8 hooks installed"   -> working (3 are disabled by config on purpose)
   - "WRONG GAME VERSION"    -> nothing patched, you are on pure vanilla
   - anything else           -> the window explains it; details are in
                                vic3hook.log next to this file

Tuning
------
Open vic3hook.ini - every hook has an _every_n value (2 = every other day,
1 = always, 0 = off). The file hot-reloads every 5 seconds while the game
runs; you can change cadences live without re-injecting. Leave
RecalculateModifierNodes at 0.

Updating
--------
On launch the kit checks GitHub for a newer release of your game-version
line and ASKS before downloading anything (update now / open the releases
page / not now). Your ini customizations are always preserved.

This kit is built for the game version named in version.txt. Using it on a
different game version is safe (the byte checks disable the hooks) but does
nothing. Full documentation, results and source:
https://github.com/CrimsonBolt/Vic3-Performance-Hooks
