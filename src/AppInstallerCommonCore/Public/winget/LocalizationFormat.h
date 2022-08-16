// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Resources.h"

namespace AppInstaller::StringResource
{
    namespace details
    {
        // List of approved types for output, others are potentially not localized.
        template <typename T>
        struct IsApprovedForOutput
        {
            static constexpr bool value = false;
        };

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(_t_) \
        template <> \
        struct IsApprovedForOutput<_t_> \
        { \
            static constexpr bool value = true; \
        }

        // It is assumed that single char values need not be localized, as they are matched
        // ordinally or they are punctuation / other.
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(char);
        // Localized strings (and from an Id for one for convenience).
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(StringId);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Resource::LocString);
        // Strings explicitly declared as localization independent.
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndView);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndString);
        // Normalized strings come from user data and should therefore already by localized
        // by how they are chosen (or there is no localized version).
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::NormalizedString);
    }

    template<typename ... T>
    Utility::LocIndString StringId::operator()(T ... args) const
    {
        static_assert((details::IsApprovedForOutput<std::decay_t<T>>::value && ...), "This type may not be localized, see comment for more information");
        return Utility::LocIndString{ Utility::Format(Resolve(), std::forward<T>(args)...) };
    }
}
