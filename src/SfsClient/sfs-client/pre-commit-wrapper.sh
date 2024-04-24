#!/bin/sh
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# A wrapper script to call the pre-commit.sh script
#
# Since git hooks are not committed to the server, existing only
# in the .git folder, this will be copied there and call the actual
# hook script only in case it exists. This should prevent issues
# with the user having to work on an earlier commit that does not
# have the hook or its dependencies installed.

root_dir=$(git rev-parse --show-toplevel)
hook_script="$root_dir/pre-commit.sh"
if [[ -f $hook_script ]]; then
	. "$hook_script"
fi
