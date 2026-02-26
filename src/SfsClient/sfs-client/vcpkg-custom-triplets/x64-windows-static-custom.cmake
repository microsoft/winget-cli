set(VCPKG_TARGET_ARCHITECTURE x64)

message(STATUS "PORTs are preferred with static linkage...")

set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
