// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceUpdateChecks.h"
#include "ISource.h"
#include <winget/UserSettings.h>

using namespace std::chrono_literals;

namespace AppInstaller::Repository
{
    bool ShouldUpdateBeforeOpen(ISourceReference* sourceReference, const std::optional<TimeSpan>& requestedUpdateInterval)
    {
        const SourceDetails& details = sourceReference->GetDetails();

        // Always respect this value to prevent server overloading
        if (IsBeforeDoNotUpdateBeforeTime(details))
        {
            return false;
        }

        // Allow the source reference to decide beyond this
        return sourceReference->ShouldUpdateBeforeOpen(requestedUpdateInterval);
    }

    bool IsBeforeDoNotUpdateBeforeTime(const SourceDetails& details)
    {
        if (std::chrono::system_clock::now() < details.DoNotUpdateBefore)
        {
            AICLI_LOG(Repo, Info, << "Background update for `" << details.Name << "` is suppressed until: " << details.DoNotUpdateBefore);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IsAfterUpdateCheckTime(const SourceDetails& details, std::optional<TimeSpan> requestedUpdateInterval)
    {
        return IsAfterUpdateCheckTime(details.Name, details.LastUpdateTime, requestedUpdateInterval);
    }

    bool IsAfterUpdateCheckTime(std::string_view name, std::chrono::system_clock::time_point lastUpdateTime, std::optional<TimeSpan> requestedUpdateInterval)
    {
        constexpr static TimeSpan s_ZeroMins = 0min;

        TimeSpan autoUpdateTime;
        if (requestedUpdateInterval)
        {
            autoUpdateTime = requestedUpdateInterval.value();
        }
        else
        {
            autoUpdateTime = Settings::User().Get<Settings::Setting::AutoUpdateTimeInMinutes>();
        }

        // A value of zero means no auto update, to get update the source run `winget update`
        if (autoUpdateTime != s_ZeroMins)
        {
            auto timeSinceLastUpdate = std::chrono::system_clock::now() - lastUpdateTime;
            if (timeSinceLastUpdate > autoUpdateTime)
            {
                AICLI_LOG(Repo, Info, << "Source `" << name << "` after auto update time [" <<
                    (requestedUpdateInterval ? "(override) " : "") <<
                    std::chrono::duration_cast<std::chrono::minutes>(autoUpdateTime).count() << " mins]; it has been at least " <<
                    std::chrono::duration_cast<std::chrono::minutes>(timeSinceLastUpdate).count() << " mins");
                return true;
            }
        }

        return false;
    }
}
