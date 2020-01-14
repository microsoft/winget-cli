// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "AppInstallerDownloader.h"

namespace AppInstaller::Workflow
{
    // This will be triggered by file downloader to get download progress
    class DownloaderCallback : public AppInstaller::Utility::IDownloaderCallback
    {
    public:
        DownloaderCallback(std::ostream& stream) : out(stream) {};

        void OnStarted() override;
        void OnProgress(LONGLONG progress, LONGLONG downloadSize) override;
        void OnCanceled() override;
        void OnCompleted() override;

    private:
        std::ostream& out;
    };

    // Class to print the in progress spinner
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

    // WorkflowReporter should be the central place to show workflow status to user.
    // Todo: need to implement actual console output to show color, progress bar, etc
    class WorkflowReporter
    {
    public:
        WorkflowReporter(std::ostream& stream) :
            out(stream), m_downloaderCallback(stream), m_spinner(stream) {};

        void ShowPackageInfo(
            const std::string& name,
            const std::string& version,
            const std::string& author,
            const std::string& description,
            const std::string& homepage,
            const std::string& licenceUrl);

        void ShowMsg(const std::string& msg);

        // running: shows the spinner if set to true, stops the spinner if set to false
        void ShowIndefiniteSpinner(bool running);

        DownloaderCallback& GetDownloaderCallback() { return m_downloaderCallback; }

    private:
        std::ostream& out;
        DownloaderCallback m_downloaderCallback;
        IndefiniteSpinner m_spinner;
    };
}