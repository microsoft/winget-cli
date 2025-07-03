if(VCPKG_TARGET_IS_WINDOWS)
    set(PATCHES fix-POSIX_name.patch)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO yaml/libyaml
    REF 840b65c40675e2d06bf40405ad3f12dec7f35923 # Unreleased
    SHA512 de85560312d53a007a2ddf1fe403676bbd34620480b1ba446b8c16bb366524ba7a6ed08f6316dd783bf980d9e26603a9efc82f134eb0235917b3be1d3eb4b302
    HEAD_REF master
    PATCHES
        ${PATCHES}
        export-pkgconfig.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTING=OFF
        -DINSTALL_CMAKE_DIR=share/yaml
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(PACKAGE_NAME yaml CONFIG_PATH share/yaml)

vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/include/config.h" "${CURRENT_PACKAGES_DIR}/debug/share")

configure_file("${SOURCE_PATH}/License" "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" COPYONLY)
