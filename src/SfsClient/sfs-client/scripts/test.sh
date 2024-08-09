#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Synopsis: Simplifies test commands for the SFS Client.
#
# Description: This script will contain the test commands for the SFS Client.
# Use this on non-Windows platforms in a bash session.
#
# Example:
# $ ./scripts/test.sh
#

# Ensures script stops on errors
set -e

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    error "Script is being sourced, it should be executed instead."
    return 1
fi

output_on_failure=false

usage() { echo "Usage: $0 [--output-on-failure]" 1>&2; exit 1; }

if ! opts=$(getopt \
  --longoptions "output-on-failure" \
  --name "$(basename "$0")" \
  --options "" \
  -- "$@"
); then
    usage
fi

eval set "--$opts"

while [ $# -gt 0 ]; do
    case "$1" in
        --output-on-failure)
            output_on_failure=true
            shift 1
            ;;
        *)
            break
            ;;
    esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
git_root=$(git -C "$script_dir" rev-parse --show-toplevel)
build_folder="$git_root/build"

cmd="ctest --test-dir \"$build_folder/client\""

if $output_on_failure ; then
    cmd+=" --output-on-failure"
fi

eval "$cmd"
