# Define custom options for CMake.
# Options are passed to cmake through command line, -Doption=value
option(SFS_BUILD_TESTS "Indicates if tests should be built." OFF)

option(SFS_BUILD_SAMPLES "Indicates if samples should be built." OFF)

option(
    SFS_ENABLE_TEST_OVERRIDES
    "Set SFS_ENABLE_OVERRIDES to ON to enable certain test overrides through environment variables."
    OFF)

option(
    SFS_WINDOWS_STATIC_ONLY
    "Indicates if only static libraries and dependencies should be built on Windows."
    OFF)
