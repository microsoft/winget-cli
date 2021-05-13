#include "pch.h"
#include "Public/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    ThreadGlobals* ThreadGlobals::ActivateThreadGlobals(AppInstaller::ThreadLocalStorage::ThreadGlobals* pThreadGlobals)
    {
        static AppInstaller::ThreadLocalStorage::ThreadGlobals* t_pThreadGlobals = nullptr;

        if (pThreadGlobals)
        {
            t_pThreadGlobals = pThreadGlobals;
        }

        return t_pThreadGlobals;
    }
}
