# 1.15.0 shared-seed campaign data (2026-11-06/07)

Raw data behind the README [Results](../../README.md#results) section.
Game 1.15.0 ("Ivan-Chai", rev `63385479`), one shared world: vanilla boot
1836 to 1840.1.1, then the identical save continued twice, hands-off
observer, speed 5, to 1931.1.1:

- **A (runA2-vanilla)** - no injection
- **B (runB2-hooks)** - all 8 hooks installed (`hooks installed: 8/8`,
  sidecar table), default cadences (7 tasks every-2, RecalculateModifierNodes
  every-5, weekly vanilla day)

## Files

- `economy_table.csv` - per-checkpoint (every 5 sim-years) world economy:
  world GDP, investment pool, world money, total building levels, building
  records, construction-sector levels. Extracted from the savegame files
  (melted with rakaly 0.8.22).
- `campaign_report_AB.md` - per-decade report: total ms/tick (built-in
  profiler, 1,500-tick windows before each decade boundary), per-task
  top-15, kit effect, wall seconds per sim-year, economy ratios, and the
  data-integrity block (9/9 dumps per arm, zero lost windows, 0 retried).

## Headline numbers

- Same-world throttle effect (decades where both worlds are still
  comparable, 1850-1880): total tick work -29% to -42%; the eight hooked
  tasks themselves -46% to -62%.
- The throttle keeps working after the worlds diverge: hooked tasks
  -23% to -34% (1890-1920). Late totals compare different-size worlds and
  are not a clean throttle measure.
- Economy after 90 years (1930.1.1, B vs A): world GDP +11%, building
  levels +11.5%, construction sectors +21%, investment pool 3.07x,
  world money 1.99x.
