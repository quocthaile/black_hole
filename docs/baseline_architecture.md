# B0 architecture at the M0 fork snapshot

Scope: `origin/main` commit `0f74de4ae0fcb6436f95a7a5cd709aa023c93f67`.
This audit supports B-01/B-03 and the initial FPS/Frame Time part of E-01/E-02
in [AGENT_REQUIREMENTS.md](AGENT_REQUIREMENTS.md) (proposal pages 2-4, 6-7).
It does not complete the wider numerical-reference or profiling requirements.
The user explicitly limits this branch to baseline preservation, style and M0.

## Identity and build facts

All seven files in `src/baseline/` have the same raw Git blobs as the corresponding
root files at upstream [`dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317`](https://github.com/kavan010/black_hole/tree/dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317).
They were compared as bytes, not inferred from names. The fork's original snapshot
is that same commit; its reorganized snapshot is the M0 base above. Exact raw
SHA-1/SHA-256 and normalized hashes are in
[`baseline_manifest.json`](../benchmarks/baseline_manifest.json).
The entire baseline directory remains immutable, including the non-built sources.

| CMake target | Source | Runtime shaders |
| --- | --- | --- |
| `BlackHole3D` | `src/black_hole.cpp` | `shaders/{geodesic.comp,grid.vert,grid.frag}` |
| `BlackHole3D_baseline` | `src/baseline/black_hole.cpp` | the same filenames in `src/baseline/` |
| `BlackHole2D_baseline` | `src/baseline/2D_lensing.cpp` | none |

`CPU-geodesic.cpp` and `ray_tracing.cpp` are historical sources without targets.
There is no `src/reference/` or new simulation target.

`CMakeLists.txt` requires C++17, links OpenGL/GLFW/GLEW/GLM independently for each
target and adds `/utf-8` on MSVC. It does not set special optimization flags.
`CMakePresets.json` selects Visual Studio 18 2026, x64, `x64-windows` and the
toolchain under `VCPKG_ROOT`; using this preset requires CMake 4.2+, although the
plain CMake project minimum is 3.21. `vcpkg.json` pins registry baseline
`3855d4d6b52353ee2e2f73044327e59d01a18c84`. Actual resolved packages/compiler are
recorded with captures; upstream's manifest had the same three dependencies but
no registry baseline, and its CMake layout/target names differed.

`add_black_hole_executable()` places binaries in `<build>/bin/<config>/<target>/`
and sets that directory as the Visual Studio debugger working directory.
`add_black_hole_shaders()` stages three files via a dependency target on every
build and also after linking. Each 3D executable has a separate runtime directory.
The load functions open **relative filenames**: launching from the repository root
does not locate these files. CMake and both shader sets are unchanged in M0.

## Startup and initial scene (confirmed from source)

The two 3D C++ sources were identical at the base. `Camera`, `SagA`, `objects` and
`Engine engine` are globals; `Engine::Engine()` initializes OpenGL before `main()`.
It requests a 4.3 core context, initializes GLEW, compiles the embedded GLSL 330
screen shaders, GLSL 330 grid pair and GLSL 430 compute shader, and allocates
UBOs, quad geometry and an RGBA8 texture. Grid and compute compile/link failures
exit with diagnostics. The embedded screen program has no status checks.

The window is requested at 800 x 600; compute output is 200 x 150 (30,000 rays).
Neither dimension changes when `camera.moving` changes in the original settings.
The shader uses 16 x 16 local groups; 13 x 10 workgroups launch 33,280 invocations,
of which bounds checks exclude the excess. OS/DPI framebuffer size must be
observed for a capture: these are source settings, not a runtime size query.
The code does not handle resizing by updating the fixed `WIDTH`/`HEIGHT`.

`Camera::position()` uses radius `6.34194e10f`, azimuth zero, elevation `M_PI/2.0f`
and an origin target; the initial position is approximately on the positive x
axis. View FOV is 60 degrees. Gravity starts false. There are three scene objects:
yellow/red spheres at `(4e11,0,0)` and `(0,0,4e11)`, each radius `4e10` and mass
`1.98892e30`, plus the origin black hole with mass `8.54e36` and CPU-computed
radius `2*G*mass/(c*c)`. CPU constants are `G=6.67430e-11`, `c=299792458`.
`uploadDiskUBO()` supplies radii `2.2*r_s`, `5.2*r_s`, `disk_num=2` and thickness
`1e9`; the last two parameters do not affect the current shader intersections.
M0 requires no mouse/key/scroll interaction, no resize and gravity remaining off.

## Per-frame data flow and ownership

1. `main()` clears the frame and computes an elapsed `dt` (unused by the original
   gravity updates). The nested object loops calculate distances/accelerations
   even when gravity is off; object motion is guarded by `Gravity`.
2. `generateGrid(objects)` builds 676 CPU vertices (26 x 26) and 2,500 indices
   for the original 25 x 25 grid. It allocates temporary vectors, warps vertex
   height for every object, and uploads vertices/indices using `glBufferData`.
   Grid VAO/VBO/EBO handles are created on the first call and reused thereafter.
3. `drawGrid(viewProj)` draws lines with alpha blending and depth disabled, then
   enables depth again. It precedes the compute/screen pass, despite the original
   comment describing it as an overlay.
4. `dispatchCompute()` calls `glTexImage2D` every frame, updates camera/disk/object
   UBOs with `glBufferSubData`, binds image unit 0 and dispatches the compute shader.
   Binding 1 holds the camera basis/FOV; binding 2 the disk; binding 3 up to 16
   object positions/radii/colors/masses. These are **UBOs, not SSBOs**. `objects`
   remains the CPU source of scene state. There is no trajectory readback.
5. Each compute invocation initializes one ray, integrates and tests events,
   and stores one RGBA8 pixel. `glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)`
   follows dispatch. The later texture sampling synchronization deserves a future
   audit; the original barrier is retained exactly, without claiming sufficiency.
6. `drawFullScreenQuad()` samples the texture with linear filtering over the grid,
   with the blend state left active. The historical call draws six vertices as a
   triangle strip. Then `glfwSwapBuffers()` presents and `glfwPollEvents()` processes
   input. The separate `renderScene()` helper is not called by `main()`.

Engine handles own the GPU resources for the process lifetime. Program/texture/
buffer cleanup is not explicit for every resource; window/context destruction and
GLFW termination occur at exit. M0 does not refactor this lifetime management.

The 3D source initializes `Clock`, `lastPrintTime`, `framesCount` and a CPU pixel
vector, but its active loop does **not** update/print FPS. Its unused CPU variable
`numSteps=80000` is not the ray integration limit. No `glfwSwapInterval()` call
exists. The driver/compositor may affect presentation; absence of the call does
not establish VSync off. External M0 intervals are not GPU execution times.

## Numerical configuration (unchanged)

`geodesic.comp::main()` uses `D_LAMBDA=1e7`, 60,000 maximum steps, shader
`SagA_rs=1.269e10` and `ESCAPE_R=1e30`. It checks the horizon before the step,
then disk crossing, object containment and escape after the step. The state
is spherical plus Cartesian position; `Ray` fields are float, with isolated
double declarations elsewhere in the shader. The GPU function called `rk4Step`
actually makes a single RHS evaluation. See [the numerical audit](baseline_numerical_audit.md).

## Candidates for future profiling, not measured bottlenecks

Source suggests investigating per-frame vector allocation/grid reconstruction,
buffer uploads, texture storage redefinition, repeated uniform-location lookup,
work in the gravity-disabled loop, long ray loops with different exit times,
and presentation waits. Grid work is roughly proportional to vertices times
objects; worst-case ray work includes up to `P*S*M` object containment tests.
These counts alone do not establish which stage limits frame rate.

Future stage timers/profiling should distinguish CPU submission, GPU execution
and presentation waits. CPU/GPU utilization, RAM/VRAM, allocation optimization,
adaptive integration and resolution/termination changes are outside M0.
No performance difference in M0 may be described as numerical-optimization speedup.
