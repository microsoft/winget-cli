#!/usr/bin/env bash

#
# Exit codes:
#
# 0  - success
# 64 - usage
# 65 - invalid adapter
#

set -euo pipefail

adapter_path=include/valijson/adapters
utils_path=include/valijson/utils

# find all available adapters
pushd "${adapter_path}" > /dev/null
adapters=($(ls *.hpp))
popd > /dev/null

# remove _adapter.hpp suffix
adapters=("${adapters[@]/_adapter.hpp/}")

usage() {
  echo 'Generates a single header file for a particular Valijson adapter'
  echo
  echo 'This makes it easier to embed Valijson in smaller projects, where integrating a'
  echo 'third-party dependency is inconvenient or undesirable.'
  echo
  echo 'Output is written to STDOUT.'
  echo
  echo 'Usage:'
  echo
  echo '  ./bundle.sh <adapter-prefix>'
  echo
  echo 'Example usage:'
  echo
  echo '  ./bundle.sh nlohmann_json > valijson_nlohmann_bundled.hpp'
  echo
  echo 'Available adapters:'
  echo
  for adapter in "${adapters[@]}"; do
    echo "  - ${adapter}"
  done
  echo
  exit 64
}

if [ $# -ne 1 ]; then
  usage
fi

adapter_header=
for adapter in "${adapters[@]}"; do
  if [ "${adapter}" == "$1" ]; then
    adapter_header="${adapter_path}/${adapter}_adapter.hpp"
    break
  fi
done

if [ -z "${adapter_header}" ]; then
  echo "Error: Adapter name is not valid."
  exit 65
fi

common_headers=(
  include/valijson/exceptions.hpp
  include/compat/optional.hpp
  include/valijson/internal/optional_bundled.hpp
  include/valijson/internal/adapter.hpp
  include/valijson/internal/basic_adapter.hpp
  include/valijson/internal/custom_allocator.hpp
  include/valijson/internal/debug.hpp
  include/valijson/internal/frozen_value.hpp
  include/valijson/internal/json_pointer.hpp
  include/valijson/internal/json_reference.hpp
  include/valijson/internal/uri.hpp
  include/valijson/utils/file_utils.hpp
  include/valijson/utils/utf8_utils.hpp
  include/valijson/constraints/constraint.hpp
  include/valijson/subschema.hpp
  include/valijson/schema.hpp
  include/valijson/constraints/constraint_visitor.hpp
  include/valijson/constraints/basic_constraint.hpp
  include/valijson/constraints/concrete_constraints.hpp
  include/valijson/constraint_builder.hpp
  include/valijson/schema_parser.hpp
  include/valijson/adapters/std_string_adapter.hpp
  include/valijson/validation_results.hpp
  include/valijson/validation_visitor.hpp
  include/valijson/validator.hpp)

# remove internal #includes
grep --no-filename -v "include <valijson/" ${common_headers[@]}

# std_string_adapter is always included
if [ "${adapter}" != "std_string" ]; then
  grep --no-filename -v "include <valijson/" "${adapter_header}"
fi

# include file utils if available
utils_header="${utils_path}/${adapter}_utils.hpp"
if [ -f "${utils_header}" ]; then
  grep --no-filename -v "include <valijson/" "${utils_header}"
fi
