# Corrected fixed-step geodesic baseline B1

This directory records the B0 audit, Schwarzschild model derivation, independent CPU reference R0, GPU implementation, measured validation, and version identity. B0 remains the historical immutable comparison target. The scope excludes Adaptive Geodesic Marching, Dynamic LOD, and unrelated optimizations.

## Phase documents

| File | Contents |
| --- | --- |
| [01_baseline_audit.md](01_baseline_audit.md) | Starting commit/branch, B0 integrity, source pipeline, build and audit plan |
| [02_schwarzschild_model.md](02_schwarzschild_model.md) | Metric, Christoffel symbols, derived planar null equations, units, invariants and events |
| [03_cpu_reference.md](03_cpu_reference.md) | R0 implementation, state, configuration, CSV/JSON output and limitations |
| [04_gpu_fixedstep_implementation.md](04_gpu_fixedstep_implementation.md) | B1 shader, OpenGL data path, std140 layouts and runtime behavior |
| [05_numerical_validation.md](05_numerical_validation.md) | Frozen tolerances, analytical/convergence tests, GPU/R0 errors and status |
| [06_reference_freeze.md](06_reference_freeze.md) | B1 target, build/run instructions, manifest, hashes and reproduction record |

## Current status

B1 is **validated for the documented Schwarzschild exterior model and test configurations**, with explicit limits recorded in the validation and freeze documents. Release builds and CTest must pass on a compatible OpenGL 4.3 driver. The checked-in [radial R0 trajectory](data/r0_radial_horizon.csv) and its [metadata](data/r0_radial_horizon.json) provide one reproducible reference dataset. No Adaptive Marching, Dynamic LOD, Vulkan, or performance speedup is implemented or claimed.
