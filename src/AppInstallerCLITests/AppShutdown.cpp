// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Runtime.h>
#include "Commands/TestCommand.h"
#include "..\Microsoft.Management.Configuration\ShutdownSynchronization.h"

using namespace AppInstaller::CLI;

TEST_CASE("AppShutdown_WindowMessage", "[appShutdown]")
{
    if (AppInstaller::Runtime::IsRunningAsAdmin() && AppInstaller::Runtime::IsRunningInPackagedContext())
    {
        WARN("Test can't run as admin in package context");
        return;
    }

    std::ostringstream output;
    Execution::Context context{ output, std::cin };
    context.Args.AddArg(Execution::Args::Type::Force);

    TestAppShutdownCommand appShutdownCmd({});
    appShutdownCmd.Execute(context);

    REQUIRE(context.IsTerminated());
    REQUIRE(S_OK == context.GetTerminationHR());
}

TEST_CASE("ConfigurationShutdown_CancelExpiredWork", "[appShutdown]")
{
    using namespace winrt::Microsoft::Management::Configuration::implementation;

    ShutdownSynchronization synchronization;
    ShutdownAwareAsyncCancellationPromise promise;
    AppInstaller::WinRT::AsyncCancellation active{ winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ &promise } };
    synchronization.RegisterWorkBegin(active.GetWeak());

    for (int i = 0; i < 3; ++i)
    {
        AppInstaller::WinRT::AsyncCancellation expired{ winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ &promise } };
        synchronization.RegisterWorkBegin(expired.GetWeak());
    }

    synchronization.CancelAllWork();
    REQUIRE(active.IsCancelled());

    synchronization.RegisterWorkEnd(active.GetWeak());
    synchronization.Wait();
}

TEST_CASE("ConfigurationShutdown_WaitForExpiredWork", "[appShutdown]")
{
    using namespace winrt::Microsoft::Management::Configuration::implementation;

    ShutdownSynchronization synchronization;
    ShutdownAwareAsyncCancellationPromise promise;

    for (int i = 0; i < 3; ++i)
    {
        AppInstaller::WinRT::AsyncCancellation expired{ winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ &promise } };
        synchronization.RegisterWorkBegin(expired.GetWeak());
    }

    synchronization.Wait();
    synchronization.BlockNewWork();

    AppInstaller::WinRT::AsyncCancellation newWork{ winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ &promise } };
    REQUIRE_THROWS_HR(synchronization.RegisterWorkBegin(newWork.GetWeak()), E_ABORT);
}
