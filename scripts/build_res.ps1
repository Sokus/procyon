#!/usr/bin/pwsh

Push-Location "${PSScriptRoot}/.."

& "$blender" ./res/fox.blend -b -P ./src/blender/p3d_export.py -- --output ./res/fox.pp3d --portable --bone_weight_size 1 --no-indices

Pop-Location
