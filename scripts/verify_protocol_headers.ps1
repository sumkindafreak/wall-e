# Verify canonical wire-protocol headers exist (run from repo root).
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$canon = Join-Path $root "firmware_common\include"
foreach ($f in @("node_health_protocol.h", "walle_link_packet.h", "audio_protocol.h")) {
    $p = Join-Path $canon $f
    if (-not (Test-Path $p)) {
        Write-Error "Missing canonical protocol header: $p"
        exit 1
    }
}
Write-Host "OK: firmware_common protocol headers present."
exit 0
