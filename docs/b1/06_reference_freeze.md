# B1 phase 6: reference freeze and reproducibility

## Frozen source identity

| Item | Value |
| --- | --- |
| Repository | `https://github.com/quocthaile/black_hole` |
| Starting commit | `3c084b4d742207d83c7bb2b874358c30854729d3` |
| Development branch | `research/blackhole-b1` |
| B1 source snapshot commit | `2a63277f60c05800e5e1861f8bd7e7d1b7ebd29a` |
| B0 manifest | `benchmarks/baseline_manifest.json` (unchanged) |
| B1 build target | `BlackHole3D_B1` |
| B1 runtime shaders | `shaders/b1/{geodesic.comp,grid.vert,grid.frag}` |

`src/b1/black_hole.cpp` and the three B1 runtime shaders were committed with the separate target at the source snapshot commit. Subsequent commits add validation records, configuration, and hashes; they do not modify those frozen source files. `BlackHole3D` remains the development target. Its runtime also uses the B1 shader folder. The root `shaders/` files retain the previous M0 lock so the existing B0 integrity checker and manifest remain intact.

The versioned configuration is [`config/b1.json`](config/b1.json). SHA-256 source identities and target names are recorded in [`b1_manifest.json`](b1_manifest.json). The original B0 identity and lock were not overwritten.

## Frozen numerical configuration

| Parameter | B1 value |
| --- | --- |
| Metric | Schwarzschild, signature `(-,+,+,+)` |
| Coordinate scale | Schwarzschild radius `r_s`; dimensionless exterior coordinates |
| Camera frame | Static orthonormal observer outside `r_s` |
| Propagation | Camera toward scene with increasing normalized affine parameter |
| Conserved energy | `E=1` |
| Ray sampling | 200 x 150 pixels (30,000 rays), fixed |
| Compute workgroup | 16 x 16 |
| Affine step | `h=0.01`, fixed |
| Maximum steps | 60,000 per ray |
| Escape radius | `100 r_s` |
| Disk annulus | `2.2 r_s` to `5.2 r_s`, plane normal `(0,1,0)` |
| Event rule | Earliest segment intersection among horizon, disk, objects, escape |
| GPU precision | GLSL `float` |
| CPU reference precision | C++ `double` |

No Adaptive Geodesic Marching or Dynamic LOD is part of this snapshot.

## Build, run, and reproduce

Build the frozen executable and independent checks with the existing MSVC/vcpkg preset:

```powershell
cmake --preset msvc-vs2026 -B build/msvc-vs2026
cmake --build build/msvc-vs2026 --config Release --target BlackHole3D_B1 BlackHole3D_baseline BlackHoleGeodesicReference BlackHoleGeodesicReferenceTests BlackHoleB1GpuValidation
ctest --test-dir build/msvc-vs2026 -C Release --output-on-failure
python benchmarks/scripts/verify_baseline.py
python -m unittest discover -s benchmarks/scripts -p 'test_*.py' -v
```

Run the executable from its own staged folder so the target-specific shader copies resolve:

```powershell
Push-Location build/msvc-vs2026/bin/Release/BlackHole3D_B1
try { ./BlackHole3D_B1.exe } finally { Pop-Location }
```

The standalone R0 CLI writes repeatable CSV and JSON outputs; the frozen radial example and metadata are in [`data/`](data/). GPU/R0 comparison and validation commands are detailed in [`05_numerical_validation.md`](05_numerical_validation.md).

## Tested environment and evidence

| Component | Observed value |
| --- | --- |
| OS | Windows 11 Pro 10.0.26300, 64-bit |
| CPU | 13th Gen Intel Core i5-13420H |
| GPU | Intel UHD Graphics, driver 32.0.101.7085 |
| Graphics API | OpenGL 4.3.0 (`32.0.101.7085`) |
| Compiler | MSVC 19.52.37029.0, toolset 14.52.37025, x64 |
| Generator / SDK | Visual Studio 18 2026 / Windows SDK 10.0.28000.0 |
| CMake | 4.4.0-rc2 |
| Python | 3.14.3 |
| vcpkg triplet | `x64-windows` |
| Main dependencies | GLEW 2.3.1, GLFW 3.4#1, GLM 1.0.3 |

Executed on this environment:

- Release build of all configured targets, including B0, development B1, frozen B1, R0 and the 2D baseline: PASS.
- CTest CPU and GPU suites: 2/2 PASS. GPU trajectory, event, max-step, and invalid-state measurements are recorded in `05_numerical_validation.md`.
- Existing B0 and locked development-shader integrity check: PASS (7 B0 files and 3 locked root shaders).
- Existing M0 Python unit tests: 11/11 PASS; PowerShell capture script parser: PASS.
- `BlackHole3D_B1.exe` launch from its own target directory: process remained running for four seconds with no stderr output. A hidden-window smoke process was then stopped by the runner. The GPU validation executable independently created an OpenGL 4.3 context, compiled the exact staged B1 shader source, dispatched it, read diagnostic output, and passed numerical/event checks.

No screenshot or visual-equivalence judgment was recorded. No GPU frame time, FPS, speedup, PresentMon capture, or performance bottleneck was measured. The executable's local build SHA-256 is machine/build-specific; the frozen source hashes, rather than the binary hash, define B1 identity.

## Limits and future comparison

The validation supports B1 for the specified Schwarzschild exterior model, local camera initialization, fixed-step settings, and deterministic test configurations. E and plane angular momentum are constants in the reduced solver and shader state, so their reported drift is zero by construction; the measured dynamic invariant is null-constraint residual. Event locations use segment interpolation, GPU arithmetic is float, and no interior horizon evolution is provided. See the exact tolerances, measured errors, and test limitations in [`05_numerical_validation.md`](05_numerical_validation.md).

Future Adaptive Geodesic Marching and Dynamic Ray/Resolution LOD work should retain `BlackHole3D_B1`, the R0 tests, B1 configuration, validation dataset, and source hashes as their comparison reference. The B0 target remains useful for historical performance and physical/numerical differences; B0-to-B1 differences must not be attributed solely to RK4 integration error.
