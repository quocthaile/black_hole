# B0 / M0: original baseline and presentation timing

B0 is the immutable `BlackHole3D_baseline` target. Its seven source blobs match
upstream `dc263bb5a1b72f5a24190d2e38b84b5c8fe9a317`; the reorganized fork snapshot
is `0f74de4ae0fcb6436f95a7a5cd709aa023c93f67`. See
[the lock](baseline_manifest.json), [architecture](../docs/baseline_architecture.md),
[numerical audit](../docs/baseline_numerical_audit.md) and
[executed verification](../docs/baseline_m0_verification.md).

M0 implements only average FPS, mean/median/p95/p99 frame time, frame count and
measurement duration. No GPU execution timers, memory/utilization metrics,
numerical corrections or optimization are implemented. A visual smoke check
does not establish physical correctness or replace timed measurements.

## Build and integrity

From the repository root in PowerShell, with Visual Studio 2026, CMake 4.2+,
Python 3.9+ and `VCPKG_ROOT` set to your vcpkg installation:

```powershell
cmake --preset msvc-vs2026 -B build/m0-msvc-vs2026
cmake --build build/m0-msvc-vs2026 --config Release --target BlackHole3D_baseline BlackHole3D
python benchmarks/scripts/verify_baseline.py
python -m unittest discover -s benchmarks/scripts -p 'test_*.py' -v
```

Always run each executable with its own `build/m0-msvc-vs2026/bin/Release/<target>`
directory as the working directory. No changes to CMake or compiler optimization
flags are needed. Both targets have separate staged shaders.

`verify_baseline.py` reads only standard-library Python and the lock, not a Git
installation. It rejects changed/missing/extra baseline files and changed
development shaders. Original Git objects contain both LF and CRLF files. The
manifest's `git_blob_sha1` and `blob_sha256` preserve those exact bytes; `lf_*`
hashes allow **only CRLF -> LF** for worktree verification. Spaces, BOMs and all
other content remain significant. Never regenerate the lock to conceal an edit.
Build-file hashes in the manifest describe the snapshot; they are provenance,
not included in the source-integrity script's ten-file check.

## Capture protocol

The fixed [m0.json](configs/m0.json) specifies Release x64, original static camera
and scene, requested 800 x 600 window, 200 x 150 compute, gravity off and unchanged
numerical constants. Each implementation gets five fresh process launches,
10 seconds warm-up and 30 seconds capture. The delay starts after a render window
appears; verify that rendering begins during warm-up. Keep the window visible,
uncovered, unminimized and untouched. Do not orbit, zoom, toggle gravity or resize.
Use the same display, driver settings, power mode, background workload and capture
tool version for both sets; record run order and any interruptions in the evidence.

Obtain the official **console** executable (documented interface: PresentMon
2.3.1 x64, `--v1_metrics`) from [Intel's PresentMon releases](https://github.com/GameTechDev/PresentMon/releases/tag/v2.3.1).
The script checks its advertised CLI options and records its hash/version/help.
It does not install PresentMon, elevate permissions or stop unrelated ETW sessions.
If tracing is denied, use an environment with the required ETW access and retry;
retain the failure logs. No tool or no usable CSV means **unmeasured**.

Create an operator record inside the ignored results directory, for example
`benchmarks/results/presentation.json`. These are placeholders, not observations:

```json
{
  "display": "record monitor and desktop mode",
  "refresh_hz": "unknown",
  "driver_vsync": "record global AND per-executable overrides for both targets",
  "vrr": "unknown",
  "window_mode": "windowed",
  "power_mode": "record power plan and AC/battery state",
  "observed_client_size": "unknown",
  "observed_framebuffer_size": "unknown",
  "opengl_version": "copy actual application startup output",
  "scene_camera_gravity_check": "record untouched initial camera/scene and gravity off",
  "evidence": "record observations, timestamps, driver/display evidence, run order and interruptions"
}
```

Replace the descriptions with observed values; retain `unknown` for anything not
established. The immutable program does not query all these settings. In particular,
no call to set swap interval exists: do not assume VSync off. Inspect the effective
driver overrides for **both executable names**. PresentMon's SyncInterval alone
does not establish driver/compositor equivalence.

Commit source changes before official captures. Use new output directories:

```powershell
./benchmarks/scripts/capture_presentmon.ps1 -Implementation BlackHole3D_baseline `
  -PresentMon ./tools/PresentMon-2.3.1-x64.exe `
  -PresentationMetadata ./benchmarks/results/presentation.json `
  -OutputDirectory ./benchmarks/results/b0-session1

./benchmarks/scripts/capture_presentmon.ps1 -Implementation BlackHole3D `
  -PresentMon ./tools/PresentMon-2.3.1-x64.exe `
  -PresentationMetadata ./benchmarks/results/presentation.json `
  -OutputDirectory ./benchmarks/results/dev-session1
```

Paths resolve from your current shell directory; script/config paths resolve from
the repository. Optional `-BuildDirectory` selects another configured VS2026 x64
build; `-Python` selects a Python executable. The script rebuilds Release, checks
source/runtime shader byte identity, launches each app from its runtime directory,
filters PresentMon by the launched PID, and gives each trace a unique session name.
It hides console hosts while GLFW creates its normal render window. It records
application output/errors, tool output/errors, executable/DLL/shader hashes,
Git commit, OS/CPU/GPU/driver, build project/cache/dependencies, config, operator
presentation metadata and command arguments. It closes only processes it started.
The five runs are sequential, independent launches, not a guarantee of statistically
independent thermal/background conditions. Record those conditions explicitly.

## CSV processing and statistics

Required raw columns: `Application`, `ProcessID`, `SwapChainAddress`,
`TimeInSeconds` (seconds), and `msBetweenPresents` (milliseconds; the capitalized
`MsBetweenPresents` spelling is also accepted). Modern default CPU/GPU-time schemas
are deliberately not substituted for inter-present intervals. The official
[console documentation](https://github.com/GameTechDev/PresentMon/blob/v2.3.1/README-ConsoleApplication.md)
describes the `--v1_metrics` option. Legacy column definitions and OpenGL caveats
are in [PresentMon 1.10's documentation](https://github.com/GameTechDev/PresentMon/blob/v1.10.0/README-ConsoleApplication.md)
and [OpenGL notes](https://github.com/GameTechDev/PresentMon/blob/v1.10.0/README.md#analyzing-opengl-and-vulkan-applications).
For OpenGL, events may be observed in the kernel (`Runtime=Other`, swap chain 0);
these are observed presentation intervals, not exact GPU execution durations.

The capture script invokes normalization automatically. To reproduce it from raw
data in a new output directory (replace 1234 with the recorded PID):

```powershell
python benchmarks/scripts/normalize_presentmon.py benchmarks/results/b0-session1/run-01/raw.csv `
  --process-id 1234 --metadata benchmarks/results/b0-session1/run-01/metadata.json `
  --output-dir benchmarks/results/b0-session1/run-01/reprocessed
```

If a PID has multiple swap chains, normalization fails until `--swap-chain` selects
one explicitly; document why it is the render stream. Other PIDs are excluded.
The first selected present is excluded because its preceding event is outside the
sample. All later intervals must be finite and positive, and timestamps strictly
increasing; malformed/missing data fails rather than becoming fabricated samples.
No outlier removal or dropped-frame filtering is applied. Raw data is never edited.

Normalized `frames.csv` has `source_row,time_s,frame_time_ms`; source row numbers
include the raw header as row 1. `summary.json` has `schema_version`, selection/
presentation descriptors, raw SHA-256, capture metadata, definitions, comparison
status and the seven `metrics` keys listed in [catalog.json](metrics/catalog.json).
For K accepted intervals Ti:

```text
average_fps = 1000*K / sum(Ti)
mean_frame_time_ms = sum(Ti) / K
measurement_duration_s = sum(Ti) / 1000
```

Frame count is K intervals (K+1 observed present events), not every CSV row.
Measurement duration can be shorter than the requested 30-second wall window.
Percentiles sort Ti and linearly interpolate at zero-based index `(K-1)*p`, with
p = 0.50, 0.95, 0.99 (Hyndman-Fan type 7). A single interval returns itself.
`mean(1000/Ti)` is not the average FPS. Never label these numbers GPU execution time.

## Dataset review and limitations

Every summary defaults to `non_comparable_pending_presentation_and_protocol_review`.
Before comparing, inspect all ten runs: executable/shader/commit identities, actual
scene/resolutions, warm-up/render startup, capture coverage, PresentMode/SyncInterval/
tearing descriptors, driver overrides, monitor/refresh/VRR and power conditions.
Compare metadata across sets and retain a separate evidence-backed review with the
dataset; do not overwrite raw metadata. Missing/unknown or unequal presentation
conditions mean **non-comparable**. The script cannot infer equivalence automatically.

Keep all five per-run summaries visible; investigate variability rather than
selecting the fastest run. No optimization speedup is expected or claimed in M0.
Significant differences require checking builds, shader staging, scene/input,
driver/presentation conditions and measurement variability first.

`results/` is ignored by default to avoid committing machine-specific/raw datasets
accidentally. Preserve complete session directories (including raw captures) in
your chosen research archive; explicitly add a reviewed dataset if versioning it.
Tests contain synthetic intervals only and are not simulation measurements.
