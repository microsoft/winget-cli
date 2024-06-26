#!/usr/bin/env bash

#
# Shellcheck is a static analyzer for shell scripts: https://shellcheck.net/
# It is available in several operating systems and also as a docker image.
#
# If it finds any issues, it will output a small blurb describing the affected
# line(s) and will have a generic issue ID. The issue ID can be opened on its
# website to learn more about what the underlying problem is, why it's a
# problem, and (usually) suggests a way to fix.
# Specific shellcheck issues can be disabled (aka silenced). Doing so is
# usually pretty loud during code review.
# https://github.com/koalaman/shellcheck/wiki/Directive

# https://stackoverflow.com/a/2871034/1111557
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SHELLCHECK="${SHELLCHECK:-"/usr/bin/shellcheck"}"
SEARCH_DIR="${SEARCH_DIR:-"$HERE"}"
cd "${SEARCH_DIR}" #so that we can call git

#
# This block will:
# 1) `find` files under `SEARCH_DIR`
# 2) skip anything under `/thirdparty/`, `/.git/`
# 3) in a loop reading each path:
# 3a) ignore files that git also ignores
# 3b) use `file` to filter only script files
# 3c) run shellcheck against that script
# 4) if any paths are found to have an error, their paths are collated.
FAILED_PATHS=()
while read -r file_path
do
  if git rev-parse --git-dir > /dev/null 2>&1;
  then
    git check-ignore --quiet "${file_path}" && continue
  fi
  file "${file_path}" | grep -q 'shell script' || continue
  SCRIPT_PATH="${file_path}"
  echo "Checking: ${SCRIPT_PATH}"
  "${SHELLCHECK}" \
    "${SCRIPT_PATH}" \
  || FAILED_PATHS+=( "${SCRIPT_PATH}" )
done < <(
  find "${SEARCH_DIR}" -type f \
  | grep -v '/\.git/\|/thirdparty/'
)

#
# If there are any failed paths, summarize them here.
# Then report a failing status to our caller.
if [[ 0 -lt "${#FAILED_PATHS[@]}" ]]; then
  >&2 echo "These scripts aren't shellcheck-clean:"
  for path in "${FAILED_PATHS[@]}"; do
    >&2 echo "${path}"
  done
  exit 1
fi

# If we get here, then none of the scripts had any warnings.
echo "All scripts found (listed above) passed shellcheck"
