// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "Public/AppInstallerRepositorySearch.h"

namespace AppInstaller::Workflow
{
    // Class to print a indefinite spinner.
    class IndefiniteSpinner
    {
    public:
        IndefiniteSpinner(std::ostream& stream) : out(stream) {};

        void ShowSpinner();
        void StopSpinner();

    private:
        std::atomic<bool> m_canceled = false;
        std::atomic<bool> m_spinnerRunning = false;
        std::future<void> m_spinnerJob;
        std::ostream& out;

        void ShowSpinnerInternal();
    };

    // Todo: Need to implement real progress bar. Only prints progress number now.
    class ProgressBar
    {
    public:
        ProgressBar(std::ostream& stream) : out(stream) {};

        void ShowProgress(bool running, int progress);

    private:
        std::atomic<bool> m_isVisible = false;
        std::ostream& out;
    };

    // WorkflowReporter should be the central place to show workflow status to user.
    // Todo: need to implement actual console output to show color, progress bar, etc
    class WorkflowReporter
    {
    public:

        enum class Level
        {
            Verbose,
            Info,
            Warning,
            Error,
        };

        WorkflowReporter(std::ostream& outStream, std::istream& inStream) :
            out(outStream), in(inStream), m_progressBar(outStream), m_spinner(outStream) {};

        void ShowAppInfo(
            const AppInstaller::Manifest::Manifest& manifest,
            const AppInstaller::Manifest::ManifestLocalization& selectedLocalization,
            const AppInstaller::Manifest::ManifestInstaller& selectedInstaller);

        void ShowSearchResult(
            const AppInstaller::Repository::SearchResult& result);

        bool PromptForBoolResponse(Level level, const std::string& msg);

        void ShowMsg(Level level, const std::string& msg);

        // Used to show definite progress.
        // running: shows progress bar if set to true, dismisses progress bar if set to false
        void ShowProgress(bool running, int progress);

        // Used to show indefinite progress. Currently an indefinite spinner is the form of
        // showing indefinite progress.
        // running: shows indefinite progress if set to true, stops indefinite progress if set to false
        void ShowIndefiniteProgress(bool running);

    private:
        std::ostream& out;
        std::istream& in;
        IndefiniteSpinner m_spinner;
        ProgressBar m_progressBar;
    };
}