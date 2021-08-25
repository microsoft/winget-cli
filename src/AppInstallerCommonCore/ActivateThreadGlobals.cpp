#include "pch.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    ThreadGlobals* ThreadGlobals::SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals)
    {
        thread_local AppInstaller::ThreadLocalStorage::ThreadGlobals* t_pThreadGlobals = nullptr;

        if (setThreadGlobals == true)
        {
            t_pThreadGlobals = pThreadGlobals;
        }

        return t_pThreadGlobals;
    }
}
