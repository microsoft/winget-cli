
namespace xlang::impl
{
    template <> struct abi<Windows::Foundation::IUnknown>
    {
        using type = ::xlang_unknown;
    };
    template <> struct guid_storage<Windows::Foundation::IUnknown>
    {
        static constexpr guid value{ 0x00000000,0x0000,0x0000,{ 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };
    };
    using unknown_abi = abi_t<Windows::Foundation::IUnknown>;

    template <> struct abi<Windows::Foundation::IXlangObject>
    {
        using type = ::IXlangObject;
    };
    template <> struct guid_storage<Windows::Foundation::IXlangObject>
    {
        static constexpr guid value{ ::IXlangObject_guid };
    };
    template <> struct name<Windows::Foundation::IXlangObject>
    {
        static constexpr auto & value{ u8"Object" };
        static constexpr auto & data{ u8"cinterface(IXlangObject)" };
    };
    template <> struct category<Windows::Foundation::IXlangObject>
    {
        using type = basic_category;
    };
    using xlang_object_abi = abi_t<Windows::Foundation::IXlangObject>;

    struct XLANG_NOVTABLE IAgileObject : unknown_abi
    {
    };
    template <> struct guid_storage<IAgileObject>
    {
        static constexpr guid value{ 0x94EA2B94,0xE9CC,0x49E0,{ 0xC0,0xFF,0xEE,0x64,0xCA,0x8F,0x5B,0x90 } };
    };

    struct XLANG_NOVTABLE IWeakReference : unknown_abi
    {
        virtual com_interop_result XLANG_CALL Resolve(guid const& iid, void** objectReference) noexcept = 0;
    };
    template <> struct guid_storage<IWeakReference>
    {
        static constexpr guid value{ 0x00000037,0x0000,0x0000,{ 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };
    };

    struct XLANG_NOVTABLE IWeakReferenceSource : unknown_abi
    {
        virtual com_interop_result XLANG_CALL GetWeakReference(IWeakReference** weakReference) noexcept = 0;
    };
    template <> struct guid_storage<IWeakReferenceSource>
    {
        static constexpr guid value{ 0x00000038,0x0000,0x0000,{ 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };
    };

}
