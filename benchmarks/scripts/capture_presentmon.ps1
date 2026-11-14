<#
.SYNOPSIS
Capture five fresh M0 runs of one Release x64 target without changing its source.
.DESCRIPTION
Requires Windows PowerShell 5.1+, Python 3.9+, Git, CMake and a PresentMon console
executable supporting the 2.3.1 CLI (--v1_metrics). Configure the build directory
first. The script rebuilds the selected target and verifies deployed shaders.
Required parameters: Implementation, PresentMon, PresentationMetadata and
OutputDirectory (must not exist). Optional BuildDirectory and Python are paths.
PresentationMetadata is operator-authored JSON; see benchmarks/README.md.
Input: clean Git tree, configured MSVC x64 build, m0.json and presentation JSON.
Output: build/config/tool records and five run directories containing raw.csv,
stdout/stderr logs, metadata.json, analysis/frames.csv and analysis/summary.json.
Missing tools, dirty source, wrong build configuration, changed shaders, existing
output, failed app/capture or invalid CSV terminate with an error. Partial output
is retained for diagnosis; choose a new output directory for a retry.
This does not elevate privileges, change VSync or certify presentation equivalence.
.EXAMPLE
./benchmarks/scripts/capture_presentmon.ps1 -Implementation BlackHole3D_baseline `
  -PresentMon ./tools/PresentMon-2.3.1-x64.exe `
  -PresentationMetadata ./presentation.json -OutputDirectory ./benchmarks/results/b0-session1
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('BlackHole3D_baseline', 'BlackHole3D')][string]$Implementation,
    [Parameter(Mandatory = $true)][string]$PresentMon,
    [Parameter(Mandatory = $true)][string]$PresentationMetadata,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$BuildDirectory = './build/m0-msvc-vs2026',
    [string]$Python = 'python'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$presentMonPath = (Resolve-Path -LiteralPath $PresentMon).Path
$presentationPath = (Resolve-Path -LiteralPath $PresentationMetadata).Path
$buildPath = (Resolve-Path -LiteralPath $BuildDirectory).Path
$outputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
if (Test-Path -LiteralPath $outputPath) { throw "Refusing to overwrite $outputPath" }

function Write-JsonFile($Value, [string]$Path) {
    $Value | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Get-FileIdentity([string]$Path) {
    $item = Get-Item -LiteralPath $Path
    return @{ path = $item.FullName; bytes = $item.Length;
              sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
}

$presentation = Get-Content -LiteralPath $presentationPath -Raw | ConvertFrom-Json
foreach ($field in @('display', 'refresh_hz', 'driver_vsync', 'vrr', 'window_mode',
                     'power_mode', 'observed_client_size', 'observed_framebuffer_size',
                     'opengl_version', 'scene_camera_gravity_check', 'evidence')) {
    if ($null -eq $presentation.PSObject.Properties[$field] -or
        [string]::IsNullOrWhiteSpace([string]$presentation.$field)) {
        throw "Presentation metadata needs '$field'; use 'unknown' when unverified."
    }
}
$configPath = Join-Path $repoRoot 'benchmarks/configs/m0.json'
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
if ($config.schema_version -ne 1 -or $config.runs_per_implementation -ne 5 -or
    $config.warmup_seconds -ne 10 -or $config.capture_seconds -ne 30) {
    throw 'Expected the locked M0 protocol: 5 runs, 10 s warm-up, 30 s capture.'
}
$cachePath = Join-Path $buildPath 'CMakeCache.txt'
$cache = Get-Content -LiteralPath $cachePath -Raw
if ($cache -notmatch '(?m)^CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\r?$' -or
    $cache -notmatch '(?m)^CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026\r?$') {
    throw 'M0 requires the configured Visual Studio 2026 x64 build.'
}
$homeMatch = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=(.+?)\r?$')
if (-not $homeMatch.Success -or
    [IO.Path]::GetFullPath($homeMatch.Groups[1].Value) -ne [IO.Path]::GetFullPath($repoRoot)) {
    throw 'Build directory belongs to a different source tree.'
}

Push-Location $repoRoot
try {
    $gitStatus = & git status --porcelain --untracked-files=normal
    if ($LASTEXITCODE -ne 0 -or $gitStatus) { throw 'Commit source changes before official captures.' }
    $commit = (& git rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Cannot identify Git commit.' }
    & $Python (Join-Path $PSScriptRoot 'verify_baseline.py')
    if ($LASTEXITCODE -ne 0) { throw 'Baseline integrity check failed.' }

    New-Item -ItemType Directory -Path $outputPath | Out-Null
    $buildLog = & cmake --build $buildPath --config Release --target $Implementation 2>&1
    $buildExit = $LASTEXITCODE
    $buildLog | Set-Content -LiteralPath (Join-Path $outputPath 'build.log') -Encoding UTF8
    if ($buildExit -ne 0) { throw 'Release build failed; see build.log.' }
    Copy-Item -LiteralPath $cachePath -Destination (Join-Path $outputPath 'CMakeCache.txt')
    Copy-Item -LiteralPath $configPath -Destination (Join-Path $outputPath 'm0.json')
    Copy-Item -LiteralPath $presentationPath -Destination (Join-Path $outputPath 'presentation.json')
    Copy-Item -LiteralPath (Join-Path $buildPath "$Implementation.vcxproj") -Destination $outputPath
    Copy-Item -LiteralPath (Join-Path $buildPath 'vcpkg_installed/vcpkg/status') -Destination (Join-Path $outputPath 'dependencies.txt')
    $helpText = & $presentMonPath --help 2>&1 | Out-String
    $helpText | Set-Content -LiteralPath (Join-Path $outputPath 'presentmon-help.txt') -Encoding UTF8
    foreach ($option in @('--v1_metrics', '--process_id', '--delay', '--timed', '--terminate_after_timed', '--no_track_gpu')) {
        if (-not $helpText.Contains($option)) { throw "PresentMon does not advertise $option" }
    }

    $runtime = Join-Path $buildPath "bin/Release/$Implementation"
    $exePath = Join-Path $runtime "$Implementation.exe"
    $shaderRoot = if ($Implementation -eq 'BlackHole3D_baseline') { 'src/baseline' } else { 'shaders' }
    $shaderIdentities = @()
    foreach ($shader in @('geodesic.comp', 'grid.vert', 'grid.frag')) {
        $deployed = Get-FileIdentity (Join-Path $runtime $shader)
        $source = Get-FileIdentity (Join-Path $repoRoot "$shaderRoot/$shader")
        if ($deployed.sha256 -ne $source.sha256) { throw "Incorrect deployed shader: $shader" }
        $shaderIdentities += $deployed
    }
    $software = @{
        os = Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, OSArchitecture
        cpu = @(Get-CimInstance Win32_Processor | Select-Object Name)
        gpu = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, CurrentRefreshRate)
        cmake = (& cmake --version | Out-String).Trim()
        python = (& $Python --version | Out-String).Trim()
    }
    $exeIdentity = Get-FileIdentity $exePath
    $toolIdentity = Get-FileIdentity $presentMonPath
    $toolIdentity.version = (Get-Item -LiteralPath $presentMonPath).VersionInfo.FileVersion
    $dllIdentities = @(Get-ChildItem -LiteralPath $runtime -Filter '*.dll' | ForEach-Object { Get-FileIdentity $_.FullName })

    for ($run = 1; $run -le $config.runs_per_implementation; $run++) {
        $runPath = Join-Path $outputPath ('run-{0:D2}' -f $run)
        New-Item -ItemType Directory -Path $runPath | Out-Null
        $app = $null
        $monitor = $null
        $metadata = @{
            schema_version = 1; implementation = $Implementation; run = $run; git_commit = $commit
            configuration = $config; executable = $exeIdentity; shaders = $shaderIdentities
            runtime_dlls = $dllIdentities; presentmon = $toolIdentity; system = $software
            presentation = $presentation; working_directory = $runtime
            comparison_status = 'non_comparable_pending_presentation_and_protocol_review'
            status = 'started'; started_utc = [DateTime]::UtcNow.ToString('o')
        }
        $metadataPath = Join-Path $runPath 'metadata.json'
        try {
            # Hide the console host; GLFW still creates its normal visible render window.
            $app = Start-Process -FilePath $exePath -WorkingDirectory $runtime -WindowStyle Hidden -PassThru `
                -RedirectStandardOutput (Join-Path $runPath 'app.stdout.txt') `
                -RedirectStandardError (Join-Path $runPath 'app.stderr.txt')
            $metadata.process_id = $app.Id
            $deadline = [DateTime]::UtcNow.AddSeconds(30)
            do {
                Start-Sleep -Milliseconds 200
                $app.Refresh()
                if ($app.HasExited) { throw 'Application exited during startup; inspect app logs.' }
            } while ($app.MainWindowHandle -eq 0 -and [DateTime]::UtcNow -lt $deadline)
            if ($app.MainWindowHandle -eq 0) { throw 'No application window within 30 seconds.' }
            $rawPath = Join-Path $runPath 'raw.csv'
            $sessionName = 'BlackHoleM0-' + [Guid]::NewGuid().ToString('N')
            $pmArgs = @('--process_id', [string]$app.Id, '--output_file', ('"' + $rawPath + '"'),
                        '--v1_metrics', '--delay', [string]$config.warmup_seconds,
                        '--timed', [string]$config.capture_seconds, '--terminate_after_timed',
                        '--terminate_on_proc_exit', '--no_console_stats', '--no_track_gpu', '--no_track_input',
                        '--session_name', $sessionName)
            $metadata.capture_arguments = $pmArgs
            $metadata.warmup_definition = 'PresentMon delay after application window appears; verify rendering starts during warm-up'
            Write-JsonFile $metadata $metadataPath
            Write-Host "Run $run/5: keep the render window visible and untouched for 40 seconds."
            $monitor = Start-Process -FilePath $presentMonPath -ArgumentList $pmArgs -WindowStyle Hidden -PassThru `
                -RedirectStandardOutput (Join-Path $runPath 'presentmon.stdout.txt') `
                -RedirectStandardError (Join-Path $runPath 'presentmon.stderr.txt')
            # Retain the handle before exit so Windows PowerShell can read ExitCode.
            $null = $monitor.Handle
            if (-not $monitor.WaitForExit(60000)) { throw 'PresentMon timed out; capture is incomplete.' }
            $monitor.WaitForExit()
            $app.Refresh()
            if ($monitor.ExitCode -ne 0 -or $app.HasExited) { throw 'Capture failed or application exited early.' }
            $metadata.status = 'captured_pending_review'
            $metadata.finished_utc = [DateTime]::UtcNow.ToString('o')
            $metadata.raw_csv = Get-FileIdentity $rawPath
            Write-JsonFile $metadata $metadataPath
            & $Python (Join-Path $PSScriptRoot 'normalize_presentmon.py') $rawPath `
                --process-id $app.Id --output-dir (Join-Path $runPath 'analysis') --metadata $metadataPath
            if ($LASTEXITCODE -ne 0) { throw 'CSV normalization failed; raw capture retained.' }
        }
        catch {
            $metadata.status = 'failed'
            $metadata.error = $_.Exception.Message
            Write-JsonFile $metadata $metadataPath
            throw
        }
        finally {
            # Stop only the processes created for this run, never unrelated sessions.
            if ($null -ne $app -and -not $app.HasExited) {
                $null = $app.CloseMainWindow()
                if (-not $app.WaitForExit(5000)) { $app.Kill(); $app.WaitForExit() }
            }
            if ($null -ne $monitor -and -not $monitor.HasExited) {
                if (-not $monitor.WaitForExit(5000)) { $monitor.Kill(); $monitor.WaitForExit() }
            }
        }
    }
}
finally { Pop-Location }
