# Build LROS WebUI for ESP32 PROGMEM embedding
# Reads webui/index.html + css/lros.css + js/*.js and writes main_wall_E_base/main/web_page_lros.h

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

$css = [System.IO.File]::ReadAllText((Join-Path $root "css\lros.css"), [System.Text.UTF8Encoding]::new($false))
$jsFiles = @(
  "js\walleConnection.js",
  "js\pathPlanner.js",
  "js\proximityAlert.js",
  "js\navWorldContext.js",
  "js\lros-navigation.js",
  "js\lros-maplibre-nav.js",
  "js\navMissionPanel.js",
  "js\lros-sequences.js",
  "js\lros.js"
)
$parts = foreach ($rel in $jsFiles) {
  [System.IO.File]::ReadAllText((Join-Path $root $rel), [System.Text.UTF8Encoding]::new($false))
}
$jsCombined = ($parts -join "`n`n")

$html = [System.IO.File]::ReadAllText((Join-Path $root "index.html"), [System.Text.UTF8Encoding]::new($false))

$html = $html.Replace(
  '<link rel="stylesheet" href="css/lros.css">',
  "<style>`n$css`n</style>"
)

$pattern = '(?s)<script src="js/walleConnection\.js"></script>\s*<script src="js/pathPlanner\.js"></script>\s*<script src="js/proximityAlert\.js"></script>\s*<script src="js/navWorldContext\.js"></script>\s*<script src="js/lros-navigation\.js"></script>\s*<script src="js/lros-maplibre-nav\.js"></script>\s*<script src="js/navMissionPanel\.js"></script>\s*<script src="js/lros-sequences\.js"></script>\s*<script src="js/lros\.js"></script>'
$embedded = "<script>`n" + $jsCombined + "`n</script>"
if (-not ([regex]::IsMatch($html, $pattern))) {
  Write-Error "Could not find expected script bundle in index.html; update build-embed.ps1 pattern."
}
$html = [regex]::Replace($html, $pattern, { param($m) return $embedded })

$out = Join-Path (Join-Path (Join-Path $root "..") "main_wall_E_base") "main\web_page_lros.h"

$header = @"
#pragma once
#include <Arduino.h>
// WALL-E LROS Web UI - Built from webui/ (run: webui/build-embed.ps1)
// Served at GET / via web_server.cpp
const char WALLE_PAGE_LROS[] PROGMEM = R"rawliteral(
"@

$footer = @"
)rawliteral";
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($out, $header + $html + $footer, $utf8NoBom)
Write-Host "Built: $out"
