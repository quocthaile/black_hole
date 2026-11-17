# B1 phase 5: numerical validation record

## Frozen configurations and tolerances

The tolerances below were set before the final listed test run. The CPU tests use MSVC Release x64 / C++17 / double; GPU checks use the production B1 GLSL source, a hidden OpenGL 4.3 context, one diagnostic ray, and R0 double output. Schwarzschild coordinates use `r_s=1` unless a flat-space limit sets `r_s=0`. The GPU compute test is deterministic and uses no random sampling.

| Metric/test | Frozen acceptance threshold | Reason |
| --- | --- | --- |
| Initial null residual | `< 2e-15` | Double arithmetic and local-frame algebra |
| Radial horizon radius and affine location | `|r-1| < 1e-12`; `|sigma-9| < 1e-10` | Exact radial null solution from `r0=10` |
| Photon sphere radial drift | `< 1e-14` per step | Exact equilibrium at `r=1.5`, `|L|=3 sqrt(3)/2` |
| Flat-space impact distance | `< 5e-4` | Smooth RK4 path at fixed `h=0.01` |
| Leading weak-field deflection | relative error `< 8%` | `b/r_s=30`; first-order asymptotic formula plus finite camera radius |
| Critical impact classification | `b=2.50` captured; `b=2.70` scattered | Brackets `b_crit=3 sqrt(3)/2=2.598076...` |
| RK4 refinement | first halving error ratio `>10` | Smooth interval; fourth-order target is about 16 before the double-precision floor |
| R0 maximum null residual | `< 2e-5` | Allows accumulated fixed-step truncation on smooth exterior trajectory |
| GPU radial max state error | `< 2e-4` | GLSL float versus CPU double near the horizon event |
| GPU general 3D max state error | `< 1e-2` | Accumulated float error over an off-axis scatter to `r=100` |
| GPU general null residual | `< 1e-2` | Single precision metric residual across the exterior path |
| GPU event classification | Exact class match | Deterministic surface cases |
| GPU event affine location | `< 0.01` | At most one `h=0.01` step, with linear segment event interpolation |
| GPU maximum-step and invalid-state handling | Maximum-step status and magenta invalid marker | Explicit one-step shader variant and singular camera input |

## R0 test results

Executed `BlackHoleGeodesicReferenceTests.exe` from the Release build. All 10 groups passed: local null initialization; exact radial geodesic/horizon location; photon sphere; flat limit and 3D plane orientation; weak-field deflection; sub/supercritical capture; disk and object surfaces; RK4 refinement; null constraint and conserved parameters; invalid and maximum-step statuses.

| Quantity | Measured result | Status |
| --- | ---: | --- |
| Weak-field deflection, `r_s=1`, `b=30`, `r0=200` | `0.0701551 rad` | PASS |
| Leading prediction `2 r_s/b` | `0.0666667 rad` | Comparison value |
| Relative leading-order deflection error | `5.23269%` | PASS (`<8%`) |
| R0 max null residual, `h=0.01`, exterior smooth run | `6.67567e-14` | PASS |
| Radial analytic horizon radius error | `<1e-12` | PASS |
| Radial analytic affine error | `<1e-10` | PASS |
| Object entry radius error | `<1e-10` | PASS |

RK4 final-state error is the sum of absolute `(r, psi, dr/dsigma)` differences at the common affine time `sigma=8`, against a fixed-step `h=0.00125` comparison trajectory. The first three columns are measured, not estimated from formal order.

| Step size `h` | Final-state error vs `h=0.00125` | Error ratio on halving |
| ---: | ---: | ---: |
| 0.16 | `4.58010e-10` | — |
| 0.08 | `2.86902e-11` | `15.964` |
| 0.04 | `1.81935e-12` | `15.7695` |
| 0.02 | `1.54765e-13` | `11.7556` |
| 0.00125 | reference | — |

The first two halving ratios are close to the classical RK4 value 16. The final ratio is reduced as the measured error approaches the double-precision/reference floor; it is not presented as an order-four measurement.

Energy and plane angular momentum are parameter invariants and are held fixed by the reduced equations: measured drift is `0` by construction, not an independent drift measurement. The nontrivial invariant check is the null residual above. This limitation is documented rather than hidden behind a nominal zero error.

## GPU versus R0

Executed `BlackHoleB1GpuValidation.exe` on the tested Intel UHD Graphics / OpenGL 4.3 environment. It compiled and dispatched `shaders/b1/geodesic.comp` itself with diagnostics enabled only for the harness.

| GPU/R0 comparison | Measured result | Threshold/status |
| --- | ---: | --- |
| Radial horizon max state error | `1.37100e-4` | `<2e-4`, PASS |
| Radial horizon affine event error | `1.18256e-4` | `<0.01`, PASS |
| General 3D-oriented trajectory max state error | `1.79441e-3` | `<0.01`, PASS |
| General trajectory GPU max null residual | `1.03607e-5` | `<0.01`, PASS |
| Maximum affine event-location error over horizon/disk/object/escape cases | `4.86755e-3` | `<0.01`, PASS |
| Horizon, disk, object, escape classifications | `4/4` exact matches | PASS |
| Maximum-step termination | `maximum_steps`, 1 step | PASS |
| Invalid GPU state | Magenta invalid marker, not transparent escape | PASS |
| General trajectory | 11,170 GPU steps to escape | Same step count as R0 within one event-boundary step |

The GPU/R0 radial endpoint differs by one attempted step in some floating-point runs because CPU double and GPU float reach the horizon boundary on opposite sides of an exact fixed-step endpoint. Event location is compared separately and passes the stricter affine-location threshold. The general 3D test rotates the local transverse basis into y-z and compares the common scalar orbital-plane state plus escape event; it does not claim a pixel-image comparison.

## Error sources and limits

The comparison separates fixed-step RK4 truncation (CPU refinement table), event-chord error (event fraction and affine comparison), double-versus-float trajectory error (GPU/R0 state differences), and coordinate/model error (Schwarzschild exterior, static camera, event-surface scene convention). The weak-field comparison uses a leading-order formula, not an exact finite-radius closed form. The near-critical GPU classification is tested through a subcritical capture and a supercritical 3D scatter; no claim is made about arbitrarily close finite-precision critical tuning.

The OpenGL tests validate the compute shader's numerical output for selected deterministic rays and validate event classes. They do not compare a rendered image against a scientific image dataset, test all 30,000 pixels exhaustively, profile runtime, or establish visual equivalence with B0. The B1 result is therefore validated only within the documented equations, precision, events, thresholds, and tested configurations.

## Commands and evidence

```powershell
cmake --build build/msvc-vs2026 --config Release --target BlackHoleGeodesicReferenceTests BlackHoleB1GpuValidation
ctest --test-dir build/msvc-vs2026 -C Release --output-on-failure
./build/msvc-vs2026/Release/BlackHoleGeodesicReferenceTests.exe
./build/msvc-vs2026/Release/BlackHoleB1GpuValidation.exe
python benchmarks/scripts/verify_baseline.py
```

The measured trajectory dataset is [`data/r0_radial_horizon.csv`](data/r0_radial_horizon.csv), with [`data/r0_radial_horizon.json`](data/r0_radial_horizon.json). Runtime/window smoke output is recorded in `06_reference_freeze.md`; no frame-time or speedup data was collected.
