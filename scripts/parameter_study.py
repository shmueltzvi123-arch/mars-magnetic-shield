# -*- coding: utf-8 -*-

import csv
import itertools
import os
import re
import shutil
import subprocess
from datetime import datetime

from generate_magnetic_field_map import generate_field_map


# Root directory of MarsShield_Final
PROJECT_ROOT = os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))
)


def run_geant4_simulation(map_filepath, geant4_output_dir):
    """
    Run the Geant4 simulation for one magnetic-field map
    and return the detailed shielding statistics.
    """

    # DEV runner exposes MarsShield.exe through PATH.
    exe_path = shutil.which("MarsShield.exe")

    # Legacy/manual-build fallback.
    if exe_path is None:
        exe_path = os.path.join(
            PROJECT_ROOT,
            "build",
            "Release",
            "MarsShield.exe"
        )

    macro_path = os.path.join(
        PROJECT_ROOT,
        "run_study.mac"
    )

    os.makedirs(geant4_output_dir, exist_ok=True)

    # Tell the C++ code which magnetic-field map to load.
    env = os.environ.copy()
    env["MAG_FIELD_MAP"] = os.path.abspath(map_filepath)

    print(
        f"--- Starting Geant4: "
        f"{os.path.basename(map_filepath)} ---"
    )

    try:
        result = subprocess.run(
            [exe_path, macro_path],
            env=env,
            cwd=geant4_output_dir,
            capture_output=True,
            text=True,
            check=True
        )

        # Extract detailed shielding statistics.
        eff_total = re.search(
            r"Shielding Efficiency:\s+([\d.]+)\s+%",
            result.stdout
        )

        low_hits = re.search(
            r"Efficiency Low \(<100MeV\):\s+(\d+)\s+hits",
            result.stdout
        )

        med_hits = re.search(
            r"Efficiency Medium \(100-500MeV\):\s+(\d+)\s+hits",
            result.stdout
        )

        high_hits = re.search(
            r"Efficiency High \(>500MeV\):\s+(\d+)\s+hits",
            result.stdout
        )

        sim_data = {
            "total_efficiency":
                float(eff_total.group(1))
                if eff_total else None,

            "low_energy_hits":
                int(low_hits.group(1))
                if low_hits else 0,

            "med_energy_hits":
                int(med_hits.group(1))
                if med_hits else 0,

            "high_energy_hits":
                int(high_hits.group(1))
                if high_hits else 0
        }

        print(
            f"--- Result: "
            f"{sim_data['total_efficiency']}% efficiency "
            f"| Low={sim_data['low_energy_hits']} "
            f"| Medium={sim_data['med_energy_hits']} "
            f"| High={sim_data['high_energy_hits']} ---"
        )

        return sim_data

    except subprocess.CalledProcessError as e:
        print(f"--- Error running Geant4: {e} ---")
        return None


def run_parameter_study():
    # Parameter ranges.
    R_shield_values = [18.0, 20.0]
    I_shield_values = [8e5, 1e6]
    R_cancel_values = [8.0, 9.0]
    I_cancel_values = [-4e5, -4.5e5]

    # Every execution receives its own result directory.
    timestamp = datetime.now().strftime(
        "%Y-%m-%d_%H-%M-%S"
    )

    run_root = os.path.join(
        PROJECT_ROOT,
        "results",
        "runs",
        timestamp
    )

    maps_dir = os.path.join(
        run_root,
        "maps"
    )

    geant4_dir = os.path.join(
        run_root,
        "geant4"
    )

    output_csv = os.path.join(
        run_root,
        "study_results_detailed.csv"
    )

    os.makedirs(maps_dir, exist_ok=True)
    os.makedirs(geant4_dir, exist_ok=True)

    all_results = []

    all_combinations = list(
        itertools.product(
            R_shield_values,
            I_shield_values,
            R_cancel_values,
            I_cancel_values
        )
    )

    print(f"Results directory: {run_root}")

    for i, (Rs, Is, Rc, Ic) in enumerate(
        all_combinations,
        1
    ):
        print("\n" + "=" * 60)

        print(
            f"Simulation Run "
            f"{i} of {len(all_combinations)}"
        )

        # 1. Generate field map.
        summary = generate_field_map(
            R_shield=Rs,
            I_shield=Is,
            R_cancel=Rc,
            I_cancel=Ic,
            output_dir=maps_dir,
            run_id=i
        )

        # 2. Give each Geant4 execution its own folder.
        current_geant4_dir = os.path.join(
            geant4_dir,
            f"run_{i:03d}"
        )

        sim_data = run_geant4_simulation(
            summary["filepath"],
            current_geant4_dir
        )

        # 3. Store detailed simulation data.
        if sim_data:
            summary.update(sim_data)
        else:
            summary.update({
                "total_efficiency": None,
                "low_energy_hits": 0,
                "med_energy_hits": 0,
                "high_energy_hits": 0
            })

        all_results.append(summary)

    # Save detailed master CSV.
    if all_results:
        fieldnames = list(all_results[0].keys())

        with open(
            output_csv,
            "w",
            newline="",
            encoding="utf-8"
        ) as f:
            writer = csv.DictWriter(
                f,
                fieldnames=fieldnames
            )

            writer.writeheader()
            writer.writerows(all_results)

    print("\n" + "=" * 60)

    print(
        "Automation Complete.\n"
        f"Results saved to:\n{run_root}"
    )


if __name__ == "__main__":
    run_parameter_study()