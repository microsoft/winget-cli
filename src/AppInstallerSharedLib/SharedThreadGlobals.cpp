// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SharedThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

    static ThreadGlobals* SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals = nullptr)
    {
        thread_local AppInstaller::ThreadLocalStorage::ThreadGlobals* t_pThreadGlobals = nullptr;

        if (setThreadGlobals)
        {
            AppInstaller::ThreadLocalStorage::ThreadGlobals* previous_pThreadGlobals = t_pThreadGlobals;
            t_pThreadGlobals = pThreadGlobals;
            return previous_pThreadGlobals;
        }

        return t_pThreadGlobals;
    }

    std::unique_ptr<PreviousThreadGlobals> ThreadGlobals::SetForCurrentThread()
    {
        return std::make_unique<PreviousThreadGlobals>(SetOrGetThreadGlobals(true, this));
    }

    ThreadGlobals* ThreadGlobals::GetForCurrentThread()
    {
        return SetOrGetThreadGlobals(false);
    }

    PreviousThreadGlobals::~PreviousThreadGlobals()
    {
        std::ignore = SetOrGetThreadGlobals(true, m_previous);
    }
}
