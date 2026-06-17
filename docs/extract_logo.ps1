$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$header = Join-Path $root "logo_data.h"
$png = Join-Path $PSScriptRoot "artdmx-bridge32-logo.png"

if (-not (Test-Path $header)) {
    throw "Missing $header"
}

$content = Get-Content $header -Raw
$matches = [regex]::Matches($content, '0x([0-9A-Fa-f]{2})')
if ($matches.Count -eq 0) {
    throw "No PNG bytes found in logo_data.h"
}

$bytes = New-Object byte[] $matches.Count
for ($i = 0; $i -lt $matches.Count; $i++) {
    $bytes[$i] = [byte]::Parse($matches[$i].Groups[1].Value, [System.Globalization.NumberStyles]::HexNumber)
}

[System.IO.File]::WriteAllBytes($png, $bytes)
Write-Output "Created $png ($($bytes.Length) bytes)"
