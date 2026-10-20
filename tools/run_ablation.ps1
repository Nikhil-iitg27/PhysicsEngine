# run_ablation.ps1
#
# Sweeps sphere count for the 4 combinations of {OrbitSpring, UniformGravity}
# gravity mode x {BroadPhase, BruteForce} collision mode, at a fixed
# restitution, launching Physix.exe --benchmark --hidden once per (mode,
# count) point and recording its steady-state smoothed-FPS CSV line (see
# Application.cpp's RunConfig / --benchmark). From the resulting (count, fps)
# curve per combo, interpolates the sphere count at which FPS first drops
# below each of --Thresholds.
#
# Lives in tools/ rather than in the C++ project deliberately -- the engine
# only needs to expose CLI-tunable parameters (Application.cpp's ParseArgs)
# and print one
# machine-readable line per run; the sweep/interpolation logic itself has no
# reason to live inside the C++ project.

# $BaseSphereCounts covers the range where brute force already breaks down
# (see docs/benchmarks/ablation_results.csv from the first sweep: brute force
# crossed 15fps well under 1500 spheres). $ExtendedSphereCounts continues
# geometrically further, up to $MaxSpheres, for combos that are still above
# every threshold at the end of the base list (in practice: broad-phase mode,
# which stayed above 60fps through the entire base list -- the whole point of
# the broad-phase grid is to not degrade until much higher counts).
param(
    [string]$Configuration = "Release",
    [double]$Restitution = 1.0,
    [uint32]$Seed = 42,
    [double]$Duration = 7.0,
    [double]$Warmup = 3.0,
    [int[]]$BaseSphereCounts = @(10, 20, 30, 50, 75, 100, 150, 200, 300, 400, 500, 650, 800, 1000, 1250, 1500),
    [int[]]$ExtendedSphereCounts = @(2000, 2500, 3000, 4000, 5000, 6500, 8000, 10000, 12000),
    [double[]]$Thresholds = @(60.0, 30.0, 15.0),
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot "Physix.sln"
$exePath = Join-Path $repoRoot "x64\$Configuration\Physix.exe"
$outDir = Join-Path $repoRoot "docs\benchmarks"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

if (-not $SkipBuild) {
    Write-Host "Building $Configuration|x64 ..."
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    & $msbuild $solution /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 /m /nologo /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}
if (-not (Test-Path $exePath)) { throw "Physix.exe not found at $exePath" }

$combos = @(
    @{ GravityMode = "orbit";   Collision = "broadphase" },
    @{ GravityMode = "orbit";   Collision = "bruteforce" },
    @{ GravityMode = "uniform"; Collision = "broadphase" },
    @{ GravityMode = "uniform"; Collision = "bruteforce" }
)

$results = New-Object System.Collections.Generic.List[object]
$lowestThreshold = ($Thresholds | Measure-Object -Minimum).Minimum

function Run-One([int]$n, [hashtable]$combo) {
    Write-Host ("Running spheres={0} gravity={1} collision={2} ..." -f $n, $combo.GravityMode, $combo.Collision)
    $args = @(
        "--benchmark", "--hidden",
        "--spheres=$n",
        "--restitution=$Restitution",
        "--gravity-mode=$($combo.GravityMode)",
        "--collision=$($combo.Collision)",
        "--seed=$Seed",
        "--duration=$Duration",
        "--warmup=$Warmup"
    )
    $line = & $exePath @args 2>$null | Select-Object -Last 1
    if (-not $line) { Write-Warning "No output for spheres=$n $($combo.GravityMode)/$($combo.Collision), skipping"; return $null }
    $fields = $line.Split(",")
    $point = [PSCustomObject]@{
        Spheres      = [int]$fields[0]
        Restitution  = [double]$fields[1]
        GravityMode  = $fields[2]
        Collision    = $fields[3]
        AvgFps       = [double]$fields[4]
        MinFps       = [double]$fields[5]
    }
    $results.Add($point)
    return $point
}

foreach ($combo in $combos) {
    $lastPoint = $null
    foreach ($n in $BaseSphereCounts) { $lastPoint = Run-One $n $combo }

    # Only push into the extended (higher-N) range if the base sweep never
    # dropped below the lowest requested threshold -- avoids wasting minutes
    # sweeping brute-force collision far past the point it's already broken.
    if ($lastPoint -and $lastPoint.AvgFps -ge $lowestThreshold) {
        foreach ($n in $ExtendedSphereCounts) {
            $lastPoint = Run-One $n $combo
            if ($lastPoint -and $lastPoint.AvgFps -lt $lowestThreshold) { break }
        }
    }
}

$csvPath = Join-Path $outDir "ablation_results.csv"
$results | Export-Csv -Path $csvPath -NoTypeInformation
Write-Host "Wrote $csvPath"

# Linear-interpolate the sphere count at which AvgFps first drops below each
# threshold, walking the (count, fps) curve in increasing N order. Returns
# $null ("never") if the swept range never crosses it.
function Find-Crossing($points, [double]$threshold) {
    $sorted = $points | Sort-Object Spheres
    for ($i = 1; $i -lt $sorted.Count; $i++) {
        $prev = $sorted[$i - 1]
        $cur = $sorted[$i]
        if ($prev.AvgFps -ge $threshold -and $cur.AvgFps -lt $threshold) {
            $t = ($prev.AvgFps - $threshold) / ($prev.AvgFps - $cur.AvgFps)
            return [math]::Round($prev.Spheres + $t * ($cur.Spheres - $prev.Spheres))
        }
    }
    return $null
}

$summaryLines = New-Object System.Collections.Generic.List[string]
$summaryLines.Add("# Ablation summary (restitution=$Restitution, seed=$Seed)")
$summaryLines.Add("")
$summaryLines.Add("| Gravity mode | Collision mode | Sphere count @ 60fps | @ 30fps | @ 15fps |")
$summaryLines.Add("|---|---|---|---|---|")
foreach ($combo in $combos) {
    $points = $results | Where-Object { $_.GravityMode -eq $combo.GravityMode -and $_.Collision -eq $combo.Collision }
    $cells = foreach ($th in $Thresholds) {
        $c = Find-Crossing $points $th
        if ($null -eq $c) { "never (within tested range)" } else { $c }
    }
    $summaryLines.Add("| $($combo.GravityMode) | $($combo.Collision) | $($cells[0]) | $($cells[1]) | $($cells[2]) |")
}

$summaryPath = Join-Path $outDir "ablation_summary.md"
$summaryLines | Set-Content -Path $summaryPath -Encoding utf8
Write-Host "Wrote $summaryPath"
Get-Content $summaryPath
