#!/usr/bin/env powershell

$repository_root = $(Resolve-Path "${PSScriptRoot}/..")
$repository_root_dirname = Split-Path $repository_root -Leaf

$docker_run_options = @(
    "--tty",
    "--rm",
    "--volume", "${repository_root}:/${repository_root_dirname}",
    "--workdir", "/${repository_root_dirname}"
)
$docker_command = @(
    "/bin/bash", "-c",
    "psp-cmake -B psp_build && make --directory=psp_build"
)
docker run @docker_run_options pspdev/pspdev @docker_command