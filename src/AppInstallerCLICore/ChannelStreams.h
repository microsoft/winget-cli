// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Resources.h"
#include "VTSupport.h"
#include <winget/LocIndependent.h>

#include <Windows.h>

#include <memory>
#include <optional>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>


namespace AppInstaller::CLI::Execution
{
    // Gets the current console width, or std::nullopt if stdout is not attached to a console
    // (e.g. redirected to a file or pipe). Callers that receive nullopt should not truncate output.
    std::optional<size_t> GetConsoleWidth();

    // Gets the type of the given standard handle, or FILE_TYPE_UNKNOWN if the handle is not valid.
    DWORD GetStandardHandleType(DWORD standardHandle);

    // Determines whether redirected human readable output has to be written in the given code page.
    //
    // A console host decodes the output of a native command with the code page that was active
    // before the command started (PowerShell, for example, decodes native command output with the
    // console output code page), so human readable output that such a host consumes has to be
    // written in that code page. Output that is redirected to a file is not decoded by anyone, and
    // machine readable channels stay UTF-8 for the consumers that parse them.
    bool ShouldTranscodeRedirectedOutput(DWORD standardOutputStreamType, UINT consumerCodePage);

    // A stream buffer that converts the UTF-8 that the CLI writes into the given code page.
    //
    // A UTF-8 sequence that is split across writes is held until the rest of it arrives. While the
    // buffer is bypassed, output is forwarded unchanged so that machine readable channels stay
    // UTF-8 for the consumers that parse them.
    struct Utf8ToCodePageStreamBuf : public std::streambuf
    {
        Utf8ToCodePageStreamBuf(std::ostream& out, UINT codePage);
        ~Utf8ToCodePageStreamBuf() override;

        // Writes any held output and restores the stream's original buffer.
        void Restore();

        // Sets whether output is forwarded unchanged instead of being converted.
        void SetBypass(bool bypass);

        Utf8ToCodePageStreamBuf(const Utf8ToCodePageStreamBuf&) = delete;
        Utf8ToCodePageStreamBuf& operator=(const Utf8ToCodePageStreamBuf&) = delete;

    protected:
        int_type overflow(int_type value) override;
        std::streamsize xsputn(const char* data, std::streamsize count) override;
        int sync() override;

    private:
        // Writes the output that is ready to be converted; an incomplete UTF-8 sequence is held.
        // Returns false if the target could not be written, leaving the output held.
        bool Flush();

        // Writes all held output, converting an incomplete sequence with a replacement character.
        bool FlushAll();

        // Returns false if the target could not be written, leaving the output held.
        bool Put(const char* data, size_t count);

        // Writes every byte to the target and returns false if the target refused any of them.
        bool Write(const char* data, size_t count);

        bool Convert(std::string_view value);

        std::ostream& m_out;
        std::streambuf* m_target = nullptr;
        UINT m_codePage = 0;
        std::string m_pending;
        std::wstring m_wide;
        std::string m_bytes;
        bool m_bypass = false;
    };

    // Applies the code page that a consumer of redirected output uses to a stream's human readable
    // output, and lets machine readable channels opt out.
    //
    // The conversion is installed when the object is constructed and removed when it is destroyed,
    // so the owner has to outlive all of the output that the conversion applies to. Exactly one
    // instance should own the conversion for a given stream.
    class RedirectedOutputEncoding
    {
    public:
        // standardOutputStreamType is the GetFileType value for stdout, and consumerCodePage is the
        // code page that decodes redirected output. The code page has to be captured before the
        // console is switched to UTF-8, because that is the value that the consumer is using.
        RedirectedOutputEncoding(std::ostream& out, DWORD standardOutputStreamType, UINT consumerCodePage);
        ~RedirectedOutputEncoding();

        RedirectedOutputEncoding(const RedirectedOutputEncoding&) = delete;
        RedirectedOutputEncoding& operator=(const RedirectedOutputEncoding&) = delete;

        // Sets whether the output that follows is machine readable and must stay UTF-8.
        void SetMachineReadable(bool machineReadable);

        // Whether output is currently converted to the consumer's code page.
        bool IsConverting() const;

    private:
        std::unique_ptr<Utf8ToCodePageStreamBuf> m_conversion;
        bool m_machineReadable = false;
    };

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

        std::ostream& Get();

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

        // Clears the current format value.
        void ClearFormat();

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
            // TODO: This assertion is currently only applied to placeholders in localized strings.
            //       Convert the rest of the code base and uncomment to enforce localization.
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

        inline bool IsEnabled() {
            return m_enabled;
        }

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
