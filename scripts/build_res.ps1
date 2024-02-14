#!/usr/bin/env powershell

$blender = ""
if (-not $blender -and $env:PROCYON_BLENDER_PATH) {
    $blender = ${env:PROCYON_BLENDER_PATH}
}
if (-not $blender) {
    $blender = "blender"
}

$blender_not_found_message = @"
build_res.ps1 : Could not start Blender with "${blender}"
Make sure the command is available or add executable path to PROCYON_BLENDER_PATH
"@
if (-not (Test-Path -Path $blender -PathType Leaf)) {
    Write-Host $blender_not_found_message
    exit 1
}

Push-Location "${PSScriptRoot}/.."

& "$blender" ./res/fox.blend -b -P ./src/blender/p3d_export.py -- --output ./res/fox.pp3d --portable --bone_weight_size 1 --no-indices

Pop-Location
