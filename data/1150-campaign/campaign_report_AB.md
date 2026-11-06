# 1.15 Performance Campaign - Final Report (draft)

Method: shared-seed three-arm design. One vanilla boot 1836->1840 creates the shared
world; arms resume from the identical 1840.1.1 save: A = vanilla, B = hooks (8/8 injected),
C = hooks + AI Builds More. Dump windows are the engine profiler ring (last ~1,500 ticks
(~4 sim-years) before each decade boundary). All arms ran in one session on one machine.

## Table 1 - total ms/tick per decade

| decade | A | B | 1.14.5 vanilla |
|---|---|---|---|
| 1840 | - | - | 6.95 |
| 1850 | 15.38 | 8.96 | 11.19 |
| 1860 | 16.57 | 11.35 | 13.57 |
| 1870 | 15.42 | 10.93 | 12.88 |
| 1880 | 18.24 | 12.80 | 13.17 |
| 1890 | 17.11 | 17.91 | 14.29 |
| 1900 | 20.80 | 18.89 | 17.09 |
| 1910 | 22.08 | 19.58 | 20.19 |
| 1920 | 27.28 | 25.05 | 25.56 |
| 1930 | 28.92 | 32.94 | 31.09 |

## Table 3 - kit effect (A vanilla - B hooks), ms/tick

| decade | A total | B total | saved | % |
|---|---:|---:|---:|---:|
| 1850 | 15.38 | 8.96 | +6.42 | +41.8% |
| 1860 | 16.57 | 11.35 | +5.22 | +31.5% |
| 1870 | 15.42 | 10.93 | +4.49 | +29.1% |
| 1880 | 18.24 | 12.80 | +5.44 | +29.8% |
| 1890 | 17.11 | 17.91 | -0.79 | -4.6% |
| 1900 | 20.80 | 18.89 | +1.91 | +9.2% |
| 1910 | 22.08 | 19.58 | +2.50 | +11.3% |
| 1920 | 27.28 | 25.05 | +2.22 | +8.1% |
| 1930 | 28.92 | 32.94 | -4.02 | -13.9% |

## Table 2 - per-task ms/tick (top 15, A / B / C)

### 1850

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.96 | 1.17 |
| RecalculateModifierNodes | 1.51 | 0.37 |
| SyncedAIPostUpdate | 1.17 | 1.04 |
| UpdatePopGrowth | 1.02 | 0.77 |
| MoveArmies | 0.96 | 0.02 |
| ProcessEconomicMigration | 0.90 | 0.31 |
| UpdateEmployment | 0.58 | 0.43 |
| UpdateWorldMarket | 0.52 | 0.46 |
| MonthlyPulseJournalEntries | 0.52 | 0.44 |
| UpdateAICountryGoalsAdjustments | 0.44 | 0.21 |
| PrepareNavyAIData | 0.33 | 0.17 |
| UpdateStatePopNeedCache | 0.30 | 0.27 |
| UpdateSpendingVariables | 0.29 | 0.12 |
| UpdateColonizationProgress | 0.19 | 0.29 |
| UpdateInterestGroupPoliticalStrength | 0.26 | 0.25 |

### 1860

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.85 | 1.40 |
| RecalculateModifierNodes | 1.34 | 0.36 |
| UpdatePopGrowth | 1.17 | 1.11 |
| SyncedAIPostUpdate | 1.06 | 1.15 |
| ProcessEconomicMigration | 1.06 | 0.36 |
| MoveArmies | 0.80 | 0.00 |
| UpdateEmployment | 0.66 | 0.59 |
| UpdateAICountryGoalsAdjustments | 0.64 | 0.36 |
| UpdateWorldMarket | 0.57 | 0.58 |
| MonthlyPulseJournalEntries | 0.46 | 0.48 |
| PrepareNavyAIData | 0.48 | 0.27 |
| UpdateSpendingVariables | 0.38 | 0.20 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.35 | 0.26 |
| UpdateStatePopNeedCache | 0.34 | 0.31 |
| UpdateInterestGroupPoliticalStrength | 0.30 | 0.26 |

### 1870

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.78 | 1.41 |
| UpdatePopGrowth | 1.28 | 1.16 |
| RecalculateModifierNodes | 1.22 | 0.36 |
| ProcessEconomicMigration | 1.06 | 0.40 |
| SyncedAIPostUpdate | 0.81 | 0.86 |
| UpdateAICountryGoalsAdjustments | 0.77 | 0.42 |
| UpdateEmployment | 0.71 | 0.65 |
| MoveArmies | 0.66 | 0.00 |
| UpdateWorldMarket | 0.55 | 0.59 |
| MonthlyPulseJournalEntries | 0.37 | 0.39 |
| UpdateStatePopNeedCache | 0.36 | 0.35 |
| UpdateInterestGroupPoliticalStrength | 0.36 | 0.27 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.34 | 0.33 |
| PreUpdatePoliticalMovements | 0.26 | 0.29 |
| PrepareNavyAIData | 0.29 | 0.18 |

### 1880

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.69 | 1.64 |
| UpdatePopGrowth | 1.61 | 1.32 |
| RecalculateModifierNodes | 1.48 | 0.45 |
| ProcessEconomicMigration | 1.06 | 0.39 |
| UpdateAICountryGoalsAdjustments | 1.05 | 0.47 |
| UpdateEmployment | 0.94 | 0.76 |
| SyncedAIPostUpdate | 0.65 | 0.79 |
| MoveArmies | 0.75 | 0.02 |
| UpdateWorldMarket | 0.60 | 0.61 |
| UpdateInterestGroupPoliticalStrength | 0.50 | 0.45 |
| PrepareNavyAIData | 0.48 | 0.32 |
| PreUpdatePoliticalMovements | 0.46 | 0.39 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.38 | 0.45 |
| UpdateStatePopNeedCache | 0.41 | 0.39 |
| MonthlyPulseJournalEntries | 0.33 | 0.38 |

### 1890

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.72 | 2.59 |
| UpdatePopGrowth | 1.63 | 1.69 |
| RecalculateModifierNodes | 1.20 | 0.43 |
| ProcessEconomicMigration | 1.19 | 0.40 |
| MoveArmies | 0.46 | 1.13 |
| UpdateEmployment | 0.92 | 0.99 |
| UpdateAICountryGoalsAdjustments | 0.86 | 0.54 |
| SyncedAIPostUpdate | 0.57 | 0.82 |
| UpdateWorldMarket | 0.60 | 0.70 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.38 | 0.68 |
| PrepareNavyAIData | 0.63 | 0.24 |
| PreUpdatePoliticalMovements | 0.45 | 0.50 |
| UpdateInterestGroupPoliticalStrength | 0.50 | 0.50 |
| UpdateStatePopNeedCache | 0.40 | 0.43 |
| HandleAutonomousInvestment | 0.24 | 0.43 |

### 1900

| task | A | B |
|---|---:|---:|
| UpdateAI | 3.95 | 1.97 |
| UpdatePopGrowth | 1.91 | 1.88 |
| ProcessEconomicMigration | 1.63 | 0.54 |
| RecalculateModifierNodes | 1.40 | 0.46 |
| UpdateEmployment | 1.07 | 1.18 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.37 | 0.92 |
| UpdateAICountryGoalsAdjustments | 0.89 | 0.62 |
| HandleAutonomousInvestment | 0.24 | 0.87 |
| MoveArmies | 0.80 | 0.14 |
| SyncedAIPostUpdate | 0.79 | 0.77 |
| UpdateWorldMarket | 0.65 | 0.75 |
| UpdateInterestGroupPoliticalStrength | 0.53 | 0.50 |
| PreUpdatePoliticalMovements | 0.49 | 0.50 |
| UpdateSpendingVariables | 0.47 | 0.34 |
| UpdateStatePopNeedCache | 0.46 | 0.47 |

### 1910

| task | A | B |
|---|---:|---:|
| UpdateAI | 2.88 | 1.78 |
| UpdatePopGrowth | 2.34 | 2.17 |
| ProcessEconomicMigration | 1.74 | 0.69 |
| RecalculateModifierNodes | 1.64 | 0.43 |
| UpdateEmployment | 1.31 | 1.48 |
| HandleAutonomousInvestment | 0.37 | 1.28 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.46 | 1.10 |
| UpdateAICountryGoalsAdjustments | 0.94 | 0.52 |
| UpdateWorldMarket | 0.81 | 0.83 |
| UpdateInterestGroupPoliticalStrength | 0.74 | 0.69 |
| MoveArmies | 0.69 | 0.03 |
| SyncedAIPostUpdate | 0.61 | 0.52 |
| PrepareNavyAIData | 0.42 | 0.55 |
| UpdateStatePopNeedCache | 0.55 | 0.50 |
| PreUpdatePoliticalMovements | 0.50 | 0.50 |

### 1920

| task | A | B |
|---|---:|---:|
| UpdateAI | 4.05 | 2.28 |
| UpdatePopGrowth | 2.72 | 2.65 |
| ProcessEconomicMigration | 2.51 | 0.93 |
| HandleAutonomousInvestment | 0.62 | 2.31 |
| UpdateEmployment | 1.72 | 2.03 |
| RecalculateModifierNodes | 1.60 | 0.55 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.52 | 1.28 |
| PrepareNavyAIData | 1.18 | 0.95 |
| UpdateAICountryGoalsAdjustments | 1.10 | 0.59 |
| UpdateWorldMarket | 0.82 | 0.98 |
| MoveArmies | 0.90 | 0.02 |
| UpdateInterestGroupPoliticalStrength | 0.75 | 0.75 |
| UpdateStatePopNeedCache | 0.54 | 0.58 |
| SyncedAIPostUpdate | 0.51 | 0.54 |
| PreUpdatePoliticalMovements | 0.50 | 0.52 |

### 1930

| task | A | B |
|---|---:|---:|
| UpdateAI | 3.17 | 4.51 |
| UpdatePopGrowth | 3.16 | 3.08 |
| HandleAutonomousInvestment | 1.12 | 2.79 |
| UpdateEmployment | 2.25 | 2.60 |
| ProcessEconomicMigration | 2.37 | 0.99 |
| RecalculateModifierNodes | 1.78 | 0.60 |
| UpdateBuildingSpendingPriorities_Stage1 | 0.16 | 1.31 |
| UpdateBuildingSpendingPriorities_Stage2 | 0.77 | 1.30 |
| PrepareNavyAIData | 0.66 | 1.30 |
| UpdateWorldMarket | 0.86 | 1.07 |
| UpdateAICountryGoalsAdjustments | 1.07 | 0.55 |
| MoveArmies | 0.98 | 0.10 |
| UpdateInterestGroupPoliticalStrength | 0.78 | 0.96 |
| MoveFleets | 0.12 | 0.69 |
| UpdateStateTrades | 0.43 | 0.65 |

## Table 5 - wall seconds per sim-year per decade

| decade | A | B |
|---:|---:|---:|
| 1840 | 114.2 | 96.2 |
| 1850 | 130.8 | 115.8 |
| 1860 | 142.8 | 124.6 |
| 1870 | 148.6 | 130.6 |
| 1880 | 145.6 | 142.8 |
| 1890 | 150.2 | 151.6 |
| 1900 | 153.2 | 157.8 |
| 1910 | 178.8 | 190.8 |
| 1920 | 187.8 | 202.6 |
| 1930 | - | - |

## Table 6 - economy pivot (from economy_table.csv)

| decade | metric | A | B |
|---|---|---:|---:|
| 1840 | world_gdp | 440,570,247 | 440,570,247 |
| 1840 | world_investment_pool | 29,433,255 | 29,433,255 |
| 1840 | world_money | 47,177,823 | 47,177,823 |
| 1840 | total_levels | 98,912 | 98,912 |
| 1840 | construction_sector_levels | 573 | 573 |
| 1850 | world_gdp | 591,431,897 | 588,368,273 |
| 1850 | world_investment_pool | 112,424,401 | 114,785,251 |
| 1850 | world_money | 87,867,024 | 91,724,717 |
| 1850 | total_levels | 103,132 | 102,527 |
| 1850 | construction_sector_levels | 638 | 669 |
| 1860 | world_gdp | 762,523,281 | 742,161,309 |
| 1860 | world_investment_pool | 188,023,081 | 186,027,529 |
| 1860 | world_money | 77,819,039 | 67,472,347 |
| 1860 | total_levels | 107,085 | 107,981 |
| 1860 | construction_sector_levels | 849 | 816 |
| 1870 | world_gdp | 967,318,305 | 913,510,422 |
| 1870 | world_investment_pool | 161,415,189 | 152,189,410 |
| 1870 | world_money | 77,866,200 | 69,295,683 |
| 1870 | total_levels | 112,844 | 113,746 |
| 1870 | construction_sector_levels | 946 | 981 |
| 1880 | world_gdp | 1,189,956,892 | 1,137,808,248 |
| 1880 | world_investment_pool | 213,599,561 | 210,825,475 |
| 1880 | world_money | 84,816,352 | 87,767,598 |
| 1880 | total_levels | 120,665 | 122,224 |
| 1880 | construction_sector_levels | 921 | 1,007 |
| 1890 | world_gdp | 1,438,629,658 | 1,516,507,508 |
| 1890 | world_investment_pool | 288,495,349 | 285,391,856 |
| 1890 | world_money | 82,903,386 | 120,676,474 |
| 1890 | total_levels | 131,305 | 133,701 |
| 1890 | construction_sector_levels | 928 | 1,133 |
| 1900 | world_gdp | 1,725,864,828 | 2,066,156,114 |
| 1900 | world_investment_pool | 291,779,999 | 423,533,818 |
| 1900 | world_money | 76,370,918 | 185,080,680 |
| 1900 | total_levels | 143,164 | 149,906 |
| 1900 | construction_sector_levels | 1,138 | 1,368 |
| 1910 | world_gdp | 2,244,878,134 | 3,053,976,271 |
| 1910 | world_investment_pool | 377,313,626 | 716,376,621 |
| 1910 | world_money | 139,850,523 | 296,054,543 |
| 1910 | total_levels | 158,617 | 173,561 |
| 1910 | construction_sector_levels | 1,240 | 1,862 |
| 1920 | world_gdp | 3,167,502,029 | 4,211,853,118 |
| 1920 | world_investment_pool | 461,791,110 | 1,240,173,553 |
| 1920 | world_money | 248,414,932 | 515,513,598 |
| 1920 | total_levels | 182,176 | 205,896 |
| 1920 | construction_sector_levels | 1,527 | 2,233 |
| 1930 | world_gdp | 4,664,062,355 | 5,183,450,847 |
| 1930 | world_investment_pool | 791,912,793 | 2,430,565,717 |
| 1930 | world_money | 441,295,980 | 878,924,606 |
| 1930 | total_levels | 218,859 | 244,122 |
| 1930 | construction_sector_levels | 2,373 | 2,871 |

Economy similarity (hooks vs vanilla, same world): per-decade ratio B/A for GDP, levels, sectors

| decade | GDP B/A | levels B/A | sectors B/A |
|---|---:|---:|---:|
| 1840 | 1.000 | 1.000 | 1.000 |
| 1850 | 0.995 | 0.994 | 1.049 |
| 1860 | 0.973 | 1.008 | 0.961 |
| 1870 | 0.944 | 1.008 | 1.037 |
| 1880 | 0.956 | 1.013 | 1.093 |
| 1890 | 1.054 | 1.018 | 1.221 |
| 1900 | 1.197 | 1.047 | 1.202 |
| 1910 | 1.360 | 1.094 | 1.502 |
| 1920 | 1.330 | 1.130 | 1.462 |
| 1930 | 1.111 | 1.115 | 1.210 |

## Table 7 - data integrity

| arm | dumps | lost windows | injected 8/8 | resumed | duration min |
|---|---:|---|---|---|---:|
| A | 9 | 0 | not injected (vanilla arm) | yes | 115.4 |
| B | 9 | 0 | 8/8 | yes | 112.6 |

## Limitations

- Dump windows are the engine profiler ring: the last ~1,500 ticks (~4 sim-years) before each decade boundary.
- One shared world per lineage: results are one-world, not a population study; economy deltas at the same decade
  are arm effects, but their magnitude on other seeds may vary.
- Arms ran sequentially in one session on one machine; month-scale machine drift (external-measured ~8%) does not apply.