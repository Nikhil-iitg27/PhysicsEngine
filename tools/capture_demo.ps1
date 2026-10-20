# capture_demo.ps1
#
# Produces the two README demo scenarios via Physix.exe's --screenshot-at /
# --gif capture flags (Application.cpp), both using the robust broad-phase
# collision mode:
#   1. Central gravity (OrbitSpring) + low restitution: bodies lose energy on
#      every bounce and settle into a cluster near the origin.
#   2. Uniform gravity + high restitution: bodies fall and bounce energetically;
#      one sphere is preselected (--track-index=0) so it renders highlighted
#      for the whole clip, making it easy to follow.
# Runs with --hidden so nothing flashes on screen.

param(
    [string]$Configuration = "Release",
    [int]$Spheres = 150,
    [uint32]$Seed = 1234,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot "Physix.sln"
$exePath = Join-Path $repoRoot "x64\$Configuration\Physix.exe"
$outDir = Join-Path $repoRoot "docs\media"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

if (-not $SkipBuild) {
    Write-Host "Building $Configuration|x64 ..."
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    & $msbuild $solution /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 /m /nologo /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}
if (-not (Test-Path $exePath)) { throw "Physix.exe not found at $exePath" }

$demo1Png = Join-Path $outDir "demo1_central_gravity.png"
$demo1Gif = Join-Path $outDir "demo1_central_gravity.gif"
Write-Host "Capturing demo 1: central gravity, low restitution, clustering ..."
& $exePath --hidden `
    --spheres=$Spheres --seed=$Seed `
    --gravity-mode=orbit --restitution=0.1 --collision=broadphase `
    --duration=20 --screenshot-at="20@$demo1Png" --gif="14@20@$demo1Gif" --gif-fps=10
if ($LASTEXITCODE -ne 0) { throw "Demo 1 capture failed with exit code $LASTEXITCODE" }

$demo2Png = Join-Path $outDir "demo2_uniform_gravity_tracked.png"
$demo2Gif = Join-Path $outDir "demo2_uniform_gravity_tracked.gif"
Write-Host "Capturing demo 2: uniform gravity, high restitution, tracked sphere ..."
& $exePath --hidden `
    --spheres=$Spheres --seed=$Seed --track-index=0 `
    --gravity-mode=uniform --restitution=0.95 --collision=broadphase `
    --duration=15 --screenshot-at="10@$demo2Png" --gif="4@10@$demo2Gif" --gif-fps=10
if ($LASTEXITCODE -ne 0) { throw "Demo 2 capture failed with exit code $LASTEXITCODE" }

Write-Host "Done. Output files:"
Get-ChildItem $outDir -Filter "demo*" | Select-Object Name, Length
