
#if defined(_WIN32) && defined(_DEBUG)
#define XLANG_NATVIS
#endif

#ifdef XLANG_NATVIS

namespace xlang::impl
{
    struct natvis
    {
        static auto XLANG_CALL abi_val(void* object, wchar_t const * iid_str, int method)
        {
            union variant
            {
                bool b;
                wchar_t c;
                int8_t i1;
                int16_t i2;
                int32_t i4;
                int64_t i8;
                uint8_t u1;
                uint16_t u2;
                uint32_t u4;
                uint64_t u8;
                float r4;
                double r8;
                guid g;
                void* s;
                uint8_t v[1024];
            }
            value{};
            guid iid{};
            if (XLANG_IIDFromString(iid_str, &iid) == 0)
            {
                xlang_object_abi* pinsp;
                typedef int32_t(XLANG_CALL xlang_object_abi::* PropertyAccessor)(void*);
                if (((unknown_abi*)object)->QueryInterface(iid, reinterpret_cast<void**>(&pinsp)) == com_interop_result::success)
                {
                    auto vtbl = *(PropertyAccessor**)pinsp;
                    static const int IXlangObject_vtbl_size = 5;
                    auto get_Property = vtbl[method + IXlangObject_vtbl_size];
                    (pinsp->*get_Property)(&value);
                    pinsp->Release();
                }
            }
            return value;
        }

        static auto XLANG_CALL get_val(xlang::Windows::Foundation::IXlangObject* object, wchar_t const * iid_str, int method)
        {
            return abi_val(static_cast<unknown_abi*>(get_abi(*object)), iid_str, method);
        }
    };
}

extern "C"
__declspec(selectany) decltype(xlang::impl::natvis::abi_val) & XLANG_abi_val = xlang::impl::natvis::abi_val;

extern "C"
__declspec(selectany) decltype(xlang::impl::natvis::get_val) & XLANG_get_val = xlang::impl::natvis::get_val;

#ifdef _M_IX86
#pragma comment(linker, "/include:_XLANG_abi_val")
#pragma comment(linker, "/include:_XLANG_get_val")
#else
#pragma comment(linker, "/include:XLANG_abi_val")
#pragma comment(linker, "/include:XLANG_get_val")
#endif

#endif
