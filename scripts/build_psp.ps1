#!/usr/bin/env powershell

param (
    [string]$BuildType = "Release"
)

$repository_root = $(Resolve-Path "${PSScriptRoot}/..")

$cmake_exit_code = 0
Push-Location $repository_root
wsl.exe --distribution Ubuntu export PSPDEV=~/pspdev`; export PATH=`$PATH:`$PSPDEV/bin`; psp-cmake -DCMAKE_BUILD_TYPE="${BuildType}" -B psp_build
$cmake_configure_exit_code = $LASTEXITCODE
Pop-Location

if ($cmake_exit_code -ne 0) {
    exit $cmake_exit_code
}

$make_exit_code = 0
Push-Location $repository_root/psp_build
wsl.exe --distribution Ubuntu make
$make_exit_code = $LASTEXITCODE
Pop-Location

if ($make_exit_code -ne 0) {
    exit $make_exit_code
}