#pragma once

#include "AppInstallerDownloader.h"

namespace AppInstaller::Workflow
{
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

    class IndefiniteSpinner
    {
    public:
        IndefiniteSpinner(std::ostream& stream) : out(stream) {};

        void ShowSpinner();
        void StopSpinner();

    private:
        bool m_canceled = false;
        std::ostream& out;
        std::future<void> m_spinnerJob;

        void ShowSpinnerInternal();
    };

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
        void ShowIndefiniteSpinner(bool running);

        DownloaderCallback& GetDownloaderCallback() { return m_downloaderCallback; }

    private:
        std::ostream& out;
        DownloaderCallback m_downloaderCallback;
        IndefiniteSpinner m_spinner;
    };
}