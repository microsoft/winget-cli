#pragma once
#include "Resources.h"

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(_t_) template <> struct IsApprovedForOutput<_t_> : std::true_type {};
#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_STRING_TYPE(_t_) " '" #_t_ "'"

#define WINGET_APPROVED_FOR_OUTPUT_LIST(_f_) \
        /* It is assumed that single char values need not be localized, as they are matched
           ordinally or they are punctuation / other. */ \
        _f_(char) \
        /* Localized strings (and from an Id for one for convenience).*/ \
        _f_(AppInstaller::StringResource::StringId) \
        _f_(AppInstaller::Resource::LocString) \
        /* Strings explicitly declared as localization independent.*/ \
        _f_(AppInstaller::Utility::LocIndView) \
        _f_(AppInstaller::Utility::LocIndString) \
        /* Normalized strings come from user dataand should therefore already by localized
           by how they are chosen (or there is no localized version).*/ \
        _f_(AppInstaller::Utility::NormalizedString)

#define WINGET_ISAPPROVEDFOROUTPUT_ERROR_MESSAGE  \
        "Only the following types are approved for output:" \
        WINGET_APPROVED_FOR_OUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_STRING_TYPE)

namespace AppInstaller
{
    namespace Execution::details
    {
        // List of approved types for output, others are potentially not localized.
        template <typename T>
        struct IsApprovedForOutput : std::false_type {};
        WINGET_APPROVED_FOR_OUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION);
    }

    template<typename ... T>
    Utility::LocIndString StringResource::StringId::operator()(T ... args) const
    {
        static_assert((Execution::details::IsApprovedForOutput<T>::value && ...), WINGET_ISAPPROVEDFOROUTPUT_ERROR_MESSAGE);
        return Utility::LocIndString{ Utility::Format(Resolve(), std::forward<T>(args)...) };
    }
}
