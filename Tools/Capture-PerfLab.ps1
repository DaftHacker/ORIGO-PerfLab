[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineExe,
    [ValidateSet('static','moving','physics','particles','empty')][string]$Scenario = 'static',
    [ValidateSet(0,100,500,1000,5000,10000,25000,50000)][int]$Count = 1000,
    [ValidateRange(0,600)][int]$WarmupSeconds = 10,
    [ValidateRange(1,600)][int]$CaptureSeconds = 30,
    [switch]$CameraSweep
)

$ErrorActionPreference = 'Stop'
$EngineExe = (Resolve-Path $EngineExe).Path
$ProjectFile = (Resolve-Path (Join-Path $PSScriptRoot '..\ORIGO-PerfLab.origoproject')).Path
$ResultsDir = Join-Path $PSScriptRoot '..\Results'
New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

if ($Scenario -eq 'physics' -and ($Count -lt 1 -or $Count -gt 1000)) {
    throw 'Physics captures support 100 or 500 bodies through this helper; use a custom environment for larger experiments.'
}
if (($Scenario -eq 'static' -or $Scenario -eq 'moving') -and $Count -lt 1) {
    throw 'Static and moving scenarios require Count >= 1.'
}
if ($Scenario -eq 'particles' -and $Count -lt 100) {
    throw 'Particle captures require Count >= 100.'
}
if ($Scenario -eq 'empty') { $Count = 0 }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$label = "perflab-$Scenario-$Count-$stamp"
$profileFile = Join-Path $ResultsDir "$label.txt"
$metadataFile = Join-Path $ResultsDir "$label.meta.txt"

@(
    "Label: $label"
    "Date: $((Get-Date).ToString('o'))"
    "Engine executable: $EngineExe"
    "Project: $ProjectFile"
    "Scenario: $Scenario"
    "Count: $Count"
    "Warm-up seconds: $WarmupSeconds"
    "Capture seconds: $CaptureSeconds"
    "Camera sweep: $([bool]$CameraSweep)"
    "Build/configuration: record the exact engine build used for this run"
) | Set-Content -Encoding UTF8 $metadataFile

# Preserve this PowerShell process's environment even when the child fails.
$keys = @(
    'ORIGO_FRAME_PROFILE',
    'ORIGO_FRAME_PROFILE_WARMUP',
    'ORIGO_FRAME_PROFILE_SECONDS',
    'ORIGO_FRAME_PROFILE_LABEL',
    'ORIGO_FRAME_PROFILE_EXIT',
    'ORIGO_PERFLAB_SCENARIO',
    'ORIGO_PERFLAB_COUNT',
    'ORIGO_PERFLAB_HUD',
    'ORIGO_PERFLAB_CAMERA_SWEEP'
)
$previous = @{}
foreach ($key in $keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process') }

try {
    $env:ORIGO_FRAME_PROFILE = $profileFile
    $env:ORIGO_FRAME_PROFILE_WARMUP = [string]$WarmupSeconds
    $env:ORIGO_FRAME_PROFILE_SECONDS = [string]$CaptureSeconds
    $env:ORIGO_FRAME_PROFILE_LABEL = $label
    $env:ORIGO_FRAME_PROFILE_EXIT = '1'
    $env:ORIGO_PERFLAB_SCENARIO = $Scenario
    $env:ORIGO_PERFLAB_COUNT = [string]$Count
    $env:ORIGO_PERFLAB_HUD = '0'
    $env:ORIGO_PERFLAB_CAMERA_SWEEP = if ($CameraSweep) { '1' } else { '0' }

    Write-Host "Starting ORIGO PerfLab: $Scenario / $Count"
    Write-Host "Warm-up: $WarmupSeconds s; capture: $CaptureSeconds s"
    Write-Host "Profile: $profileFile"
    & $EngineExe $ProjectFile --play-only --auto-play
    if ($LASTEXITCODE -ne 0) {
        throw "ORIGO exited with code $LASTEXITCODE. Check the engine Console/Build Output and runtime logs."
    }
} finally {
    foreach ($key in $keys) {
        [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process')
    }
}

if (-not (Test-Path $profileFile)) {
    throw "ORIGO exited without creating the expected profile: $profileFile. Confirm this engine build includes ORIGO_FRAME_PROFILE support."
}
Write-Host "Capture complete:"
Write-Host "  $profileFile"
Write-Host "  $metadataFile"
