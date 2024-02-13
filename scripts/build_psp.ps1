#!/usr/bin/env powershell

$repository_root = $(Resolve-Path "${PSScriptRoot}/..")

Push-Location $repository_root
wsl.exe --distribution Ubuntu export PSPDEV=~/pspdev`; export PATH=`$PATH:`$PSPDEV/bin`; psp-cmake -B psp_build
Pop-Location

Push-Location $repository_root/psp_build
wsl.exe --distribution Ubuntu make
Pop-Location

