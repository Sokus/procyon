#!/usr/bin/pwsh

Push-Location "${PSScriptRoot}/.."

& odin build src/tools/map_compiler -o:none -debug -collection:p=src/tools/map_compiler "-out=build/map_compiler.exe"

Pop-Location
