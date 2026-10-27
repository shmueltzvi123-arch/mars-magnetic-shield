# Results

## What is kept here

| Path | Contents |
|---|---|
| `baseline_2026-04-24/` | The earlier serial study: 16 configurations, 100 events each |
| `full_study_2026-08-21/` | The multithreaded study: 16 configurations, 1000 events each, plus the per-run hit-energy histograms |

Each `study_results_detailed.csv` carries one row per configuration: the four coil
parameters, the field-map grid metadata, and the resulting shielding efficiency with
its breakdown into low (<100 MeV), medium (100–500 MeV) and high (>500 MeV) hits.

The histogram files are Geant4 analysis output, one per configuration. Their total
entry count equals the sum of the three hit counters in the corresponding CSV row,
which is how histogram filling and multithreaded merging were verified.

## What is not kept here

**The magnetic field maps.** Each is about 8 MB and a full study produces 16 of them,
so a single run would add 126 MB to the repository. They are also fully deterministic:
regenerating from the same four parameters reproduces them byte for byte, which was
verified by SHA-256 during the 2026-08-21 migration. Regenerating all 16 takes under
20 seconds.

One map is kept in `example_map/` at the repository root so the simulation can be run
without a Python step.

New study runs land in `results/runs/<timestamp>/` and are git-ignored.

## Reading the numbers

At 1000 events the Poisson uncertainty on a hit count of 73 is ±8.6, which is ±0.9 %
on the efficiency. Differences of a few hits between configurations are not
significant. See `docs/known_limitations.md` before drawing conclusions from these
tables.
