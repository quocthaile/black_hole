# B1 phase 1: baseline audit and reproducibility

## Repository identity

| Item | Observed value |
| --- | --- |
| Repository | `https://github.com/quocthaile/black_hole` |
| Starting branch | `research/blackhole-baseline` |
| Fetched `origin/research/blackhole-baseline` | `3c084b4d742207d83c7bb2b874358c30854729d3` |
| Starting commit | `3c084b4d742207d83c7bb2b874358c30854729d3` |
| Development branch | `research/blackhole-b1` |
| Starting worktree | Clean |

The fetched remote branch and `FETCH_HEAD` both resolved to the starting commit. This is a fork development snapshot, while the B0 file manifest identifies the historical upstream code as `kavan010/black_hole` commit `dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317`. These are distinct identities.

## Existing build and source layout

`CMakeLists.txt` requires C++17 and uses the existing `add_black_hole_executable()` and `add_black_hole_shaders()` helpers. It defines `BlackHole3D`, `BlackHole3D_baseline`, and `BlackHole2D_baseline`; only the first development target uses `src/black_hole.cpp` and `shaders/`. The baseline executable uses `src/baseline/black_hole.cpp` and stages its shaders into a separate target directory. `CMakePresets.json` selects Visual Studio 18 2026 x64, and `vcpkg.json` pins the registry baseline `3855d4d6b52353ee2e2f73044327e59d01a18c84` with GLFW, GLEW, and GLM.

The development `Engine` requests an OpenGL 4.3 core context, owns camera/disk/object UBOs and an RGBA8 output texture, and compiles `geodesic.comp` at runtime from its executable working directory. Each compute invocation samples one of 200 x 150 rays; local workgroups are 16 x 16, so bounds checks reject extra invocations. The host uploads camera basis/FOV at binding 1, disk parameters at binding 2, and scene arrays at binding 3. The shader writes image unit 0; the host follows dispatch with `GL_SHADER_IMAGE_ACCESS_BARRIER_BIT` and draws a fullscreen textured quad. The grid is generated and uploaded on the CPU each frame.

The original scene uses SI-length coordinates, `G=6.67430e-11`, `c=299792458`, a black-hole mass of `8.54e36 kg`, two spherical objects, an 800 x 600 window, and a 200 x 150 compute image. The camera orbits the origin with a 60-degree vertical field of view. The B0 M0 protocol is separately documented in `benchmarks/` and is preserved.

## Original numerical behavior in the development copy

The starting development shader matches the locked development-shader hash at this revision. Its state stores Cartesian position plus spherical `r, theta, phi` and derivatives as `float`. `initRay()` projects a camera direction into spherical derivatives and stores `L = r^2 sin(theta) dphi`, but the RHS does not use `L`; its energy initialization omits a radial metric factor that is required by the null constraint. The radial acceleration uses `r` instead of `f r` in its angular term. `rk4Step()` has one RHS call and is Forward Euler. It advances Cartesian coordinates from spherical state after each step.

The shader integrates with `D_LAMBDA=1e7`, at most 60,000 steps, and an escape radius `1e30`. It tests the horizon before stepping, then checks a sign change in Cartesian `y` using the new endpoint's disk radius, then object containment at the endpoint, then escape. A step can cross multiple events without locating the first one. A non-finite state has no explicit status and can fall through to a transparent miss. Shader `std140` declarations include a tightly packed scalar mass array; the host packs it as tightly packed floats even though `std140` scalar arrays have 16-byte stride. The object masses are currently unused by the shader. Camera packing includes a C++ `bool` and padding; this is not a defined matching `std140` contract. The current barrier covers image access but not explicitly texture fetch.

The independent archived `src/baseline/CPU-geodesic.cpp` has a four-stage RK4 structure but shares the unverified physical equations and uses float camera vectors. It is not adopted as R0. The B0 compute shader is single-stage Euler and remains immutable.

## Integrity and reproducibility checks before changes

Commands run from the repository root:

```powershell
git fetch origin research/blackhole-baseline
git rev-parse FETCH_HEAD
git rev-parse origin/research/blackhole-baseline
python benchmarks/scripts/verify_baseline.py
python -m unittest discover -s benchmarks/scripts -p 'test_*.py' -v
cmake --build build/msvc-vs2026 --config Release --target BlackHole3D BlackHole3D_baseline
```

Results: fetched branch and `FETCH_HEAD` both resolved to `3c084b4d742207d83c7bb2b874358c30854729d3`; integrity verification passed for all seven B0 files and three locked development shaders; all 11 M0 unit tests passed; both Release x64 targets built successfully with Visual Studio 18 2026, MSVC toolset 14.52.37025, CMake 4.4.0-rc2, and the existing x64-windows vcpkg packages.

No new visual-equivalence, numerical, or GPU runtime claim is made by this audit. The existing M0 record documents prior smoke tests and limitations. B1 GPU output and physical behavior remain to be validated in the later phases.

## Plan grounded in the audit

1. Preserve the locked B0 source and its existing target unchanged.
2. Specify a Schwarzschild exterior model in dimensionless `r/r_s` coordinates and derive a planar geodesic solver using spherical symmetry, avoiding the polar-axis coordinate singularity in the original six-state formulation.
3. Implement R0 in standard C++17/double with fixed-step RK4, explicit event statuses, CSV/metadata output, analytic and convergence tests.
4. Port the same reduced state and camera local-frame initialization to a separate B1 compute shader, correcting UBO packing and image-to-texture synchronization only where required.
5. Compare R0 and B1 through reproducible trajectory and event outputs if the local OpenGL driver/runtime permits readback; otherwise label the GPU comparison blocked and do not label B1 validated.

The actual mathematical convention and parameters are frozen in `02_schwarzschild_model.md` before implementation. Hardware-dependent runtime and display equivalence are not inferred from a successful build.
