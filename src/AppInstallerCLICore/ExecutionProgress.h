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
    // Displays an indefinite spinner.
    struct IndefiniteSpinner
    {
        IndefiniteSpinner(std::ostream& stream, bool enableVT) : 
            m_out(stream), m_enableVT(enableVT) {}

        void ShowSpinner();

        void StopSpinner();

        void DisableVT() { m_enableVT = false; }

    private:
        bool m_enableVT = false;
        std::atomic<bool> m_canceled = false;
        std::atomic<bool> m_spinnerRunning = false;
        std::future<void> m_spinnerJob;
        std::ostream& m_out;

        void ShowSpinnerInternalNoVT();

        void ShowSpinnerInternalWithVT();
    };

    // Displays progress 
    class ProgressBar
    {
    public:
        ProgressBar(std::ostream& stream, bool enableVT) :
            m_out(stream), m_enableVT(enableVT) {}

        void ShowProgress(bool running, uint64_t current, uint64_t maximum, ProgressType type);

        void DisableVT() { m_enableVT = false; }

        void EnableRainbow(bool enable) { m_enableRainbow = enable; }

    private:
        bool m_enableVT = false;
        bool m_enableRainbow = false;
        std::atomic<bool> m_isVisible = false;
        std::ostream& m_out;
        uint64_t m_lastCurrent = 0;

        void OutputBytes(uint64_t byteCount);

        void ClearLine();

        void ShowProgressNoVT(bool running, uint64_t current, uint64_t maximum, ProgressType type);

        void ShowProgressWithVT(bool running, uint64_t current, uint64_t maximum, ProgressType type);
    };
}
