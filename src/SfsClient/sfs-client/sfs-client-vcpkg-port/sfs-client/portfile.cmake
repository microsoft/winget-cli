# This file is a template portfile to be used to consume the sfs-client library using the vcpkg infrastructure

# Choose one of the consumption formats below. SOURCE_PATH must be defined after either step.

# 1. Get from Github
# vcpkg_from_github(
#     OUT_SOURCE_PATH SOURCE_PATH
#     REPO microsoft/sfs-client
#     REF <commit-id>
#     SHA512 0 # Run vcpkg the first time and this value will be shown in the logs for you to replace
#     HEAD_REF main
# )

# 2. Use a locally available path, like a subtree'd repository
# set(SOURCE_PATH D:/sfs-client)

# Preferring static linkage in Windows
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

# Configure and install the library
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()

# Exposes the package targets like other vcpkg packages do
vcpkg_cmake_config_fixup(PACKAGE_NAME sfsclient CONFIG_PATH lib/cmake/sfsclient)

# Handle copyright
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Handle usage
file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# Remove duplicated include files
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
