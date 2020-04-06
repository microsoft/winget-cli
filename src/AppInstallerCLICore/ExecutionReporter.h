// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "VTSupport.h"
#include <AppInstallerProgress.h>

#include <wil/resource.h>

#include <atomic>
#include <future>
#include <istream>
#include <ostream>
#include <string>
#include <vector>


namespace AppInstaller::CLI::Execution
{
    namespace details
    {
        // Class to print a indefinite spinner.
        class IndefiniteSpinner
        {
        public:
            IndefiniteSpinner(std::ostream& stream) : m_out(stream) {}

            void ShowSpinner();
            void StopSpinner();

        private:
            std::atomic<bool> m_canceled = false;
            std::atomic<bool> m_spinnerRunning = false;
            std::future<void> m_spinnerJob;
            std::ostream& m_out;

            void ShowSpinnerInternal();
        };

        // Todo: Need to implement real progress bar. Only prints progress number now.
        class ProgressBar
        {
        public:
            ProgressBar(std::ostream& stream) : m_out(stream) {}

            void ShowProgress(bool running, uint64_t progress);

        private:
            std::atomic<bool> m_isVisible = false;
            std::ostream& m_out;
        };
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

        Reporter(std::ostream& outStream, std::istream& inStream) :
            m_out(outStream), m_in(inStream), m_progressBar(outStream), m_spinner(outStream) {}

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

        bool PromptForBoolResponse(const std::string& msg, Level level = Level::Info);

        // Used to show definite progress.
        // running: shows progress bar if set to true, dismisses progress bar if set to false
        void ShowProgress(bool running, uint64_t progress);

        // Used to show indefinite progress. Currently an indefinite spinner is the form of
        // showing indefinite progress.
        // running: shows indefinite progress if set to true, stops indefinite progress if set to false
        void ShowIndefiniteProgress(bool running);

        // IProgressSink
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;

        // Runs the given callable of type: auto(IProgressCallback&)
        template <typename F>
        auto ExecuteWithProgress(F&& f)
        {
            ProgressCallback callback(this);
            SetProgressCallback(&callback);
            ShowIndefiniteProgress(true);

            auto hideProgress = wil::scope_exit([this]()
                {
                    SetProgressCallback(nullptr);
                    ShowIndefiniteProgress(false);
                    ShowProgress(false, 0);
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
        details::IndefiniteSpinner m_spinner;
        details::ProgressBar m_progressBar;
        wil::srwlock m_progressCallbackLock;
        std::atomic<ProgressCallback*> m_progressCallback;
    };

    // Indirection to enable change without tracking down every place
    extern VirtualTerminal::Sequence HelpCommandEmphasis;
    extern VirtualTerminal::Sequence HelpArgumentEmphasis;
}
