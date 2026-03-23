# Build LROS WebUI for ESP32 PROGMEM embedding
# Output: web_page_lros.h (replace web_page.h content or use conditionally)

$standalone = Join-Path $PSScriptRoot "index-standalone.html"
$out = Join-Path (Join-Path (Join-Path $PSScriptRoot "..") "main_wall_E_base") "main\web_page_lros.h"

if (-not (Test-Path $standalone)) {
    Write-Host "Run first: Create index-standalone.html (inline css+js)"
    $css = Get-Content (Join-Path $PSScriptRoot "css\lros.css") -Raw
    $js = Get-Content (Join-Path $PSScriptRoot "js\lros.js") -Raw
    $html = Get-Content (Join-Path $PSScriptRoot "index.html") -Raw
    $html = $html -replace '<link rel="stylesheet" href="css/lros.css">', "<style>$css</style>"
    $html = $html -replace '<script src="js/lros.js"></script>', "<script>$js</script>"
    $html | Set-Content $standalone -Encoding UTF8
}

$content = Get-Content $standalone -Raw -Encoding UTF8
# R"rawliteral(...)" uses content verbatim — do not escape

$header = @"
#pragma once
#include <Arduino.h>
// WALL-E LROS Web UI - Built from webui/
// Replace WALLE_PAGE in web_page.h with WALLE_PAGE_LROS to use this UI.
const char WALLE_PAGE_LROS[] PROGMEM = R"rawliteral(
"@

$footer = @"
)rawliteral";
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($out, $header + $content + $footer, $utf8NoBom)
Write-Host "Built: $out"
Write-Host "To use: In web_page.h, use WALLE_PAGE_LROS instead of WALLE_PAGE, or merge the content."
