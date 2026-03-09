# WALL-E OTA Build Script (PowerShell)
# Builds all OTA-enabled projects. Run from wall-e repo root.
# Usage: .\ota_build_all.ps1
# Upload: .\ota_build_all.ps1 -Upload

param([switch]$Upload)

$ErrorActionPreference = "Stop"
$projects = @(
    @{ Name = "Base"; Path = "main_wall_E_base"; Env = "wall_e_brain_s3" },
    @{ Name = "Controller"; Path = "wall_e_master_controller"; Env = "cyd_esp32_2432s028" },
    @{ Name = "Dock"; Path = "dock_station"; Env = "dock_esp32" }
)

$pio = "platformio"
if (-not (Get-Command $pio -ErrorAction SilentlyContinue)) {
    $pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
}
if (-not (Test-Path $pio) -and -not (Get-Command platformio -ErrorAction SilentlyContinue)) {
    Write-Host "PlatformIO not found. Install: pip install platformio" -ForegroundColor Red
    exit 1
}

foreach ($p in $projects) {
    $dir = Join-Path $PSScriptRoot $p.Path
    if (-not (Test-Path (Join-Path $dir "platformio.ini"))) {
        Write-Host "[$($p.Name)] Skipping (no platformio.ini)" -ForegroundColor Yellow
        continue
    }
    Write-Host "`n[$($p.Name)] Building $($p.Path)..." -ForegroundColor Cyan
    Push-Location $dir
    try {
        & $pio run -e $p.Env
        if ($LASTEXITCODE -ne 0) { throw "Build failed" }
        if ($Upload) {
            Write-Host "[$($p.Name)] Uploading (OTA)..." -ForegroundColor Cyan
            & $pio run -e $p.Env -t upload
            if ($LASTEXITCODE -ne 0) { throw "Upload failed" }
        }
    } finally {
        Pop-Location
    }
}
Write-Host "`nDone." -ForegroundColor Green
