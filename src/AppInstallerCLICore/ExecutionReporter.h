// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionProgress.h"
#include "Resources.h"
#include "VTSupport.h"
#include <AppInstallerProgress.h>
#include <winget/LocIndependent.h>

#include <wil/resource.h>

#include <atomic>
#include <iomanip>
#include <istream>
#include <ostream>
#include <string>


namespace AppInstaller::CLI::Execution
{
#define WINGET_OSTREAM_FORMAT_HRESULT(hr) "0x" << std::hex << std::setw(8) << std::setfill('0') << hr

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

    // Reporter should be the central place to show workflow status to user.
    // Todo: need to implement actual console output to show progress bar, etc
    struct Reporter : public IProgressSink
    {
        enum class Level
        {
            Verbose,
            Info,
            Warning,
            Error,
        };

        Reporter(std::ostream& outStream, std::istream& inStream);

        ~Reporter();

        // Holds output formatting information.
        struct OutputStream
        {
            OutputStream(std::ostream& out, bool enableVT);

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
                ApplyFormat();
                m_out << t;
                return *this;
            }

            OutputStream& operator<<(std::ostream& (__cdecl* f)(std::ostream&));
            OutputStream& operator<<(const VirtualTerminal::Sequence& sequence);

        private:
            // Applies the format for the stream.
            void ApplyFormat();

            std::ostream& m_out;
            bool m_isVTEnabled;
            size_t m_applyFormatAtOne = 1;
            std::string m_format;
        };

        // Get a stream for verbose output.
        OutputStream Verbose() { return GetOutputStream(Level::Verbose); }

        // Get a stream for informational output.
        OutputStream Info() { return GetOutputStream(Level::Info); }

        // Get a stream for warning output.
        OutputStream Warn() { return GetOutputStream(Level::Warning); }

        // Get a stream for error output.
        OutputStream Error() { return GetOutputStream(Level::Error); }

        // Gets a stream for output of the given level.
        OutputStream GetOutputStream(Level level);

        void EmptyLine() { m_out << std::endl; }

        // Sets the visual style (mostly for progress currently)
        void SetStyle(AppInstaller::Settings::VisualStyle style);

        // Used to show indefinite progress. Currently an indefinite spinner is the form of
        // showing indefinite progress.
        // running: shows indefinite progress if set to true, stops indefinite progress if set to false
        void ShowIndefiniteProgress(bool running);

        // IProgressSink
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;

        // Runs the given callable of type: auto(IProgressCallback&)
        template <typename F>
        auto ExecuteWithProgress(F&& f, bool hideProgressWhenDone = false)
        {
            if (m_consoleMode.IsVTEnabled())
            {
                m_out << VirtualTerminal::Cursor::Visibility::DisableShow;
            }

            ProgressCallback callback(this);
            SetProgressCallback(&callback);
            ShowIndefiniteProgress(true);

            auto hideProgress = wil::scope_exit([this, hideProgressWhenDone]()
                {
                    SetProgressCallback(nullptr);
                    ShowIndefiniteProgress(false);
                    m_progressBar.EndProgress(hideProgressWhenDone);

                    if (m_consoleMode.IsVTEnabled())
                    {
                        m_out << VirtualTerminal::Cursor::Visibility::EnableShow;
                    }
                });
            return f(callback);
        }

        // Sets the in progress callback.
        void SetProgressCallback(ProgressCallback* callback);

        // Cancels the in progress task.
        void CancelInProgressTask(bool force);

    private:
        std::ostream& m_out;
        std::istream& m_in;
        VirtualTerminal::ConsoleModeRestore m_consoleMode;
        IndefiniteSpinner m_spinner;
        ProgressBar m_progressBar;
        wil::srwlock m_progressCallbackLock;
        std::atomic<ProgressCallback*> m_progressCallback;
    };

    // Indirection to enable change without tracking down every place
    extern const VirtualTerminal::Sequence& HelpCommandEmphasis;
    extern const VirtualTerminal::Sequence& HelpArgumentEmphasis;
    extern const VirtualTerminal::Sequence& NameEmphasis;
    extern const VirtualTerminal::Sequence& IdEmphasis;
    extern const VirtualTerminal::Sequence& UrlEmphasis;
}
