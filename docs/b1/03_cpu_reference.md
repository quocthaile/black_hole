# B1 phase 3: CPU numerical reference R0

## Problem and current behavior

The historical CPU file is not a trustworthy oracle: it shares the GPU's unverified physical formulas, mixes double geodesic state with float camera vectors, and uses different ray and event rules. R0 is a separate C++17 implementation under `src/reference/` and does not depend on OpenGL rendering.

## State, equations, and units

`reference::State` contains `(r, psi, dr/dsigma)` in the ray's orbital plane. `reference::Ray` adds an orthonormal 3D radial/tangent basis, signed `L/E`, `E=1`, and affine position. `ModelParameters` contains dimensionless `r_s`, disk plane/annulus, and object spheres. `InitialConditions` contains camera radius, signed impact parameter, radial propagation sign, and the 3D plane basis. All lengths use one common coordinate unit, normally `L0=r_s`; all values and stages use `double`.

`initializeRay()` converts a local static-observer direction to the normalized Killing-energy convention. Given signed `b=L/E`, its transverse direction fraction is `b sqrt(f)/r0` and `dr/dsigma = sign * sqrt(1 - (b sqrt(f)/r0)^2)`. It rejects invalid radii, non-orthogonal/zero basis vectors, non-finite inputs, and invalid radial signs. With `E=1`, the null first integral is `rDot^2=1-f L^2/r^2`.

`geodesicRHS()` computes `psiDot=L/r^2` and `rDDot=L^2/r^3 (1-3r_s/(2r))`, obtained by differentiating the null first integral. `rk4Step()` performs the standard four evaluations and weighted update with one fixed step. It returns failure for a non-finite or non-positive intermediate radius. `positionAt()` maps the plane state back into 3D coordinates. `nullConstraintResidual()` evaluates the normalized metric constraint; E and L are held as conserved model parameters in this reduced formulation.

## Integration, events, and outputs

`integrate()` applies one fixed affine step per loop, then intersects the chord between old and new Cartesian positions with configured event surfaces. The event helper solves the segment/sphere quadratic for horizon, objects, and escape and solves a segment/plane crossing for a disk annulus. The smallest fraction wins, so overlapping events are classified by first intersection. Event state and affine parameter are linearly interpolated on that segment. Termination values distinguish `event_horizon`, `disk_hit`, `object_hit`, `escaped`, `maximum_steps`, and `invalid_state`.

The trajectory CSV records affine parameter, radius, orbital-plane angle, radial velocity, Cartesian coordinates, and null residual. The JSON sidecar records metric convention, dimensionless inputs, step/limits, and terminal outcome. Example:

```powershell
cmake --build build/msvc-vs2026 --config Release --target BlackHoleGeodesicReference
./build/msvc-vs2026/Release/BlackHoleGeodesicReference.exe `
  --rs 1 --camera-radius 10 --impact 0 --radial-sign -1 `
  --step 0.01 --max-steps 2000 --escape-radius 100 `
  --csv docs/b1/data/r0_radial_horizon.csv `
  --metadata docs/b1/data/r0_radial_horizon.json
```

The checked-in radial example contains 902 samples, ends at the segment-located horizon `r=1`, and records `event_horizon` after 901 fixed step attempts. Its last null residual is `null` in JSON because the static Schwarzschild-coordinate diagnostic is singular at the horizon; the CSV writes `inf` for that endpoint.

## Complexity, memory, and precision

Each accepted step has exactly four RHS evaluations, constant work per step, plus `O(M)` object-sphere checks and constant disk/horizon/escape checks. For `P` independent CPU rays the upper integration work is `O(P S (M+1))`. R0 stores a complete point per step for CSV output, using `O(S)` memory per ray; it is intentionally not optimized for throughput. GPU rendering remains independent of this storage.

Double precision reduces reference arithmetic error but does not remove fixed-step truncation, event chord/interpolation error, coordinate-model limitations, or input precision. The planar reduction avoids the spherical polar-axis singularity. Event positions are segment-linear approximations; near tangencies or two closely spaced surfaces may require smaller fixed steps. The event horizon is a first crossing reported in exterior Schwarzschild coordinates; no interior geodesic is computed.

## Implementation and verification

| Source | Responsibility |
| --- | --- |
| [`geodesic_reference.h`](../../src/reference/geodesic_reference.h) | State, parameters, event enums, and documented public functions |
| [`geodesic_reference.cpp`](../../src/reference/geodesic_reference.cpp) | Local-frame initialization, null RHS, RK4, segment events, CSV/JSON |
| [`reference_main.cpp`](../../src/reference/reference_main.cpp) | CLI configuration and reproducible output |
| [`reference_tests.cpp`](../../src/reference/reference_tests.cpp) | Analytic, convergence, invariant, event, and invalid-input checks |
| [`gpu_validation.cpp`](../../src/reference/gpu_validation.cpp) | OpenGL test runner that compares the production B1 compute shader against R0 |

Release x64 builds use Visual Studio 18 2026 / MSVC toolset 14.52.37025 and CMake 4.4.0-rc2. The standalone target `BlackHoleGeodesicReference` compiles without graphics dependencies; `BlackHoleGeodesicReferenceTests` is registered with CTest. The measured values and test configuration are frozen in `05_numerical_validation.md`.

## Remaining limitations

Energy and angular momentum are constants in the reduced equations, so their reported drift is zero by construction; this is not an independent conservation-drift measurement from a full eight-variable geodesic integrator. The null residual is the nontrivial numerical invariant check. A second independent implementation of the full state could strengthen future validation, but is not needed to correct the coordinate-axis issue in this baseline. The disk is an event surface and contributes no stress-energy to the metric.
