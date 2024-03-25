#!/bin/pwsh

$project_dir = Resolve-Path ${PSScriptRoot}/..

$config = @"
[2] # Version number

[[workspace]]

[workspace dirs]
${project_dir}

[ignore dirs]
.git

build
psp_build
linux_build

glfw
glad-gl-4.2-core
HandmadeMath
psp-stb

[[build commands]]
build_working_dir:                 ${project_dir}
open_panel_on_build:               true
close_panel_on_success:            false
clear_build_output_before_running: true

[Desktop (Debug)]
build_command: pwsh -Command "cmake -B build && cmake --build build --config Debug"
key_binding: F9

[Desktop (Release)]
build_command: pwsh -Command "cmake -B build && cmake --build build --config Release"

[PSP]
build_command: pwsh -Command "scripts/build_psp.ps1"
"@

$config | Out-File "${PSScriptRoot}/procyon.focus-config"