// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Resources.h"
#include "VTSupport.h"
#include <winget/LocIndependent.h>

#include <ostream>
#include <string>


namespace AppInstaller::CLI::Execution
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
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Resource::StringId);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Resource::LocString);
        // Strings explicitly declared as localization independent.
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndView);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndString);
        // Normalized strings come from user data and should therefore already by localized
        // by how they are chosen (or there is no localized version).
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::NormalizedString);
    }

    // The base stream for all channels.
    struct BaseStream
    {
        BaseStream(std::ostream& out, bool enabled, bool VTEnabled);

        template <typename T>
        BaseStream& operator<<(const T& t)
        {
            Write<T>(t, false);
            return *this;
        }

        BaseStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
        BaseStream& operator<<(const VirtualTerminal::Sequence& sequence);
        BaseStream& operator<<(const VirtualTerminal::ConstructedSequence& sequence);

        void SetVTEnabled(bool enabled);

        void RestoreDefault();

        void Disable();

    private:
        template <typename T>
        void Write(const T& t, bool bypass)
        {
            if (bypass || m_enabled)
            {
                m_out << t;
            }
        };

        std::ostream& m_out;
        std::atomic_bool m_enabled;
        std::atomic_bool m_VTUpdated;
        bool m_VTEnabled;
    };

    // Holds output formatting information.
    struct OutputStream
    {
        OutputStream(BaseStream& out, bool enabled, bool VTEnabled = true);

        // Adds a format to the current value.
        void AddFormat(const VirtualTerminal::Sequence& sequence);

        template <typename T>
        OutputStream& operator<<(const T& t)
        {
            // You've found your way here because you tried to output a type that may not localized.
            // In order to ensure that all output is localized, only the types with specializations of
            // details::IsApprovedForOutput above can be output.
            // * If your string is a simple message, it should be put in
            //      /src/AppInstallerCLIPackage/Shared/Strings/en-us/winget.resw
            //      and referenced in /src/AppInstallerCLICore/Resources.h. Then either output the
            //      Resource::StringId, or load it manually and output the Resource::LocString.
            // * If your string is *definitely* localization independent, you can tag it as such with
            //      the Utility::LocInd(View/String) types.
            // * If your string came from outside of the source code, it is best to store it in a
            //      Utility::NormalizedString so that it has a normalized representation. This also
            //      informs the output that there is no localized version to use.
            // TODO: Convert the rest of the code base and uncomment to enforce localization.
            //static_assert(details::IsApprovedForOutput<std::decay_t<T>>::value, "This type may not be localized, see comment for more information");
            if (m_enabled)
            {
                if (m_VTEnabled)
                {
                    ApplyFormat();
                }
                m_out << t;
            }
            return *this;
        }

        OutputStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
        OutputStream& operator<<(const VirtualTerminal::Sequence& sequence);
        OutputStream& operator<<(const VirtualTerminal::ConstructedSequence& sequence);
        OutputStream& operator<<(const std::filesystem::path& path);

    private:
        // Applies the format for the stream.
        void ApplyFormat();

        BaseStream& m_out;
        bool m_enabled;
        bool m_VTEnabled;
        size_t m_applyFormatAtOne = 1;
        VirtualTerminal::ConstructedSequence m_format;
    };
}
