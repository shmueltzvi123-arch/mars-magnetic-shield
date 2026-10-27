# Mars Magnetic Shield

A Geant4 + Python simulation pipeline for evaluating **active magnetic shielding**
of a Mars surface habitat against solar energetic protons.

The project generates the magnetic field of a two-coil shield analytically in
Python, propagates protons through it with Geant4, and reports how many reach the
habitat — automated end to end across a sweep of coil geometries and currents.

---

## The idea

Two horizontal, concentric current loops sit in the ground plane around the habitat:

- an outer **shield coil** (18–20 m radius, +0.8–1.0 MA) that deflects incoming protons;
- an inner **cancel coil** (8–9 m radius, −0.4–0.45 MA) that nulls the field inside
  the habitat, so the crew is not exposed to the shield's own field.

At the centre the two contributions are each μ₀I/2R ≈ 31 mT and very nearly cancel.

The simulated environment is a Martian surface: silicon-dioxide ground below z = 0,
a carbon-dioxide atmosphere above it, and a polyethylene habitat cylinder of radius
9 m and height 5 m standing on the surface. Protons are drawn from a power-law solar
energetic particle spectrum, 1–500 MeV with spectral index 1.5.

---

## How it runs

```
scripts/generate_magnetic_field_map.py
    analytic Biot–Savart field of the two loops (elliptic integrals, SciPy)
    -> 51 x 51 x 51 text map, 1 m spacing, spanning -25 m to +25 m
                |
                |  MAG_FIELD_MAP environment variable
                v
MarsShield.exe  (Geant4 11.4, multithreaded, QGSP_BERT_HP)
    loads the map, trilinear interpolation, ClassicalRK4 propagation
    fires 1000 protons, counts events that reach the habitat
                |
                |  parsed from stdout
                v
scripts/parameter_study.py
    -> study_results_detailed.csv, one row per configuration
```

Each execution writes its own timestamped directory, and every Geant4 subprocess gets
its own working directory, so runs never overwrite each other's histograms.

---

## Repository layout

| Path | Contents |
|---|---|
| `src/` | The Geant4 application: geometry, field lookup and interpolation, primary generator, hit scoring, multithreaded run accounting |
| `scripts/` | Two Python entry points — one computes a magnetic field map, the other drives the 16-configuration parameter sweep |
| `example_map/` | One ready-made field map, so the simulation can be run without regenerating anything |
| `results/` | Result tables and hit-energy histograms from completed studies. Field maps are **not** stored — they are deterministic and regenerate in about a second each |
| `docs/` | Known limitations, and the development history of the project |
| `main.cc` | Entry point: run manager, physics list, user actions |
| `run_study.mac` | Production macro, 1000 events |
| `run_quick_100.mac` | Short macro, 100 events, for sanity checks |

### The Geant4 sources

| File | Role |
|---|---|
| `DetectorConstruction` | Builds world, ground, atmosphere, habitat and the two coil bodies; attaches the magnetic field, stepper and chord finder |
| `BFieldLookup` | Reads a field map into a 51³ grid and interpolates it trilinearly; returns zero below ground |
| `PrimaryGeneratorAction` | Samples the SEP energy spectrum by inverse transform and launches protons inward from 35 m |
| `SteppingAction` | Kills tracks that enter a coil body; registers entries into the habitat |
| `EventAction` | Keeps only the first habitat entry per event, so no event is counted twice |
| `RunAction` | Thread-safe counters via `G4AccValue`, the hit-energy histogram, and the final merged report |
| `ActionInitialization` | Builds the per-worker action chain required in multithreaded mode |

---

## Requirements

- Geant4 11.4 with the environment configured (`Geant4_ROOT`, `bin` on `PATH`)
- CMake 3.16 or newer, and a C++17 compiler (developed with MSVC 2022)
- Python 3.11 or newer with `numpy` and `scipy`

## Build and run

```bash
cmake -S . -B build
cmake --build build --config Release

pip install -r requirements.txt

# one simulation against the bundled example map
MAG_FIELD_MAP=example_map/map_run_001_Rs_18.00_Is_8.00e+05_Rc_8.00_Ic_-4.00e+05.txt \
  ./build/Release/MarsShield run_study.mac

# or the full 16-configuration sweep, maps included
python scripts/parameter_study.py
```

On Windows, set the variable with `set MAG_FIELD_MAP=...` and use
`build\Release\MarsShield.exe`.

---

## Results

The 16-configuration sweep completed on 2026-08-21 with 1000 events per
configuration. All eight 18 m configurations returned 73 habitat hits
(92.7 % of protons stopped) and all eight 20 m configurations returned 76
(92.4 %). Hit-energy histograms were verified to match the counters exactly in
every run.

**These numbers should be read with care.** A validation review of this pipeline
established that:

- the reported figure is dominated by **atmospheric stopping, not magnetic
  deflection**. The 35 m flight path through carbon dioxide is 6.45 g/cm², which
  stops every proton below about 85 MeV — roughly 93 % of this spectrum. An
  independent calculation with the field switched off reproduces the same
  efficiency to within statistics;
- within the swept current range the field displaces a surviving proton by only
  2–30 mm, against a 9 m habitat radius, so the identical results across
  configurations are the correct output of the model rather than a defect;
- the radial deflection of a near-vertically incident proton is a second-order
  effect for this coil topology — it scales as I² rather than I — because the field
  is nearly parallel to the velocity near the axis. The first-order force is
  azimuthal;
- at 1000 events the Poisson uncertainty on the hit count is ±8.6, so differences
  of a few hits between configurations are not significant.

What this repository therefore demonstrates is a **working, reproducible research
pipeline**, together with a quantitative account of what its current physics model
can and cannot resolve. `docs/known_limitations.md` lists the open items and the
order in which they should be addressed.

---

## Development history

This code is the multithreaded branch of a longer effort. The earlier serial
baseline, the migration work, and the snapshots taken along the way are documented
in `docs/history/`.

## License

MIT — see `LICENSE`.
