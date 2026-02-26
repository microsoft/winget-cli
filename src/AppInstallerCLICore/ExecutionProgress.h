// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ChannelStreams.h"
#include <AppInstallerProgress.h>
#include <AppInstallerStrings.h>
#include <winget/UserSettings.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::CLI::Execution
{
    // Displays an indefinite spinner.
    struct IIndefiniteSpinner
    {
        virtual ~IIndefiniteSpinner() = default;

        // Set the message for the spinner.
        virtual void SetMessage(std::string_view message) = 0;

        // Get the current message for the spinner.
        virtual std::shared_ptr<Utility::NormalizedString> Message() = 0;

        // Show the indefinite spinner.
        virtual void ShowSpinner() = 0;

        // Stop showing the indefinite spinner.
        virtual void StopSpinner() = 0;

        // Creates an indefinite spinner for the given style.
        static std::unique_ptr<IIndefiniteSpinner> CreateForStyle(BaseStream& stream, bool enableVT, AppInstaller::Settings::VisualStyle style, const std::function<bool()>& sixelSupported);
    };

    // Displays a progress bar.
    struct IProgressBar
    {
        virtual ~IProgressBar() = default;

        // Show progress with the given values.
        virtual void ShowProgress(uint64_t current, uint64_t maximum, ProgressType type) = 0;

        // Stop showing progress.
        virtual void EndProgress(bool hideProgressWhenDone) = 0;

        // Creates a progress bar for the given style.
        static std::unique_ptr<IProgressBar> CreateForStyle(BaseStream& stream, bool enableVT, AppInstaller::Settings::VisualStyle style, const std::function<bool()>& sixelSupported);
    };
}
