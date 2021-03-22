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
#include <optional>
#include <ostream>
#include <string>


namespace AppInstaller::CLI::Execution
{
#define WINGET_OSTREAM_FORMAT_HRESULT(hr) "0x" << Logging::SetHRFormat << hr

    // Reporter should be the central place to show workflow status to user.
    struct Reporter : public IProgressSink
    {
        // The channel that the reporter is targeting.
        // Based on commands/arguments, only one of these channels can be chosen.
        enum class Channel
        {
            Output,
            Completion,
        };

        // The level for the Output channel.
        enum class Level
        {
            Verbose,
            Info,
            Warning,
            Error,
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

        // Get a stream for verbose output.
        OutputStream Verbose() { return GetOutputStream(Level::Verbose); }

        // Get a stream for informational output.
        OutputStream Info() { return GetOutputStream(Level::Info); }

        // Get a stream for warning output.
        OutputStream Warn() { return GetOutputStream(Level::Warning); }

        // Get a stream for error output.
        OutputStream Error() { return GetOutputStream(Level::Error); }

        // Get a stream for outputting completion words.
        NoVTStream Completion() { return NoVTStream(m_out, m_channel == Channel::Completion); }

        // Gets a stream for output of the given level.
        OutputStream GetOutputStream(Level level);

        void EmptyLine() { GetBasicOutputStream() << std::endl; }

        // Sets the channel that will be reported to.
        // Only do this once and as soon as the channel is determined.
        void SetChannel(Channel channel);

        // Sets the visual style (mostly for progress currently)
        void SetStyle(AppInstaller::Settings::VisualStyle style);

        // Used to show indefinite progress. Currently an indefinite spinner is the form of
        // showing indefinite progress.
        // running: shows indefinite progress if set to true, stops indefinite progress if set to false
        void ShowIndefiniteProgress(bool running);

        // IProgressSink
        void BeginProgress() override;
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;
        void EndProgress(bool hideProgressWhenDone) override;

        // Runs the given callable of type: auto(IProgressCallback&)
        template <typename F>
        auto ExecuteWithProgress(F&& f, bool hideProgressWhenDone = false)
        {
            IProgressSink* sink = m_progressSink.load();
            ProgressCallback callback(sink);
            SetProgressCallback(&callback);
            sink->BeginProgress();

            auto hideProgress = wil::scope_exit([this, hideProgressWhenDone]()
                {
                    SetProgressCallback(nullptr);
                    m_progressSink.load()->EndProgress(hideProgressWhenDone);
                });
            return f(callback);
        }

        // Sets the in progress callback.
        void SetProgressCallback(ProgressCallback* callback);

        // Cancels the in progress task.
        void CancelInProgressTask(bool force);

        void SetProgressSink(IProgressSink* sink)
        {
            m_progressSink = sink;
        }

    private:
        // Gets whether VT is enabled for this reporter.
        bool IsVTEnabled() const;

        // Gets a stream for output for internal use.
        OutputStream GetBasicOutputStream();

        Channel m_channel = Channel::Output;
        std::ostream& m_out;
        std::istream& m_in;
        bool m_isVTEnabled = true;
        std::optional<AppInstaller::Settings::VisualStyle> m_style;
        std::optional<IndefiniteSpinner> m_spinner;
        std::optional<ProgressBar> m_progressBar;
        wil::srwlock m_progressCallbackLock;
        std::atomic<ProgressCallback*> m_progressCallback;
        std::atomic<IProgressSink*> m_progressSink;
    };

    // Indirection to enable change without tracking down every place
    extern const VirtualTerminal::Sequence& HelpCommandEmphasis;
    extern const VirtualTerminal::Sequence& HelpArgumentEmphasis;
    extern const VirtualTerminal::Sequence& NameEmphasis;
    extern const VirtualTerminal::Sequence& IdEmphasis;
    extern const VirtualTerminal::Sequence& UrlEmphasis;
}
