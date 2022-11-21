
namespace xlang::impl
{
#ifdef __IUnknown_INTERFACE_DEFINED__
#define XLANG_WINDOWS_ABI
    using ref_count_type = unsigned long;
#else
    using ref_count_type = uint32_t;
#endif

    using ptp_io = struct tp_io*;
    using ptp_timer = struct tp_timer*;
    using ptp_wait = struct tp_wait*;
    using bstr = wchar_t*;
}

namespace xlang
{
	using guid = xlang_guid;
}

namespace xlang::Windows::Foundation
{
    enum class TrustLevel : int32_t
    {
        BaseTrust,
        PartialTrust,
        FullTrust
    };

    struct IUnknown;
    struct IXlangObject;
    struct IActivationFactory;
}
