// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "pch.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow
{
    class InstallerHandlerBase
    {
    public:
        virtual void Download();
        virtual void Install() { THROW_HR(E_NOTIMPL); }
        virtual void Cancel() { THROW_HR(E_NOTIMPL); }

    protected:

        // This will be triggered by file downloader to get download progress
        class DownloaderCallback : public AppInstaller::Utility::IDownloaderCallback
        {
        public:
            DownloaderCallback(WorkflowReporter& reporter) : m_reporterRef(reporter) {};

            void OnStarted() override;
            void OnProgress(LONGLONG progress, LONGLONG downloadSize) override;
            void OnCanceled() override;
            void OnCompleted() override;

        private:
            WorkflowReporter& m_reporterRef;
        };

        InstallerHandlerBase(const Manifest::ManifestInstaller& manifestInstaller, WorkflowReporter& reporter) :
            m_manifestInstallerRef(manifestInstaller), m_reporterRef(reporter), m_downloaderCallback(reporter) {};

        const Manifest::ManifestInstaller& m_manifestInstallerRef;
        WorkflowReporter& m_reporterRef;
        std::filesystem::path m_downloadedInstaller;
        DownloaderCallback m_downloaderCallback;
    };
}

