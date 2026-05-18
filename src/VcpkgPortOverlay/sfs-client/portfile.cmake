vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO microsoft/sfs-client
    REF 0e27525d597c730e71646fd0b15bdc8c8503f24d
    SHA512 d926d7fdbbd120cbcbd9732a3300cccfeed4a90d6b94456d73a70675df3578a91127f7e9f310fe68d18fa34bb997c29c8455e586d81a2ba404cf19193a80ca6e
    HEAD_REF main
    PATCHES
        remove-unconditional-toolchain-override.patch
)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSFS_BUILD_TESTS=OFF
        -DSFS_BUILD_SAMPLES=OFF
)
vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()

vcpkg_cmake_config_fixup(PACKAGE_NAME sfsclient CONFIG_PATH lib/cmake/sfsclient)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
