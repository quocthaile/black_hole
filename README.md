# black_hole

Black hole simulation with independent development and preserved baseline executables.
This restructuring is not a performance optimization: physics algorithms, numerical
constants, shader precision, and GLSL versions are unchanged.

## Source layout and baseline provenance

```text
CMakeLists.txt
README.md
vcpkg.json
src/
  black_hole.cpp
  baseline/
    black_hole.cpp
    2D_lensing.cpp
    CPU-geodesic.cpp
    ray_tracing.cpp
    geodesic.comp
    grid.vert
    grid.frag
shaders/
  geodesic.comp
  grid.vert
  grid.frag
```

The baseline is preserved from [quocthaile/black_hole](https://github.com/quocthaile/black_hole)
at commit [`dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317`](https://github.com/quocthaile/black_hole/tree/dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317),
the `main` revision used for this restructuring. All seven baseline files retain
the original contents. Development sources and shaders start as independent copies.
Keep `src/baseline/` unchanged so future computational-cost benchmarks and numerical
accuracy comparisons have a stable reference. No benchmark instrumentation is added.

The source organization and small CMake target helper follow
[gravity_sim](https://github.com/quocthaile/gravity_sim/tree/c98dcd55958420b88e6c5193e954e71ee3f8b574).
This project retains its own C++17 requirement and dependencies.
`CPU-geodesic.cpp` and `ray_tracing.cpp` remain reference sources without build targets.
Other historical files, including the root executable/archive and `vs_code/` examples,
are retained; use the CMake workflow below for the new layout.

## Build

Requirements: CMake 3.21+, a C++17 compiler, GLFW, GLEW, GLM, and OpenGL.
The 3D runtime requires OpenGL 4.3 with compute shaders. The 2D baseline uses legacy
OpenGL rendering. MSVC builds use `/utf-8` to preserve original Unicode identifiers.

Windows PowerShell with Visual Studio and vcpkg (adjust the vcpkg path):

```powershell
git clone https://github.com/quocthaile/black_hole.git
cd black_hole
$env:VCPKG_ROOT = 'C:/vcpkg'
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

The manifest installs the existing dependencies automatically. To select a generator
explicitly, add e.g. `-G "Visual Studio 18 2026" -A x64` to configuration.
Build any executable independently:

```powershell
cmake --build build --config Release --target BlackHole3D
cmake --build build --config Release --target BlackHole3D_baseline
cmake --build build --config Release --target BlackHole2D_baseline
```

For a single-configuration generator, run from a compiler-enabled shell (such as
Developer PowerShell for Visual Studio on Windows):

```powershell
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build-ninja --target BlackHole3D BlackHole3D_baseline BlackHole2D_baseline
```

On Debian/Ubuntu, native packages can replace vcpkg:

```bash
sudo apt update
sudo apt install build-essential cmake libglew-dev libglfw3-dev libglm-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Executables and runtime assets

| Target | Source | Shader source |
| --- | --- | --- |
| `BlackHole3D` | `src/black_hole.cpp` | `shaders/` |
| `BlackHole3D_baseline` | `src/baseline/black_hole.cpp` | `src/baseline/` |
| `BlackHole2D_baseline` | `src/baseline/2D_lensing.cpp` | None |

Every target links dependencies independently. Executables are placed in
`<build>/bin/<configuration>/<target>/`, including for single-configuration builds
with `CMAKE_BUILD_TYPE` set. If that setting is empty, the configuration segment is
omitted. Each 3D target stages its own three shaders beside its executable using
`copy_if_different` and a target-specific `POST_BUILD` command. A staging dependency
also refreshes shaders on builds where only shader files changed and no relink occurs.
Development and baseline runtime assets never share an output directory.

Shader loading still uses the original relative filenames. **Run each executable
with its own executable directory as the working directory.** Generated Visual Studio
projects set `VS_DEBUGGER_WORKING_DIRECTORY` accordingly. For other IDE/debug launch
configurations, set the working directory explicitly to the same directory.

Run and smoke-test both 3D variants in PowerShell (close each window to continue):

```powershell
Push-Location build/bin/Release/BlackHole3D
try { ./BlackHole3D.exe } finally { Pop-Location }
Push-Location build/bin/Release/BlackHole3D_baseline
try { ./BlackHole3D_baseline.exe } finally { Pop-Location }
```

For Ninja, replace `build` with `build-ninja`; on Linux omit `.exe`.
The 2D baseline can be launched from its target directory. Its preserved scene
initially has no rays because its original ray initialization is commented out.
Check both 3D windows render and that their consoles show an OpenGL version without
file-loading, shader-compilation, or shader-link errors. Building successfully alone
does not validate graphics runtime behavior or numerical accuracy.
