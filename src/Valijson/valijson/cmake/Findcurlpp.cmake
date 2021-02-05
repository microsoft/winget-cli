include(FindPkgConfig)
include(FindPackageHandleStandardArgs)

pkg_check_modules(curlpp_PKGCONF REQUIRED curlpp)

find_path(curlpp_INCLUDE_DIR
        NAMES curlpp/cURLpp.hpp
        PATHS ${curlpp_PKGCONF_INCLUDE_DIRS}
        )

find_library(curlpp_LIBRARIES
        NAMES curlpp
        PATHS ${curlpp_PKGCONF_LIBRARY_DIRS}
        )

if(curlpp_PKGCONF_FOUND)
    set(curlpp_FOUND yes)
else()
    set(curlpp_FOUND no)
endif()
