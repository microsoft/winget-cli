// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionProgress.h"
#include "ChannelStreams.h"
#include "Resources.h"
#include "VTSupport.h"
#include <AppInstallerProgress.h>
#include <winget/LocIndependent.h>

#include <wil/resource.h>

#include <atomic>
#include <iomanip>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>


namespace AppInstaller::CLI::Execution
{
#define WINGET_OSTREAM_FORMAT_HRESULT(hr) "0x" << Logging::SetHRFormat << hr

    // One of the options available to the users when prompting for something.
    struct BoolPromptOption
    {
        BoolPromptOption(Resource::StringId labelId, char hotkey, bool value)
            : Label(labelId), Hotkey(std::string(1, hotkey)), Value(value) {}

        Utility::LocIndString Hotkey;
        Resource::LocString Label;

        // Value associated with this option.
        bool Value;
    };

    // Reporter should be the central place to show workflow status to user.
    struct Reporter : public IProgressSink
    {
        // The channel that the reporter is targeting.
        // Based on commands/arguments, only one of these channels can be chosen.
        enum class Channel
        {
            Output,
            Completion,
            Disabled,
        };

        // The level for the Output channel.
        enum class Level : uint32_t
        {
            None = 0x0,
            Verbose = 0x1,
            Info = 0x2,
            Warning = 0x4,
            Error = 0x8,
            All = Verbose | Info | Warning | Error,
        };

        Reporter(std::ostream& outStream, std::istream& inStream);
        Reporter(const Reporter&) = delete;
        Reporter& operator=(const Reporter&) = delete;

        Reporter(Reporter&&) = default;
        Reporter& operator=(Reporter&&) = default;

        // Request that a clone be constructed from the given reporter.
        struct clone_t {};
        Reporter(const Reporter& other, clone_t);

        ~Reporter();

        // Gets the primary device attributes if available.
        std::optional<VirtualTerminal::PrimaryDeviceAttributes> GetPrimaryDeviceAttributes();

        // Get a stream for verbose output.
        OutputStream Verbose() { return GetOutputStream(Level::Verbose); }

        // Get a stream for informational output.
        OutputStream Info() { return GetOutputStream(Level::Info); }

        // Get a stream for warning output.
        OutputStream Warn() { return GetOutputStream(Level::Warning); }

        // Get a stream for error output.
        OutputStream Error() { return GetOutputStream(Level::Error); }

        // Get a stream for outputting completion words.
        OutputStream Completion() { return OutputStream(*m_out, m_channel == Channel::Completion, false); }

        // Gets a stream for output of the given level.
        OutputStream GetOutputStream(Level level);

        void EmptyLine() { GetBasicOutputStream() << std::endl; }

        // Sets the channel that will be reported to.
        // Only do this once and as soon as the channel is determined.
        void SetChannel(Channel channel);

        // Sets the visual style (mostly for progress currently)
        void SetStyle(AppInstaller::Settings::VisualStyle style);

        // Prompts the user, return true if they consented.
        bool PromptForBoolResponse(Resource::LocString message, Level level = Level::Info, bool resultIfDisabled = false);

        // Prompts the user, continues when Enter is pressed
        void PromptForEnter(Level level = Level::Info);

        // Prompts the user for a path.
        std::filesystem::path PromptForPath(Resource::LocString message, Level level = Level::Info, std::filesystem::path resultIfDisabled = std::filesystem::path::path());

        // IProgressSink
        void BeginProgress() override;
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;
        void SetProgressMessage(std::string_view message) override;
        void EndProgress(bool hideProgressWhenDone) override;

        // Contains the objects used for async progress and the lifetime of said progress.
        struct AsyncProgressScope
        {
            AsyncProgressScope(Reporter& reporter, IProgressSink* sink, bool hideProgressWhenDone);
            ~AsyncProgressScope();

            AsyncProgressScope(const AsyncProgressScope&) = delete;
            AsyncProgressScope& operator=(const AsyncProgressScope&) = delete;

            AsyncProgressScope(AsyncProgressScope&&) = delete;
            AsyncProgressScope& operator=(AsyncProgressScope&&) = delete;

            ProgressCallback& Callback();
            IProgressCallback* operator->();

            bool HideProgressWhenDone() const;
            void HideProgressWhenDone(bool value);

        private:
            std::reference_wrapper<Reporter> m_reporter;
            ProgressCallback m_callback;
            std::atomic_bool m_hideProgressWhenDone;
        };

        // Runs the given callable of type: auto(IProgressCallback&)
        template <typename F>
        auto ExecuteWithProgress(F&& f, bool hideProgressWhenDone = false)
        {
            auto progressScope = BeginAsyncProgress(hideProgressWhenDone);
            return f(progressScope->Callback());
        }

        // Begins an asynchronous progress operation.
        std::unique_ptr<AsyncProgressScope> BeginAsyncProgress(bool hideProgressWhenDone = false);

        // Sets the in progress callback.
        void SetProgressCallback(ProgressCallback* callback);

        // Cancels the in progress task.
        void CancelInProgressTask(bool force, CancelReason reason);

        void CloseOutputStream(bool forceDisable = false);

        void SetProgressSink(IProgressSink* sink)
        {
            m_progressSink = sink;
        }

        bool IsLevelEnabled(Level reporterLevel)
        {
            return WI_AreAllFlagsSet(m_enabledLevels, reporterLevel);
        }

        void SetLevelMask(Level reporterLevel, bool setEnabled = true);

        // Determines if sixels are supported by the current instance.
        bool SixelsSupported();

        // Determines if sixels are enabled; they must be both supported and enabled by user settings.
        bool SixelsEnabled();

    private:
        Reporter(std::shared_ptr<BaseStream> outStream, std::istream& inStream);
        // Gets a stream for output for internal use.
        OutputStream GetBasicOutputStream();

        // Used to show indefinite progress. Currently an indefinite spinner is the form of
        // showing indefinite progress.
        // running: shows indefinite progress if set to true, stops indefinite progress if set to false
        void ShowIndefiniteProgress(bool running);

        Channel m_channel = Channel::Output;
        std::shared_ptr<BaseStream> m_out;
        std::istream& m_in;
        std::optional<AppInstaller::Settings::VisualStyle> m_style;
        std::unique_ptr<IIndefiniteSpinner> m_spinner;
        std::unique_ptr<IProgressBar> m_progressBar;
        wil::srwlock m_progressCallbackLock;
        std::atomic<ProgressCallback*> m_progressCallback;
        std::atomic<IProgressSink*> m_progressSink;

        // Enable all levels by default
        Level m_enabledLevels = Level::All;
    };

    DEFINE_ENUM_FLAG_OPERATORS(Reporter::Level);

    // Indirection to enable change without tracking down every place
    extern const VirtualTerminal::Sequence& HelpCommandEmphasis;
    extern const VirtualTerminal::Sequence& HelpArgumentEmphasis;
    extern const VirtualTerminal::Sequence& ManifestInfoEmphasis;
    extern const VirtualTerminal::Sequence& SourceInfoEmphasis;
    extern const VirtualTerminal::Sequence& NameEmphasis;
    extern const VirtualTerminal::Sequence& IdEmphasis;
    extern const VirtualTerminal::Sequence& UrlEmphasis;
    extern const VirtualTerminal::Sequence& PromptEmphasis;
    extern const VirtualTerminal::Sequence& ConvertToUpgradeFlowEmphasis;
    extern const VirtualTerminal::Sequence& ConfigurationIntentEmphasis;
    extern const VirtualTerminal::Sequence& ConfigurationUnitEmphasis;
    extern const VirtualTerminal::Sequence& AuthenticationEmphasis;
}
