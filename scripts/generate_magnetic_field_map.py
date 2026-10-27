import os
import time
import numpy as np
from scipy.special import ellipk, ellipe


MU0 = 4 * np.pi * 1e-7


def single_coil_field(R, I, r, z):
    """
    Compute magnetic field components (Br, Bz) of a single circular current loop
    in cylindrical coordinates at point (r, z).

    Parameters:
        R (float): Coil radius [m]
        I (float): Coil current [A]
        r (float): Radial distance from z-axis [m]
        z (float): Axial coordinate [m]

    Returns:
        tuple[float, float]: (Br, Bz)
    """
    # Special case: on-axis formula
    if r < 1e-5:
        bz = (MU0 * I * R**2) / (2 * (R**2 + z**2) ** 1.5)
        return 0.0, bz

    alpha = R**2 + r**2 + z**2 - 2 * R * r
    beta = R**2 + r**2 + z**2 + 2 * R * r

    # Safety checks
    if alpha <= 0 or beta <= 0:
        return 0.0, 0.0

    k_sq = 4 * R * r / beta

    # Numerical protection near singular region
    if k_sq >= 1.0 - 1e-8:
        return 0.0, 0.0

    K = ellipk(k_sq)
    E = ellipe(k_sq)

    factor = MU0 * I / (2 * np.pi * np.sqrt(beta))

    bz = factor * (K + E * (R**2 - r**2 - z**2) / alpha)
    br = factor * (z / r) * (-K + E * (R**2 + r**2 + z**2) / alpha)

    return br, bz


def get_b_field_optimized(x, y, z, R_shield, I_shield, R_cancel, I_cancel, calculation_cache):
    """
    Compute total magnetic field (Bx, By, Bz) from two coils at point (x, y, z),
    using caching in (r, z) space.

    Returns:
        tuple[float, float, float]: (Bx, By, Bz)
    """
    r = np.sqrt(x**2 + y**2)

    # Rounded key for cache stability
    r_key = round(r, 4)
    z_key = round(z, 4)
    cache_key = (r_key, z_key)

    if cache_key in calculation_cache:
        Br_total, Bz_total = calculation_cache[cache_key]
    else:
        Br1, Bz1 = single_coil_field(R_shield, I_shield, r, z)
        Br2, Bz2 = single_coil_field(R_cancel, I_cancel, r, z)

        Br_total = Br1 + Br2
        Bz_total = Bz1 + Bz2

        calculation_cache[cache_key] = (Br_total, Bz_total)

    if r < 1e-5:
        Bx = 0.0
        By = 0.0
    else:
        Bx = Br_total * (x / r)
        By = Br_total * (y / r)

    return Bx, By, Bz_total


def build_map_filename(run_id, R_shield, I_shield, R_cancel, I_cancel):
    """
    Build a readable output filename for the field map.
    """
    return (
        f"map_run_{run_id:03d}"
        f"_Rs_{R_shield:.2f}"
        f"_Is_{I_shield:.2e}"
        f"_Rc_{R_cancel:.2f}"
        f"_Ic_{I_cancel:.2e}.txt"
    )


def generate_field_map(
    R_shield,
    I_shield,
    R_cancel,
    I_cancel,
    output_dir="maps",
    filename=None,
    run_id=1,
    step=1.0,
    x_min=-25,
    x_max=25,
    y_min=-25,
    y_max=25,
    z_min=-25,
    z_max=25,
):
    """
    Generate a magnetic field map file for the two-coil system.

    Parameters:
        R_shield, I_shield, R_cancel, I_cancel : coil parameters
        output_dir (str): directory where map will be saved
        filename (str | None): optional custom file name
        run_id (int): run identifier used if filename is not provided
        step (float): grid spacing
        x_min, x_max, y_min, y_max, z_min, z_max (float): grid bounds

    Returns:
        dict: summary information about the generated map
    """
    start_time = time.time()
    calculation_cache = {}

    os.makedirs(output_dir, exist_ok=True)

    if filename is None:
        filename = build_map_filename(run_id, R_shield, I_shield, R_cancel, I_cancel)

    filepath = os.path.join(output_dir, filename)

    x_range = np.arange(x_min, x_max + 0.1, step)
    y_range = np.arange(y_min, y_max + 0.1, step)
    z_range = np.arange(z_min, z_max + 0.1, step)

    total_points = len(x_range) * len(y_range) * len(z_range)

    print(f"[Run {run_id}] Starting Grid Generation")
    print(f"[Run {run_id}] Grid: {len(x_range)} x {len(y_range)} x {len(z_range)} = {total_points} points")
    print(f"[Run {run_id}] Output: {filepath}")

    with open(filepath, "w") as f:
        f.write("X Y Z Bx By Bz\n")

        for x in x_range:
            for y in y_range:
                for z in z_range:
                    bx, by, bz = get_b_field_optimized(
                        x, y, z,
                        R_shield, I_shield,
                        R_cancel, I_cancel,
                        calculation_cache
                    )
                    f.write(f"{x:.2f} {y:.2f} {z:.2f} {bx:.6e} {by:.6e} {bz:.6e}\n")

    elapsed_sec = time.time() - start_time
    unique_calculations = len(calculation_cache)

    print(f"[Run {run_id}] Done!")
    print(f"[Run {run_id}] Unique calculations: {unique_calculations}")
    print(f"[Run {run_id}] Time: {elapsed_sec:.2f} sec")

    summary = {
        "run_id": run_id,
        "R_shield": R_shield,
        "I_shield": I_shield,
        "R_cancel": R_cancel,
        "I_cancel": I_cancel,
        "step": step,
        "x_min": x_min,
        "x_max": x_max,
        "y_min": y_min,
        "y_max": y_max,
        "z_min": z_min,
        "z_max": z_max,
        "total_points": total_points,
        "unique_calculations": unique_calculations,
        "elapsed_sec": elapsed_sec,
        "output_dir": output_dir,
        "filename": filename,
        "filepath": filepath,
    }

    return summary


def main():
    summary = generate_field_map(
        R_shield=20.0,
        I_shield=1e6,
        R_cancel=9.0,
        I_cancel=-4.5e5,
        output_dir="maps",
        filename=None,
        run_id=1,
        step=1.0,
        x_min=-25,
        x_max=25,
        y_min=-25,
        y_max=25,
        z_min=-25,
        z_max=25,
    )

    print("\n--- Run Summary ---")
    for key, value in summary.items():
        print(f"{key}: {value}")


if __name__ == "__main__":
    main()
