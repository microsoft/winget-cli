#pragma once

#ifndef XLANG_PAL_H
#define XLANG_PAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#include <array>
#include <type_traits>
#endif

#ifdef _WIN32
#define XLANG_PLATFORM_WINDOWS 1
#else
#define XLANG_PLATFORM_WINDOWS 0
#endif

#ifdef _MSC_VER
#define XLANG_COMPILER_CLANG 0
#define XLANG_COMPILER_MSVC 1
#elif defined(__clang__)
#define XLANG_COMPILER_CLANG 1
#define XLANG_COMPILER_MSVC 0
#else
#error Xlang only tested against Visual C++ and clang
#endif


#if XLANG_COMPILER_MSVC && defined(_M_IX86)
#define XLANG_CALL __stdcall
#elif defined(__clang__) && defined(__i386__)
#define XLANG_CALL __stdcall
#else
#define XLANG_CALL
#endif

#if XLANG_PLATFORM_WINDOWS
# define XLANG_EXPORT_DECL __declspec(dllexport)
# define XLANG_IMPORT_DECL __declspec(dllimport)
#else
# define XLANG_EXPORT_DECL __attribute__ ((visibility ("default")))
# define XLANG_IMPORT_DECL 
#endif

#ifndef XLANG_PAL_EXPORT
# ifdef XLANG_PAL_EXPORTS
#  define XLANG_PAL_EXPORT XLANG_EXPORT_DECL
# else
#  define XLANG_PAL_EXPORT XLANG_IMPORT_DECL
# endif
#endif

#ifdef __cplusplus
# define XLANG_NOEXCEPT noexcept
# if XLANG_COMPILER_MSVC
#  define XLANG_EBO __declspec(empty_bases)
#  define XLANG_NOVTABLE __declspec(novtable)
# else
#  define XLANG_EBO
#  define XLANG_NOVTABLE
# endif
#else
# define XLANG_NOEXCEPT
# define XLANG_EBO
# define XLANG_NOVTABLE
#endif

#ifndef XLANG_PAL_HAS_CHAR8_T
# ifdef __cpp_char8_t
#  define XLANG_PAL_HAS_CHAR8_T 1
# else
#  define XLANG_PAL_HAS_CHAR8_T 0
# endif
#endif

#if XLANG_PAL_HAS_CHAR8_T
#define XLANG_PAL_CHAR8_T char8_t
#else
#define XLANG_PAL_CHAR8_T char
#endif

#ifdef __IUnknown_INTERFACE_DEFINED__
#define XLANG_WINDOWS_ABI
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    // Type/handle definitions

    struct xlang_guid
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t  Data4[8];

#ifdef __cplusplus
        xlang_guid() noexcept = default;

        constexpr xlang_guid(uint32_t Arg1, uint16_t Arg2, uint16_t Arg3, std::array<uint8_t, 8> const& Arg4) noexcept
            : Data1(Arg1)
            , Data2(Arg2)
            , Data3(Arg3)
            , Data4{ Arg4[0], Arg4[1],Arg4[2],Arg4[3],Arg4[4],Arg4[5],Arg4[6],Arg4[7] }
        {
        }

# ifdef XLANG_WINDOWS_ABI
        constexpr xlang_guid(GUID const& value) noexcept
            : Data1(value.Data1)
            , Data2(value.Data2)
            , Data3(value.Data3)
            , Data4{ value.Data4[0], value.Data4[1], value.Data4[2], value.Data4[3], value.Data4[4], value.Data4[5], value.Data4[6], value.Data4[7] }
        {
        }

        operator GUID const&() const noexcept
        {
            static_assert(sizeof(xlang_guid) == sizeof(GUID));
            return reinterpret_cast<GUID const&>(*this);
        }
# endif
#endif

        inline bool operator==(xlang_guid const& right) const noexcept
        {
            auto const left_val = reinterpret_cast<uint32_t const*>(this);
            auto const right_val = reinterpret_cast<uint32_t const*>(&right);
            return left_val[0] == right_val[0] && left_val[1] == right_val[1] && left_val[2] == right_val[2] && left_val[3] == right_val[3];
        }

        inline bool operator!=(xlang_guid const& right) const noexcept
        {
            return !(*this == right);
        }
    };

    enum class XlangObjectInfoCategory
    {
        TypeName,
        HashCode,
        StringRepresentation,
        ObjectSize
    };

#ifdef __cplusplus
    enum class com_interop_result : int32_t
    {
        success = 0,
        no_interface = static_cast<int32_t>(0x80004002),
        pointer = static_cast<int32_t>(0x80004003)
    };
#else
    enum com_interop_result
    {
        com_interop_result_success = 0,
        com_interop_result_no_interface = 0x80004002,
        com_interop_result_pointer = 0x80004003
    };
#endif

    struct XLANG_NOVTABLE xlang_unknown
    {
        virtual com_interop_result XLANG_CALL QueryInterface(xlang_guid const& id, void** object) XLANG_NOEXCEPT = 0;
        virtual uint32_t XLANG_CALL AddRef() XLANG_NOEXCEPT = 0;
        virtual uint32_t XLANG_CALL Release() XLANG_NOEXCEPT = 0;
    };
    inline constexpr xlang_guid xlang_unknown_guid{ 0x00000000,0x0000,0x0000,{ 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };

    struct XLANG_NOVTABLE IXlangObject : xlang_unknown
    {
        virtual bool GetObjectInfo(XlangObjectInfoCategory info_category, void** info) = 0;
        virtual bool Equals(IXlangObject* object) = 0;
    };
    inline constexpr xlang_guid IXlangObject_guid{ 0x9f1770a3,0xd152,0x4041,{ 0x95,0x70,0x09,0x57,0xb6,0xeb,0x9e,0xf7 } };
  
    typedef XLANG_PAL_CHAR8_T xlang_char8;

    struct xlang_string__
    {
        int unused;
    };
    typedef xlang_string__* xlang_string;

    struct xlang_string_buffer__
    {
        int unused;
    };
    typedef xlang_string_buffer__* xlang_string_buffer;

    struct xlang_string_header
    {
        void* reserved1;
        char reserved2[16];
    };

#ifdef __cplusplus
    enum class xlang_string_encoding
    {
        none = 0x0,
        utf8 = 0x1,
        utf16 = 0x2
    };

#else
    enum xlang_string_encoding
    {
        XlangStringEncodingNone = 0x0,
        XlangStringEncodingUtf8 = 0x1,
        XlangStringEncodingUtf16 = 0x2
    };
#endif

#ifdef __cplusplus
    enum class xlang_result : uint32_t
    {
        success = 0,
        access_denied = 1,
        bounds = 2,
        fail = 3,
        handle = 4,
        invalid_arg = 5,
        invalid_state = 6,
        no_interface = 7,
        not_impl = 8,
        out_of_memory = 9,
        pointer = 10,
        type_load = 11
    };
#else
    enum xlang_result
    {
        xlang_result_success = 0,
        xlang_result_access_denied = 1,
        xlang_result_bounds = 2,
        xlang_result_fail = 3,
        xlang_result_handle = 4,
        xlang_result_invalid_arg = 5,
        xlang_result_invalid_state = 6,
        xlang_result_no_interface = 7,
        xlang_result_not_impl = 8,
        xlang_result_out_of_memory = 9,
        xlang_result_pointer = 10,
        xlang_result_type_load = 11
    };
#endif

    struct XLANG_NOVTABLE xlang_error_info : xlang_unknown
    {
        virtual void GetError(xlang_result* error) XLANG_NOEXCEPT = 0;
        virtual void GetMessage(xlang_string* message) XLANG_NOEXCEPT = 0;
        virtual void GetLanguageError(xlang_string* language_error) XLANG_NOEXCEPT = 0;
        virtual void GetExecutionTrace(xlang_unknown** execution_trace) XLANG_NOEXCEPT = 0;
        virtual void GetProjectionIdentifier(xlang_string* projection_identifier) XLANG_NOEXCEPT = 0;
        virtual void GetLanguageInformation(xlang_unknown** language_information) XLANG_NOEXCEPT = 0;
        virtual void GetPropagatedError(xlang_error_info** propagated_error) XLANG_NOEXCEPT = 0;
        virtual void PropagateError(
            xlang_string projection_identifier,
            xlang_string language_error,
            xlang_unknown* execution_trace,
            xlang_unknown* language_information
        ) XLANG_NOEXCEPT = 0;
    };
    inline constexpr xlang_guid xlang_error_info_guid{ 0xadf906fb, 0x11ac, 0x49ec, { 0x8d, 0xfd, 0x64, 0xc2, 0x6d, 0x8, 0x87, 0xb0 } };

    // Function declarations
    XLANG_PAL_EXPORT void* XLANG_CALL xlang_mem_alloc(size_t count) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT void XLANG_CALL xlang_mem_free(void* ptr) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_utf8(
        xlang_char8 const* source_string,
        uint32_t length,
        xlang_string* string
    ) XLANG_NOEXCEPT;
    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_utf16(
        char16_t const* source_string,
        uint32_t length,
        xlang_string* string
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_reference_utf8(
        xlang_char8 const* source_string,
        uint32_t length,
        xlang_string_header* header,
        xlang_string* string
    ) XLANG_NOEXCEPT;
    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_reference_utf16(
        char16_t const* source_string,
        uint32_t length,
        xlang_string_header* header,
        xlang_string* string
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT void XLANG_CALL xlang_delete_string(xlang_string string) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_delete_string_buffer(xlang_string_buffer buffer_handle) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_duplicate_string(
        xlang_string string,
        xlang_string* newString
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_string_encoding XLANG_CALL xlang_get_string_encoding(
        xlang_string string
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_string_raw_buffer_utf8(
        xlang_string string,
        xlang_char8 const* * buffer,
        uint32_t* length
    ) XLANG_NOEXCEPT;
    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_string_raw_buffer_utf16(
        xlang_string string,
        char16_t const* * buffer,
        uint32_t* length
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_preallocate_string_buffer_utf8(
        uint32_t length,
        xlang_char8** char_buffer,
        xlang_string_buffer* buffer_handle
    ) XLANG_NOEXCEPT;
    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_preallocate_string_buffer_utf16(
        uint32_t length,
        char16_t** char_buffer,
        xlang_string_buffer* buffer_handle
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_promote_string_buffer(
        xlang_string_buffer buffer_handle,
        xlang_string* string,
        uint32_t length
    ) XLANG_NOEXCEPT;

    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_activation_factory(
        xlang_string class_name,
        xlang_guid const& iid,
        void** factory
    ) XLANG_NOEXCEPT;

    typedef xlang_result(XLANG_CALL * xlang_pfn_lib_get_activation_factory)(xlang_string, xlang_guid const&, void **);

#ifdef __cplusplus
    [[nodiscard]] XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_originate_error(
        xlang_result error,
        xlang_string message = nullptr,
        xlang_string projection_identifier = nullptr,
        xlang_string language_error = nullptr,
        xlang_unknown* execution_trace = nullptr,
        xlang_unknown* language_information = nullptr) XLANG_NOEXCEPT;
#else
    XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_originate_error(
        xlang_result error,
        xlang_string message,
        xlang_string projection_identifier,
        xlang_string language_error,
        xlang_unknown* execution_trace,
        xlang_unknown* language_information) XLANG_NOEXCEPT;
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

constexpr xlang_string_encoding operator|(xlang_string_encoding lhs, xlang_string_encoding rhs) noexcept
{
    using int_t = std::underlying_type_t<xlang_string_encoding>;
    return static_cast<xlang_string_encoding>(static_cast<int_t>(lhs) | static_cast<int_t>(rhs));
}

constexpr xlang_string_encoding operator&(xlang_string_encoding lhs, xlang_string_encoding rhs) noexcept
{
    using int_t = std::underlying_type_t<xlang_string_encoding>;
    return static_cast<xlang_string_encoding>(static_cast<int_t>(lhs) & static_cast<int_t>(rhs));
}

constexpr xlang_string_encoding& operator|=(xlang_string_encoding& lhs, xlang_string_encoding rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr xlang_string_encoding& operator&=(xlang_string_encoding& lhs, xlang_string_encoding rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}
#endif

#endif