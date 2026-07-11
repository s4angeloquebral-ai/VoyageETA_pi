$ErrorActionPreference = "Stop"

$here = Split-Path -Parent $PSScriptRoot
$tmp = Join-Path $env:TEMP "voyage_eta_testplugin"

if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }

git clone --depth 1 https://github.com/jongough/testplugin_pi.git $tmp

Copy-Item "$here\src\*" "$tmp\src\" -Force
Copy-Item "$here\CMakeLists.txt" "$tmp\CMakeLists.txt" -Force
Copy-Item "$here\README.md" "$tmp\README.md" -Force
Copy-Item "$here\LICENSE" "$tmp\LICENSE" -Force

Write-Host ""
Write-Host "Prepared source tree:"
Write-Host $tmp
Write-Host ""
Write-Host "Open a Developer PowerShell and build from that folder."
