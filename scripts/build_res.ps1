#!/usr/bin/pwsh

Push-Location "${PSScriptRoot}/.."

& "$blender" ./res/assets/fox.blend -b -P ./src/blender/p3d_export.py -- --output ./res/assets/fox.pp3d --portable --bone_weight_size 1 --no-indices

Pop-Location
