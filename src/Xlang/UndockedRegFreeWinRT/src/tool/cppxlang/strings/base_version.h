
#define CPPXLANG_VERSION "%"

// XLANG_version is used by Microsoft to analyze cppxlang library adoption and inform future product decisions.
#ifdef _WIN32
extern "C"
__declspec(selectany) char const * const XLANG_version = "cppxlang version:" CPPXLANG_VERSION;
#else
extern "C"
inline char const * const XLANG_version = "cppxlang version:" CPPXLANG_VERSION;
#endif

#ifdef _M_IX86
#pragma comment(linker, "/include:_XLANG_version")
#else
#pragma comment(linker, "/include:XLANG_version")
#endif

namespace xlang
{
    template <size_t BaseSize, size_t ComponentSize>
    constexpr bool check_version(char const(&base)[BaseSize], char const(&component)[ComponentSize]) noexcept
    {
        if constexpr (BaseSize != ComponentSize)
        {
            return false;
        }

        for (size_t i = 0; i != BaseSize - 1; ++i)
        {
            if (base[i] != component[i])
            {
                return false;
            }
        }

        return true;
    }
}
