// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceUpdateChecks.h"
#include <winget/UserSettings.h>

using namespace std::chrono_literals;

namespace AppInstaller::Repository
{
    bool IsBeforeDoNotUpdateBeforeTime(const SourceDetails& details)
    {
        if (std::chrono::system_clock::now() < details.DoNotUpdateBefore)
        {
            AICLI_LOG(Repo, Info, << "Background update is suppressed until: " << details.DoNotUpdateBefore);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IsAfterUpdateCheckTime(const SourceDetails& details, std::optional<TimeSpan> requestedUpdateInterval)
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
            auto timeSinceLastUpdate = std::chrono::system_clock::now() - details.LastUpdateTime;
            if (timeSinceLastUpdate > autoUpdateTime)
            {
                AICLI_LOG(Repo, Info, << "Source `" << details.Name << "` after auto update time [" <<
                    (requestedUpdateInterval ? "(override) " : "") <<
                    std::chrono::duration_cast<std::chrono::minutes>(autoUpdateTime).count() << " mins]; it has been at least " <<
                    std::chrono::duration_cast<std::chrono::minutes>(timeSinceLastUpdate).count() << " mins");
                return true;
            }
        }

        return false;
    }
}
