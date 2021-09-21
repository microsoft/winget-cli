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
    struct BaseOutputStream
    {
        BaseOutputStream(std::ostream& out, bool enabled, bool VTEnabled) :
            BaseOutputStream(out, enabled, VTEnabled, VirtualTerminal::TextFormat::Default) {};

        BaseOutputStream(std::ostream& out, bool enabled, bool VTEnabled, VirtualTerminal::Sequence sequence) ;

        template <typename T>
        BaseOutputStream& operator<<(const T& t)
        {
            Write<T>(t, false);
            return *this;
        }

        BaseOutputStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
        BaseOutputStream& operator<<(const VirtualTerminal::Sequence& sequence);
        BaseOutputStream& operator<<(const VirtualTerminal::ConstructedSequence& sequence);

        void Disable() { m_enabled = false; }

        void Enable() { m_enabled = true; }

        void Close();

    private:
        template <typename T>
        void Write(const T&t, bool bypassIfNotEnabled) 
        {
            if (m_enabled || bypassIfNotEnabled)
            {
                m_out << t;
            }
        };

        VirtualTerminal::Sequence m_defaultSequence;
        bool m_enabled;
        bool m_VTEnabled;
        std::ostream& m_out;
    };

    // Holds output formatting information.
    struct VTOutputStream : BaseOutputStream
    {
        VTOutputStream(std::ostream& out, bool enabled = false);

        // Adds a format to the current value.
        void AddFormat(const VirtualTerminal::Sequence& sequence);

        template <typename T>
        VTOutputStream& operator<<(const T& t)
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
            ApplyFormat();
            BaseOutputStream::operator<<(t);
            return *this;
        }

        VTOutputStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
        VTOutputStream& operator<<(const VirtualTerminal::Sequence& sequence);
        VTOutputStream& operator<<(const VirtualTerminal::ConstructedSequence& sequence);
        VTOutputStream& operator<<(const std::filesystem::path& path);

    private:
        // Applies the format for the stream.
        void ApplyFormat();

        size_t m_applyFormatAtOne = 1;
        VirtualTerminal::ConstructedSequence m_format;
    };

    // Does not allow VT at all.
    struct NoVTOutputStream : BaseOutputStream
    {
        NoVTOutputStream(std::ostream& out, bool enabled = false);

        template <typename T>
        NoVTOutputStream& operator<<(const T& t)
        {
            BaseOutputStream::operator<<(t);
            return *this;
        }

        NoVTOutputStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
        NoVTOutputStream& operator<<(const VirtualTerminal::Sequence& sequence) = delete;
        NoVTOutputStream& operator<<(const VirtualTerminal::ConstructedSequence& sequence) = delete;
    };
}
