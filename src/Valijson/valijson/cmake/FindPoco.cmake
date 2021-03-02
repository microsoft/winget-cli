# - Try to find the required Poco components (default: Zip Net NetSSL Crypto Util XML CppParser Foundation)
#
# Once done this will define
#  Poco_FOUND         - System has the all required components.
#  Poco_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  Poco_LIBRARY_DIRS  - Library directories necessary for using the required components.
#  Poco_LIBRARIES     - Link these to use the required components.
#  Poco_DEFINITIONS   - Compiler switches required for using the required components.
#
# For each of the components it will additionally set.
#   - Foundation
#   - CppParser
#   - CppUnit
#   - Net
#   - NetSSL
#   - Crypto
#   - Util
#   - XML
#   - Zip
#   - Data
#   - PageCompiler
#
# the following variables will be defined
#  Poco_<component>_FOUND        - System has <component>
#  Poco_<component>_INCLUDE_DIRS - Include directories necessary for using the <component> headers
#  Poco_<component>_LIBRARY_DIRS - Library directories necessary for using the <component>
#  Poco_<component>_LIBRARIES    - Link these to use <component>
#  Poco_<component>_DEFINITIONS  - Compiler switches required for using <component>
#  Poco_<component>_VERSION      - The components version

include(CMakeHelpers REQUIRED)
include(CMakeFindExtensions REQUIRED)

# The default components to find
if (NOT Poco_FIND_COMPONENTS)
  set(Poco_FIND_COMPONENTS Zip Net NetSSL Crypto Util XML CppParser Foundation)
endif()

# Set required variables
set(Poco_ROOT_DIR "" CACHE STRING "Where is the Poco root directory located?")
set(Poco_INCLUDE_DIR "${Poco_ROOT_DIR}" CACHE STRING "Where are the Poco headers (.h) located?")
set(Poco_LIBRARY_DIR "${Poco_ROOT_DIR}/lib" CACHE STRING "Where are the Poco libraries (.dll/.so) located?")
set(Poco_LINK_SHARED_LIBS FALSE CACHE BOOL "Link with shared Poco libraries (.dll/.so) instead of static ones (.lib/.a)")

#message("Poco_ROOT_DIR=${Poco_ROOT_DIR}")
#message("Poco_INCLUDE_DIR=${Poco_INCLUDE_DIR}")
#message("Poco_LIBRARY_DIR=${Poco_LIBRARY_DIR}")

# Check for cached results. If there are then skip the costly part.
set_module_notfound(Poco)
if (NOT Poco_FOUND)

  # Set the library suffix for our build type
  set(Poco_LIB_SUFFIX "")
  if(WIN32 AND MSVC)
    set(Poco_MULTI_CONFIGURATION TRUE)
    # add_definitions(-DPOCO_NO_AUTOMATIC_LIBS)
    # add_definitions(-DPOCO_NO_UNWINDOWS)
    if(Poco_LINK_SHARED_LIBS)
      add_definitions(-DPOCO_DLL)
    else()
      add_definitions(-DPOCO_STATIC)
      if(BUILD_WITH_STATIC_CRT) 
        set(Poco_LIB_SUFFIX "mt")
      else()
        set(Poco_LIB_SUFFIX "md")
      endif()
    endif()
  endif()

  # Set search path suffixes
  set(Poco_INCLUDE_SUFFIXES
    CppUnit/include
    CppParser/include
    Data/include
    Data/ODBC/include
    Foundation/include
    Crypto/include
    Net/include
    NetSSL_OpenSSL/include
    PageCompiler/include
    Util/include
    JSON/include
    XML/include
    Zip/include
  )

  set(Poco_LIBRARY_SUFFIXES
    Linux/i686
    Linux/x86_64
  )

  # Check for all available components
  find_component(Poco CppParser    CppParser    PocoCppParser${Poco_LIB_SUFFIX}    Poco/CppParser/CppParser.h)
  find_component(Poco CppUnit      CppUnit      PocoCppUnit${Poco_LIB_SUFFIX}      Poco/CppUnit/CppUnit.h)
  find_component(Poco Net          Net          PocoNet${Poco_LIB_SUFFIX}          Poco/Net/Net.h)
  find_component(Poco NetSSL       NetSSL       PocoNetSSL${Poco_LIB_SUFFIX}       Poco/Net/NetSSL.h)
  find_component(Poco Crypto       Crypto       PocoCrypto${Poco_LIB_SUFFIX}       Poco/Crypto/Crypto.h)
  find_component(Poco Util         Util         PocoUtil${Poco_LIB_SUFFIX}         Poco/Util/Util.h)
  find_component(Poco JSON         JSON         PocoJSON${Poco_LIB_SUFFIX}         Poco/JSON/JSON.h)
  find_component(Poco XML          XML          PocoXML${Poco_LIB_SUFFIX}          Poco/XML/XML.h)
  find_component(Poco Zip          Zip          PocoZip${Poco_LIB_SUFFIX}          Poco/Zip/Zip.h)
  find_component(Poco Data         Data         PocoData${Poco_LIB_SUFFIX}         Poco/Data/Data.h)
  find_component(Poco ODBC         ODBC         PocoDataODBC${Poco_LIB_SUFFIX}     Poco/Data/ODBC/Connector.h)
  find_component(Poco PageCompiler PageCompiler PocoPageCompiler${Poco_LIB_SUFFIX} Poco/PageCompiler/PageCompiler.h)
  find_component(Poco Foundation   Foundation   PocoFoundation${Poco_LIB_SUFFIX}   Poco/Foundation.h)

  # Set Poco as found or not
  # print_module_variables(Poco)
  set_module_found(Poco)
endif ()
