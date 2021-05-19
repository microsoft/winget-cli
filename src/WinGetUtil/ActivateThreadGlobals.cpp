#include "pch.h"
#include "Public/ThreadGlobals.h"

AppInstaller::ThreadLocalStorage::ThreadGlobals g_ThreadGlobals;

namespace AppInstaller::ThreadLocalStorage
{
    ThreadGlobals* ThreadGlobals::ActivateThreadGlobals(AppInstaller::ThreadLocalStorage::ThreadGlobals*)
    {
        static AppInstaller::ThreadLocalStorage::ThreadGlobals* t_pThreadGlobals = nullptr;

        if (t_pThreadGlobals == nullptr)
        {
            t_pThreadGlobals = &g_ThreadGlobals;
        }

        // If WinGetUtil were to call Telemetry logger as well, we would need to call SetForCurrentThread() method of ThreadGlobals to Create Telemetry logger object
        // As it is currently used by E2ETests, this doesn't seem to call Telemetry logger ever


        return t_pThreadGlobals;
    }
}
