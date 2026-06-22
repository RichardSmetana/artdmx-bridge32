# Generate manual-de.pdf from docs/manual-de.html (German)
# Requires: wkhtmltopdf — winget install wkhtmltopdf.wkhtmltox

$ErrorActionPreference = "Stop"
$wk = "${env:ProgramFiles}\wkhtmltopdf\bin\wkhtmltopdf.exe"
if (-not (Test-Path $wk)) {
  throw "wkhtmltopdf not found at $wk. Install: winget install wkhtmltopdf.wkhtmltox"
}

$root = Split-Path $PSScriptRoot -Parent
$html = Join-Path $PSScriptRoot "manual-de.html"
$pdf = Join-Path $root "manual-de.pdf"

& $wk `
  --enable-local-file-access `
  --print-media-type `
  --margin-top 12mm --margin-bottom 14mm --margin-left 12mm --margin-right 12mm `
  --footer-center "[page] / [topage]" `
  --footer-font-size 8 `
  $html $pdf

Write-Host "Wrote $pdf"
