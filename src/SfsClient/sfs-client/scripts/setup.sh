#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Synopsis: Sets up dependencies required to build and work with the SFS Client.
#
# Description: This script will install all of the dependencies required to build and work with the SFS Client.
# Use this on non-Windows platforms in a bash session. It must be sourced.
#
# Example:
# $ source ./scripts/setup.sh
#

COLOR_RED="\033[1;31m"
COLOR_CYAN="\033[1;36m"
NO_COLOR="\033[0m"

header() { echo -e "${COLOR_CYAN}$*${NO_COLOR}"; }
error() { echo -e "${COLOR_RED}$*${NO_COLOR}" >&2; }

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    error "Script is being run directly, it should be sourced instead."
    exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
git_root=$(git -C "$script_dir" rev-parse --show-toplevel)

sudo apt-get -qq update

install_with_apt() {
    program_to_install=$1

    if ! sudo apt-get -qq install "$program_to_install"; then
        error "Failed to install $program_to_install"
        return 1
    fi
    return 0
}

install_python() {
    header "Installing latest Python if it's not installed"

    if ! install_with_apt "python3"; then
        return
    fi
    if ! install_with_apt "python3-pip"; then
        return
    fi
    return 0
}

install_pip_dependencies() {
    header "\nInstalling dependencies using pip"

    # Upgrade pip and install requirements. Filter out output for dependencies that are already installed
    python3 -m pip install --upgrade pip | grep -v -e "already satisfied" -e "Defaulting to user installation"
    ret="${PIPESTATUS[0]}"
    if [ "$ret" -ne 0 ]; then
        return "$ret"
    fi

    pip_reqs="$script_dir/pip.requirements.txt"

    pip install -r "$pip_reqs" | grep -v -e "already satisfied" -e "Defaulting to user installation"
    ret="${PIPESTATUS[0]}"
    if [ "$ret" -ne 0 ]; then
        return "$ret"
    fi

    return 0
}

install_cmake() {
    header "\nInstalling cmake if it's not installed"

    if ! install_with_apt "cmake"; then
        return
    fi
    return 0
}

install_vcpkg() {
    header "\nSetting up vcpkg"

    vcpkg_dir="$git_root/vcpkg"
    if [ -d "$vcpkg_dir" ]; then
        echo "Checking if vcpkg repo has new commits"
        git -C "$vcpkg_dir" pull --show-forced-updates | grep -v 'Already up to date'
    else
        echo "Cloning vcpkg repo"
        git clone https://github.com/microsoft/vcpkg "$vcpkg_dir"
        # Needed for the bootstrap and for other vcpkg packages
        if ! sudo apt-get -qq install curl zip unzip tar pkg-config; then
            return
        fi
        "$vcpkg_dir/bootstrap-vcpkg.sh"
    fi
    return 0
}

# Dependency for compiling CorrelationVector C++ library
install_uuid() {
    header "\nInstalling uuid-dev if it's not installed"

    if ! install_with_apt "pkg-config"; then
        return
    fi
    if ! install_with_apt "uuid-dev"; then
        return
    fi
    return 0
}

set_git_hooks() {
    header "\nSetting Git hooks"

    hook_dest_dir="$git_root/.git/hooks"
    declare -A git_hooks=( ["pre-commit-wrapper.sh"]="pre-commit")
    for src in "${!git_hooks[@]}"; do
        hook_src="$git_root/$src"
        hook_dest="$hook_dest_dir/${git_hooks[$src]}"

        # If the destination doesn't exist or is different than the one in the source, we'll copy it over.
        if [ ! -f "$hook_dest" ] || ! cmp -s "$hook_src" "$hook_dest"; then
            cp -f "$hook_src" "$hook_dest"
            echo "Setup git ${git_hooks[$src]} hook with $hook_src"
        fi
    done
}

set_aliases() {
    header "\nSetting aliases"

    python_scripts_dir=$(python3 -c 'import os,sysconfig;print(sysconfig.get_path("scripts",f"{os.name}_user"))')

    declare -A aliases=( ["build"]="$git_root/scripts/build.sh"
        ["clang-format"]="$python_scripts_dir/clang-format"
        ["cmake-format"]="$python_scripts_dir/cmake-format"
        ["test"]="$git_root/scripts/test.sh"
        ["vcpkg"]="$git_root/vcpkg/vcpkg")

    for alias in "${!aliases[@]}"; do
        target="${aliases[$alias]}"

        # If the alias doesn't exist or is set to something different than the one we want to target, we'll add it.

        # If alias doesn't exist, the alias command returns a non-zero result
        alias_does_not_exist=false
        alias_exists_not_the_same=false
        if ! alias "$alias" > /dev/null 2>&1; then
            alias_does_not_exist=true
        else
            output=$(alias "$alias")
            expected_output="alias $alias='$target'"
            if [ "$output" != "$expected_output" ]; then
                alias_exists_not_the_same=true
            fi
        fi

        if $alias_does_not_exist || $alias_exists_not_the_same; then
            eval "alias $alias=$target"
            echo "Setup alias $alias"
        fi
    done
}

if ! install_python; then
    return
fi

if ! install_pip_dependencies; then
    return
fi

if ! install_cmake; then
    return
fi

if ! install_vcpkg; then
    return
fi

if ! install_uuid; then
    return
fi

set_git_hooks
set_aliases
