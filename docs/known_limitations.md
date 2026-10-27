# Known limitations and open items

State of the physics model as of 2026-10-27. The software pipeline is working and
reproducible; everything below concerns the model it runs, and is listed roughly in
the order it should be addressed.

## Blocking the science

1. **The reported efficiency is atmospheric, not magnetic.**
   The gun fires from 35 m through carbon dioxide at the Geant4 NIST default density
   of 1.842 mg/cm³, giving a column of 6.45 g/cm². A CSDA range calculation puts the
   cutoff at about 85 MeV; only ~6.7 % of the 1–500 MeV spectral-index-1.5 spectrum
   lies above it, i.e. about 67 protons per 1000. The measured 73 is consistent with
   that. Every reported number should be produced twice — field on and field off —
   and the magnetic contribution reported as the difference.

2. **The field is far too weak for this geometry.**
   Within the swept range a surviving proton is displaced by 2–30 mm against a 9 m
   habitat radius. Reaching real exclusion needs 10–30× the swept currents, i.e.
   10⁷–10⁸ A-turns. Sweeping the present 0.8–1.0 MA window more finely cannot
   produce a measurable effect.

3. **The topology is nearly blind to vertical incidence.**
   Near the axis the field of a horizontal loop is almost parallel to the velocity of
   a falling proton, so the Lorentz force is almost purely azimuthal: particles swirl
   around the habitat instead of being pushed out. Radial expulsion appears only at
   second order and scales as I². Measured at r = 9 m, 100 MeV: doubling the current
   doubles the azimuthal displacement but quadruples the radial one. The radial
   displacement is also inward. Reversing both coil polarities does not help.
   This is a design question, not a parameter question.

4. **The metric is a binary hit count, not dose.**
   A 1 MeV proton grazing the roof and a 500 MeV proton crossing the full volume
   count the same. Scoring energy deposition in the habitat would give a continuous
   quantity that responds to field changes a rare-event count cannot resolve.

5. **The primary generator is a converging pencil beam.**
   Every primary is launched from 35 m along `-direction`, so all of them are aimed
   at the origin. A physical particle environment is a uniform isotropic flux with
   all impact parameters present. The current generator maximises the hit count and
   removes exactly the marginal trajectories a field could redirect. The clamp
   `if (position.z() < 0.5*m) position.setZ(0.5*m)` additionally distorts the angular
   distribution near the horizon.

6. **Statistics.** 1000 events gives 73 ± 8.6 hits, i.e. ±0.9 % on efficiency.
   Resolving a 1 % design difference needs of order 10⁵–10⁶ events per configuration,
   or variance reduction, or a continuous metric. The 2026-04-24 baseline used 100
   events and recorded 1–9 hits per configuration; those efficiency values, and the
   apparent best configuration at 99 %, are counting noise.

## Model inconsistencies

7. **Swept radii are decoupled from the geometry.** `DetectorConstruction` hardcodes
   `shieldCoilMajorRadius = 20.0 m` and `cancelCoilMajorRadius = 9.0 m`, while the
   sweep varies the field-map radii over 18/20 m and 8/9 m. In half the runs the
   field comes from one radius and the physical coil body sits at another. Both
   should be driven from the same parameters.

8. **The geometry overlap warning is structural.** Both coil tori are placed in
   `World`, centred at z = 0 with a 0.25 m tube radius, so they necessarily straddle
   `Ground_PV` (z < 0) and `Air_PV` (z > 0), which are also World daughters. In
   addition `cancelCoilMajorRadius` exactly equals the habitat outer radius, so
   `CancelCoil_PV` and `Habitat_PV` overlap by construction.

9. **The field map is truncated at ±25 m inside an 80 m world.** `BFieldLookup`
   returns zero outside that box, but the real field there is still a few mT
   (≈3.4 mT at 35 m from the net dipole moment of 7.3×10⁸ A·m²). The gun fires from
   35 m, so the first ~10 m of every track crosses an unphysical discontinuity.

10. **1 m grid against a 1/d singularity.** Grid nodes fall as close as 2.8 cm from a
    conductor, so the map contains nodes at 5.7–7.2 T — correct at that distance, but
    trilinear interpolation then smears a multi-tesla value across a whole 1 m³ cell.
    The analytic field costs about 15 000 unique (r, z) evaluations per map, so an
    inline analytic field would be both cheaper and more accurate than the map.

11. **Hits are counted for any track, not only protons.** `SteppingAction` registers
    an entry into `Habitat_PV` for every track, including secondary neutrons and
    gammas produced in the ground and air, and the histogram titled "Kinetic Energy
    of Protons" is filled with whatever arrives. This likely explains the difference
    between the serial FTFP_BERT baseline (~5.1 % hits) and the multithreaded
    QGSP_BERT_HP branch (~7.3 %), since `_HP` neutron transport produces more
    low-energy secondaries.

12. **The `High (>500 MeV)` band is dead by construction**, because the generator
    caps the spectrum at 500 MeV. It was presumably designed for a galactic
    cosmic-ray spectrum.

## Engineering

13. **No seed control.** `run_study.mac` contains no `/random/setSeeds`, so every
    execution replays the identical 1000 protons. Useful as a paired comparison, but
    it must be an explicit documented choice, and multi-seed runs are needed before
    any uncertainty can be quoted.

14. **Results are parsed by regex from stdout.** A parse failure leaves
    `total_efficiency` as `None` while the hit counters default to 0, so a failed run
    can look like a perfect one. A machine-readable output file would remove the
    whole failure mode.

15. **`BFieldLookup::ReadFieldMap` warns on a row-count mismatch and continues** with
    a partially filled array; it should fail hard.

16. **In multithreaded mode each worker reads the map file and holds its own 3.2 MB
    copy.** Correct, but a shared const field object would be cleaner.

17. **`EventAction::AddHits(...)` declared in the header is dead code.**

## Suggested order of work

1. Decide the shield concept — accept that this topology addresses shallow arrivals
   only and reframe the question, or move to a topology whose field is transverse to
   the dominant arrival direction.
2. Replace the metric with dose scored in the habitat.
3. Replace the primary generator with a uniform isotropic flux.
4. Drive the coil geometry from the same parameters as the field map, and fix the
   overlaps.
5. Report every configuration as field-on minus field-off.
6. Then scan current logarithmically from 1 to 100 MA-turns to find the threshold
   where exclusion begins.
7. Add seed control and run several seeds per configuration.
8. Only then: optimisation and selection of a best configuration.
