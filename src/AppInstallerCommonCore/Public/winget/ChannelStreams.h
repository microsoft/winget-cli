#pragma once
#include "Resources.h"

namespace AppInstaller
{
    namespace Execution::details
    {
        // List of approved types for output, others are potentially not localized.
        template <typename T>
        struct IsApprovedForOutput : std::false_type {};

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(_t_) template <> struct IsApprovedForOutput<_t_> : std::true_type {};
#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_TYPE(_t_) " '" #_t_ "'"

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(F) \
        /* It is assumed that single char values need not be localized, as they are matched
           ordinally or they are punctuation / other. */ \
        F(char) \
        /* Localized strings (and from an Id for one for convenience).*/ \
        F(AppInstaller::StringResource::StringId) \
        F(AppInstaller::Resource::LocString) \
        /* Strings explicitly declared as localization independent.*/ \
        F(AppInstaller::Utility::LocIndView) \
        F(AppInstaller::Utility::LocIndString) \
        /* Normalized strings come from user dataand should therefore already by localized
           by how they are chosen (or there is no localized version).*/ \
        F(AppInstaller::Utility::NormalizedString)

#define WINGET_ISAPPROVEDFOROUTPUT_ERROR  \
        "Only the following types are approved for output:" \
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_TYPE)

        WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION)
    }

    template<typename ... T>
    Utility::LocIndString StringResource::StringId::operator()(T ... args) const
    {
        static_assert((Execution::details::IsApprovedForOutput<T>::value && ...), WINGET_ISAPPROVEDFOROUTPUT_ERROR);
        return Utility::LocIndString{ Utility::Format(Resolve(), std::forward<T>(args)...) };
    }
}
