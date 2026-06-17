$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$html = Join-Path $PSScriptRoot "manual.html"
$pdf = Join-Path $root "manual.pdf"
$chrome = "${env:ProgramFiles}\Google\Chrome\Application\chrome.exe"

if (-not (Test-Path $html)) {
    throw "Missing $html"
}

& (Join-Path $PSScriptRoot "extract_logo.ps1") | Out-Null
if (-not (Test-Path $chrome)) {
    throw "Chrome not found at $chrome"
}

if (Test-Path $pdf) {
    try {
        Remove-Item $pdf -Force -ErrorAction Stop
    } catch {
        $pdf = Join-Path $root "manual-new.pdf"
    }
}

& $chrome `
    --headless=new `
    --disable-gpu `
    --no-pdf-header-footer `
    --print-to-pdf="$pdf" `
    "file:///$($html.Replace('\','/'))" 2>$null

Start-Sleep -Seconds 1

if (-not (Test-Path $pdf)) {
    throw "PDF generation failed"
}

Write-Output "Created $pdf"
