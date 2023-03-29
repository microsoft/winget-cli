// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "VTSupport.h"
#include <AppInstallerProgress.h>
#include <winget/UserSettings.h>
#include <ChannelStreams.h>

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
        // Shared functionality for progress visualizers.
        struct ProgressVisualizerBase
        {
            ProgressVisualizerBase(BaseStream& stream, bool enableVT) :
                m_out(stream), m_enableVT(enableVT) {}

            void SetStyle(AppInstaller::Settings::VisualStyle style) { m_style = style; }

            void SetMessage(std::string_view message);

        protected:
            BaseStream& m_out;
            Settings::VisualStyle m_style = AppInstaller::Settings::VisualStyle::Accent;
            std::string m_message;

            bool UseVT() const { return m_enableVT && m_style != AppInstaller::Settings::VisualStyle::NoVT; }

            // Applies the selected visual style.
            void ApplyStyle(size_t i, size_t max, bool foregroundOnly);

            void ClearLine();

        private:
            bool m_enableVT = false;
        };
    }

    // Displays an indefinite spinner.
    struct IndefiniteSpinner : public details::ProgressVisualizerBase
    {
        IndefiniteSpinner(BaseStream& stream, bool enableVT) :
            details::ProgressVisualizerBase(stream, enableVT) {}

        void ShowSpinner();

        void StopSpinner();

    private:
        std::atomic<bool> m_canceled = false;
        std::atomic<bool> m_spinnerRunning = false;
        std::future<void> m_spinnerJob;

        void ShowSpinnerInternal();
    };

    // Displays progress 
    class ProgressBar : public details::ProgressVisualizerBase
    {
    public:
        ProgressBar(BaseStream& stream, bool enableVT) :
            details::ProgressVisualizerBase(stream, enableVT) {}

        void ShowProgress(uint64_t current, uint64_t maximum, ProgressType type);

        void EndProgress(bool hideProgressWhenDone);

    private:
        std::atomic<bool> m_isVisible = false;
        uint64_t m_lastCurrent = 0;

        void ShowProgressNoVT(uint64_t current, uint64_t maximum, ProgressType type);

        void ShowProgressWithVT(uint64_t current, uint64_t maximum, ProgressType type);
    };
}
