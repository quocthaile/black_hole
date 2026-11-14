# M0 verification record

Executed on 2026-11-14 (Asia/Bangkok). Scope: B-01/B-03 and the initial frame
performance subset of E-01/E-02; the CPU/GPU numerical audit prepares H-01/H-05.
Full proposal requirements remain broader than this explicitly limited task.

## Git and implementation

- Working tree was clean on `main`; no uncommitted work needed stashing.
- Ran `git fetch origin main`, resolved `origin/main` to
  `0f74de4ae0fcb6436f95a7a5cd709aa023c93f67`, and created
  `research/blackhole3d-baseline-m0` directly from that commit.
- No earlier research/style branch source, commits, scripts, tests, documents or
  generated outputs were used. A new `build/m0-msvc-vs2026` directory was configured.
- Fetched original upstream HEAD and byte-compared all seven baseline files to
  `dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317`; all were identical. Raw blob hashes
  and the separate CRLF normalization policy are in the manifest.
- `git diff 0f74de4 -- src/baseline shaders CMakeLists.txt CMakePresets.json vcpkg.json`
  was empty. No numerical functions/constants or external shaders were changed.
- A code-token comparison of development source against the base, excluding only
  comments, `std::`/`glm::` qualifiers/imports and three explicit standard-library
  includes, passed. Embedded GLSL string contents also matched after newline
  normalization. No global `using namespace std`/`glm` imports remain there.
- `src/reference/` is absent. No reference executable/solver was introduced.
- No PR, merge, main update or push was performed. The task stops at local commits.

Created: baseline manifest, integrity verifier, M0 configuration and metric catalog,
capture/normalization scripts, statistical/error-handling tests, benchmark README,
results ignore file, architecture/numerical audit and this verification record.
Modified: development `src/black_hole.cpp` (namespaces/includes/comments), root
README (M0 workflow/provenance) and `.gitignore` (Python bytecode). CMake is unchanged.

## Executed build and runtime checks

```powershell
cmake --preset msvc-vs2026 -B build/m0-msvc-vs2026
cmake --build build/m0-msvc-vs2026 --config Release --target BlackHole3D_baseline BlackHole3D
python benchmarks/scripts/verify_baseline.py
python -m unittest discover -s benchmarks/scripts -p 'test_*.py' -v
git -c core.whitespace=cr-at-eol diff --check
```

Both Release targets built successfully. The existing development file's CRLF
style was retained; the whitespace check explicitly recognizes CRLF line ends.
All six staged shader files were compared byte-for-byte with their corresponding
source files and matched, in separate target runtime directories.

Each final executable was launched from its own runtime directory, observed as a
running/responding process with window title `Black Hole`, then closed using its
window-close request. Both exited with code 0. Each printed:

```text
OpenGL 4.3.0 - Build 32.0.101.7085
```

Both stderr logs were empty. The original grid and compute load/compile/link code
checks status and exits on failure, so reaching the responsive loop without those
errors verifies those startup checks on this driver. This does not independently
validate the unchecked embedded screen shader, render correctness or physics.

**Visual inspection remains unverified.** Both available computer-use entry points
failed to connect to the native service (`native pipe ... cannot find the file`).
No screenshot/rendered image was inspected. No image equivalence is claimed.

Local evidence (ignored build artifacts, retained on this machine):
`build/m0-msvc-vs2026/m0-release-build.log`, `CMakeCache.txt`, generated target
projects, `vcpkg_installed/vcpkg/status`, and
`BlackHole3D_baseline.smoke.{stdout,stderr}.txt` /
`BlackHole3D.smoke.{stdout,stderr}.txt` in that build directory.

| Environment | Observed value |
| --- | --- |
| OS | Windows 11 Pro, 10.0.26300 (build 26300) |
| CPU | 13th Gen Intel Core i5-13420H |
| GPU | Intel UHD Graphics |
| GPU driver | 32.0.101.7085 |
| Compiler | MSVC 19.52.37029.0; toolset directory 14.52.37025 |
| CMake | 4.4.0-rc2 |
| Generator / SDK | Visual Studio 18 2026 x64 / 10.0.28000.0 |
| Python | 3.14.3 |
| Main packages | GLEW 2.3.1, GLFW 3.4#1, GLM 1.0.3, x64-windows |

Executable SHA-256 values for the final local smoke-tested build (not universal
reproducible-build hashes; toolchain/link timestamps can affect rebuilds):

```text
BlackHole3D_baseline.exe f5724f8e8ab25a41b57d815db97e615a1bb26d210836743c9118458456c698a7
BlackHole3D.exe          b29016c8ee88086ac61b0516d358b5ced4fc576af4c8ef604f2852a3a81d9d20
```

## Measurement infrastructure and remaining work

- Integrity check: PASS, seven baseline files plus three development shaders.
- Python tests: **11 passed**, including FPS weighting, mean/duration/count,
  type-7 percentiles, invalid/missing data, stream selection, malformed CSV,
  raw-file preservation, overwrite refusal and baseline tamper detection.
- PowerShell parser: PASS using `System.Management.Automation.Language.Parser`.
- Capture preflight: missing PresentMon and existing output directory were each
  executed and correctly rejected without starting a capture.
- Full external capture: **not executed**. PresentMon was not found on PATH or in
  the checked Intel/PresentMon/tool directories. Console integration/ETW collection
  and five-run protocol remain unverified against a real installed PresentMon.
- No actual FPS/frame-time dataset exists for this task; synthetic unit-test
  intervals are not measurements. No speedup or performance bottleneck is claimed.
- Effective VSync, compositor, display/refresh/VRR, actual framebuffer size and
  cross-executable presentation equivalence were not established. Comparisons
  remain **non-comparable** pending the documented operator and CSV review.

Next prerequisites, not implemented here: obtain the external capture tool and
record actual presentation conditions; inspect both rendered outputs; collect and
archive five runs each. Before later CPU reference/shader RK4 work, independently
validate initialization, radial equations, units, E/L definitions, coordinate/event
conventions and convergence criteria. Preserve B0 throughout that later research.
