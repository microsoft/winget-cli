#ifndef NOEXCEPT_H_768872DA_476C_11EA_88B8_90B11C0C0FF8
#define NOEXCEPT_H_768872DA_476C_11EA_88B8_90B11C0C0FF8

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// This is here for compatibility with older versions of Visual Studio
// which don't support noexcept.
#if defined(_MSC_VER) && _MSC_VER < 1900
    #define YAML_CPP_NOEXCEPT _NOEXCEPT
#else
    #define YAML_CPP_NOEXCEPT noexcept
#endif

#endif
