// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#if _DEBUG
#include "DebugCommand.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerRuntime.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>
#include "AppInstallerDownloader.h"
#include "Sixel.h"
#include <winget/Certificates.h>

using namespace AppInstaller::CLI::Execution;

namespace AppInstaller::CLI
{
    namespace
    {
        std::string MakeInterfaceNameAttribute(std::wstring_view name)
        {
            std::string result = Utility::ConvertToUTF8(name);
            Utility::FindAndReplace(result, "<", "&lt;");
            Utility::FindAndReplace(result, ">", "&gt;");
            return result;
        }

        std::string MakeIIDAttribute(const winrt::guid& guid)
        {
            std::string result;
            wchar_t buffer[256];

            if (StringFromGUID2(guid, buffer, ARRAYSIZE(buffer)))
            {
                result = AppInstaller::Utility::ConvertToUTF8(buffer);
                result = result.substr(1, result.length() - 2);
            }
            else
            {
                result = "error";
            }

            return result;
        }

        template <typename Interface>
        void OutputProxyStubInterfaceRegistration(Execution::Context& context)
        {
            context.Reporter.Info() << "<Interface Name=\"" << MakeInterfaceNameAttribute(winrt::name_of<Interface>()) << "\" InterfaceId=\"" << MakeIIDAttribute(winrt::guid_of<Interface>()) << "\" />" << std::endl;
        }

        template <typename Interface>
        void OutputIIDMapping(Execution::Context& context)
        {
            context.Reporter.Info() << Utility::ConvertToUTF8(winrt::name_of<Interface>()) << " == " << winrt::guid_of<Interface>() << std::endl;
        }
    }

    std::vector<std::unique_ptr<Command>> DebugCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<DumpProxyStubRegistrationsCommand>(FullName()),
            std::make_unique<DumpInterestingIIDsCommand>(FullName()),
            std::make_unique<DumpErrorResourceCommand>(FullName()),
            std::make_unique<ShowSixelCommand>(FullName()),
            std::make_unique<ProgressCommand>(FullName()),
            std::make_unique<GetSignerCommand>(FullName()),
            std::make_unique<LogViewerTestCommand>(FullName()),
            std::make_unique<DebugDscResourceCommand>(FullName()),
        });
    }

    Resource::LocString DebugCommand::ShortDescription() const
    {
        return Utility::LocIndString("Debug only dev commands"sv);
    }

    Resource::LocString DebugCommand::LongDescription() const
    {
        return Utility::LocIndString("Commands that are useful in debugging and development."sv);
    }

    void DebugCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    Resource::LocString DumpProxyStubRegistrationsCommand::ShortDescription() const
    {
        return Utility::LocIndString("Dump proxy-stub registrations"sv);
    }

    Resource::LocString DumpProxyStubRegistrationsCommand::LongDescription() const
    {
        return Utility::LocIndString("Dump proxy-stub registrations for WinRT interfaces to be place in the manifest."sv);
    }

    void DumpProxyStubRegistrationsCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationUnit>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationSet>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationConflict>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationParameter>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationConflictSetting>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::GetConfigurationUnitDetailsResult>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ApplyConfigurationUnitResult>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::TestConfigurationUnitResult>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::IApplyGroupMemberSettingsResult>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ITestSettingsResult>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::ConfigurationEnvironment>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Windows::Foundation::Collections::IIterable<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails>>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails2>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails3>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IGetAllSettingsConfigurationUnitProcessor>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IGetAllUnitsConfigurationUnitProcessor>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IFindUnitProcessorsSetProcessor>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationStatics2>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationStatics3>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>(context);

        // TODO: Fix the layering inversion created by the COM deployment API (probably in order to operate winget.exe against the COM server).
        //       Then this code can just have a CppWinRT reference to the deployment API and spit out the interface registrations just like for configuration.
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&MakeInterfaceNameAttribute), &module))
        {
            return;
        }

        // TODO: Have a PRIVATE export from WindowsPackageManager that returns a set of names and IIDs to include from the Deployment API surface
    }

    Resource::LocString DumpInterestingIIDsCommand::ShortDescription() const
    {
        return Utility::LocIndString("Dump some IIDs"sv);
    }

    Resource::LocString DumpInterestingIIDsCommand::LongDescription() const
    {
        return Utility::LocIndString("Dump some IIDs that might be useful."sv);
    }

    void DumpInterestingIIDsCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputIIDMapping<winrt::Microsoft::Management::Configuration::IConfigurationStatics>(context);
        OutputIIDMapping<winrt::Microsoft::Management::Configuration::IConfigurationStatics2>(context);
        OutputIIDMapping<winrt::Microsoft::Management::Configuration::IConfigurationStatics3>(context);
    }

    Resource::LocString DumpErrorResourceCommand::ShortDescription() const
    {
        return Utility::LocIndString("Dump error resources"sv);
    }

    Resource::LocString DumpErrorResourceCommand::LongDescription() const
    {
        return Utility::LocIndString("Dump the error information as resources."sv);
    }

    void DumpErrorResourceCommand::ExecuteInternal(Execution::Context& context) const
    {
        auto info = context.Reporter.Info();

        //  <data name="InstallFlowReturnCodeInstallInProgress" xml:space="preserve">
        //    <value>Another installation is already in progress. Try again later.</value>
        //  </data>
        for (const auto& error : Errors::GetWinGetErrors())
        {
            info <<
                "  <data name=\"" << error->Symbol() << "\" xml:space=\"preserve\">\n"
                "    <value>" << error->GetDescription() << "</value>\n"
                "  </data>" << std::endl;
        }
    }

#define WINGET_DEBUG_SIXEL_FILE Args::Type::Manifest
#define WINGET_DEBUG_SIXEL_ASPECT_RATIO Args::Type::AcceptPackageAgreements
#define WINGET_DEBUG_SIXEL_TRANSPARENT Args::Type::AcceptSourceAgreements
#define WINGET_DEBUG_SIXEL_COLOR_COUNT Args::Type::ConfigurationAcceptWarning
#define WINGET_DEBUG_SIXEL_WIDTH Args::Type::AdminSettingEnable
#define WINGET_DEBUG_SIXEL_HEIGHT Args::Type::AllowReboot
#define WINGET_DEBUG_SIXEL_STRETCH Args::Type::AllVersions
#define WINGET_DEBUG_SIXEL_REPEAT Args::Type::Name
#define WINGET_DEBUG_SIXEL_OUT_FILE Args::Type::BlockingPin

    std::vector<Argument> ShowSixelCommand::GetArguments() const
    {
        return {
            Argument{ "file", 'f', WINGET_DEBUG_SIXEL_FILE, Resource::String::SourceListUpdatedNever, ArgumentType::Positional },
            Argument{ "aspect-ratio", 'a', WINGET_DEBUG_SIXEL_ASPECT_RATIO, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "transparent", 't', WINGET_DEBUG_SIXEL_TRANSPARENT, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "color-count", 'c', WINGET_DEBUG_SIXEL_COLOR_COUNT, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "width", 'w', WINGET_DEBUG_SIXEL_WIDTH, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "height", 'h', WINGET_DEBUG_SIXEL_HEIGHT, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "stretch", 's', WINGET_DEBUG_SIXEL_STRETCH, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "repeat", 'r', WINGET_DEBUG_SIXEL_REPEAT, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "out-file", 'o', WINGET_DEBUG_SIXEL_OUT_FILE, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
        };
    }

    Resource::LocString ShowSixelCommand::ShortDescription() const
    {
        return Utility::LocIndString("Output an image with sixels"sv);
    }

    Resource::LocString ShowSixelCommand::LongDescription() const
    {
        return Utility::LocIndString("Outputs an image from a file using sixel format."sv);
    }

    void ShowSixelCommand::ExecuteInternal(Execution::Context& context) const
    {
        using namespace VirtualTerminal;
        std::unique_ptr<Sixel::Image> sixelImagePtr;

        std::string imageUrl{ context.Args.GetArg(WINGET_DEBUG_SIXEL_FILE) };

        if (Utility::IsUrlRemote(imageUrl))
        {
            auto imageStream = std::make_unique<std::stringstream>();
            ProgressCallback emptyCallback;
            Utility::DownloadToStream(imageUrl, *imageStream, Utility::DownloadType::Manifest, emptyCallback);

            sixelImagePtr = std::make_unique<Sixel::Image>(*imageStream, Manifest::IconFileTypeEnum::Unknown);
        }
        else
        {
            sixelImagePtr = std::make_unique<Sixel::Image>(Utility::ConvertToUTF16(imageUrl));
        }

        Sixel::Image& sixelImage = *sixelImagePtr.get();

        if (context.Args.Contains(WINGET_DEBUG_SIXEL_ASPECT_RATIO))
        {
            switch (context.Args.GetArg(WINGET_DEBUG_SIXEL_ASPECT_RATIO)[0])
            {
            case '1':
                sixelImage.AspectRatio(Sixel::AspectRatio::OneToOne);
                break;
            case '2':
                sixelImage.AspectRatio(Sixel::AspectRatio::TwoToOne);
                break;
            case '3':
                sixelImage.AspectRatio(Sixel::AspectRatio::ThreeToOne);
                break;
            case '5':
                sixelImage.AspectRatio(Sixel::AspectRatio::FiveToOne);
                break;
            }
        }

        sixelImage.Transparency(context.Args.Contains(WINGET_DEBUG_SIXEL_TRANSPARENT));

        if (context.Args.Contains(WINGET_DEBUG_SIXEL_COLOR_COUNT))
        {
            sixelImage.ColorCount(std::stoul(std::string{ context.Args.GetArg(WINGET_DEBUG_SIXEL_COLOR_COUNT) }));
        }

        if (context.Args.Contains(WINGET_DEBUG_SIXEL_WIDTH) && context.Args.Contains(WINGET_DEBUG_SIXEL_HEIGHT))
        {
            sixelImage.RenderSizeInCells(
                std::stoul(std::string{ context.Args.GetArg(WINGET_DEBUG_SIXEL_WIDTH) }),
                std::stoul(std::string{ context.Args.GetArg(WINGET_DEBUG_SIXEL_HEIGHT) }));
        }

        sixelImage.StretchSourceToFill(context.Args.Contains(WINGET_DEBUG_SIXEL_STRETCH));

        sixelImage.UseRepeatSequence(context.Args.Contains(WINGET_DEBUG_SIXEL_REPEAT));

        if (context.Args.Contains(WINGET_DEBUG_SIXEL_OUT_FILE))
        {
            std::ofstream stream{ Utility::ConvertToUTF16(context.Args.GetArg(WINGET_DEBUG_SIXEL_OUT_FILE)) };
            stream << sixelImage.Render().Get();
        }
        else
        {
            OutputStream stream = context.Reporter.GetOutputStream(Reporter::Level::Info);
            stream.ClearFormat();
            sixelImage.RenderTo(stream);

            // Force a new line to show entire image
            stream << std::endl;
        }
    }

#define WINGET_DEBUG_PROGRESS_SIXEL Args::Type::Manifest
#define WINGET_DEBUG_PROGRESS_DISABLED Args::Type::GatedVersion
#define WINGET_DEBUG_PROGRESS_HIDE Args::Type::AcceptPackageAgreements
#define WINGET_DEBUG_PROGRESS_TIME Args::Type::AcceptSourceAgreements
#define WINGET_DEBUG_PROGRESS_MESSAGE Args::Type::ConfigurationAcceptWarning
#define WINGET_DEBUG_PROGRESS_PERCENT Args::Type::AllowReboot
#define WINGET_DEBUG_PROGRESS_POST Args::Type::AllVersions

    std::vector<Argument> ProgressCommand::GetArguments() const
    {
        return {
            Argument{ "sixel", 's', WINGET_DEBUG_PROGRESS_SIXEL, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "disabled", 'd', WINGET_DEBUG_PROGRESS_DISABLED, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "hide", 'h', WINGET_DEBUG_PROGRESS_HIDE, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "time", 't', WINGET_DEBUG_PROGRESS_TIME, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "message", 'm', WINGET_DEBUG_PROGRESS_MESSAGE, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "percent", 'p', WINGET_DEBUG_PROGRESS_PERCENT, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "post", 0, WINGET_DEBUG_PROGRESS_POST, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
        };
    }

    Resource::LocString ProgressCommand::ShortDescription() const
    {
        return Utility::LocIndString("Show progress"sv);
    }

    Resource::LocString ProgressCommand::LongDescription() const
    {
        return Utility::LocIndString("Show progress with various controls to emulate different behaviors."sv);
    }

    void ProgressCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(WINGET_DEBUG_PROGRESS_SIXEL))
        {
            context.Reporter.SetStyle(Settings::VisualStyle::Sixel);
        }

        if (context.Args.Contains(WINGET_DEBUG_PROGRESS_DISABLED))
        {
            context.Reporter.SetStyle(Settings::VisualStyle::Disabled);
        }

        auto progress = context.Reporter.BeginAsyncProgress(context.Args.Contains(WINGET_DEBUG_PROGRESS_HIDE));

        if (context.Args.Contains(WINGET_DEBUG_PROGRESS_MESSAGE))
        {
            progress->Callback().SetProgressMessage(context.Args.GetArg(WINGET_DEBUG_PROGRESS_MESSAGE));
        }

        bool sendProgress = context.Args.Contains(WINGET_DEBUG_PROGRESS_PERCENT);

        UINT timeInSeconds = 3600;
        if (context.Args.Contains(WINGET_DEBUG_PROGRESS_TIME))
        {
            timeInSeconds = std::stoul(std::string{ context.Args.GetArg(WINGET_DEBUG_PROGRESS_TIME) });
        }

        UINT ticks = timeInSeconds * 10;
        for (UINT i = 0; i < ticks; ++i)
        {
            if (sendProgress)
            {
                progress->Callback().OnProgress(i, ticks, ProgressType::Bytes);
            }

            if (progress->Callback().IsCancelledBy(CancelReason::Any))
            {
                sendProgress = false;
                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        if (sendProgress)
        {
            progress->Callback().OnProgress(ticks, ticks, ProgressType::Bytes);
        }

        progress.reset();

        if (context.Args.Contains(WINGET_DEBUG_PROGRESS_POST))
        {
            context.Reporter.Info() << context.Args.GetArg(WINGET_DEBUG_PROGRESS_POST) << std::endl;
        }
    }

    std::vector<Argument> GetSignerCommand::GetArguments() const
    {
        return {
            Argument{ "file", 'f', Args::Type::Manifest, Resource::String::SourceListUpdatedNever, ArgumentType::Positional },
        };
    }

    Resource::LocString GetSignerCommand::ShortDescription() const
    {
        return Utility::LocIndString("Get signer information"sv);
    }

    Resource::LocString GetSignerCommand::LongDescription() const
    {
        return Utility::LocIndString("Gets the signing information for a given path."sv);
    }

    void GetSignerCommand::ExecuteInternal(Execution::Context& context) const
    {
        std::string subject = Certificates::GetAuthenticodeSubject(context.Args.GetArg(Args::Type::Manifest));

        context.Reporter.Info() << "Subject: " << subject << std::endl;
    }
    
// ── LogViewerTestCommand ─────────────────────────────────────────────────────

#define WINGET_DEBUG_LOG_VIEWER_FOLLOW Args::Type::Force

    std::vector<Argument> LogViewerTestCommand::GetArguments() const
    {
        return {
            Argument{ "follow", 'f', WINGET_DEBUG_LOG_VIEWER_FOLLOW, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
        };
    }

    Resource::LocString LogViewerTestCommand::ShortDescription() const
    {
        return Utility::LocIndString("Emit test logs for the log viewer extension"sv);
    }

    Resource::LocString LogViewerTestCommand::LongDescription() const
    {
        return Utility::LocIndString(
            "Emits log entries exercising every channel, level, subchannel, continuation line, "
            "and long-line feature of the WinGet Log Viewer VS Code extension. "
            "Use --follow to stream additional log lines every 3 seconds (up to 100 iterations)."sv);
    }

    void LogViewerTestCommand::ExecuteInternal(Execution::Context& context) const
    {
        // Ensure all channels and the most verbose level are active so every test entry lands in the file.
        auto& logger = AppInstaller::Logging::Log();
        logger.EnableChannel(AppInstaller::Logging::Channel::All);
        logger.SetLevel(AppInstaller::Logging::Level::Verbose);

        // ── All five levels on CLI ────────────────────────────────────────────
        AICLI_LOG(CLI, Verbose, << "Log viewer test: Verbose level message");
        AICLI_LOG(CLI, Info,    << "Log viewer test: Info level message");
        AICLI_LOG(CLI, Warning, << "Log viewer test: Warning level message");
        AICLI_LOG(CLI, Error,   << "Log viewer test: Error level message");
        AICLI_LOG(CLI, Crit,    << "Log viewer test: Critical level message");

        // ── One Info entry on every channel ──────────────────────────────────
        AICLI_LOG(Fail,     Info, << "Log viewer test: Failure channel");
        AICLI_LOG(SQL,      Info, << "Log viewer test: SQL channel");
        AICLI_LOG(Repo,     Info, << "Log viewer test: Repository channel");
        AICLI_LOG(YAML,     Info, << "Log viewer test: YAML channel");
        AICLI_LOG(Core,     Info, << "Log viewer test: Core channel");
        AICLI_LOG(Test,     Info, << "Log viewer test: Test channel");
        AICLI_LOG(Config,   Info, << "Log viewer test: Configuration channel");
        AICLI_LOG(Workflow, Info, << "Log viewer test: Workflow channel");

        // ── Subchannel simulation (sub-component logs routed through CLI) ─────
        AICLI_LOG(CLI, Info, << "[SQL ] Subchannel test: database query initiated for package lookup");
        AICLI_LOG(CLI, Info, << "[REPO] Subchannel test: fetching package metadata from remote source");

        // ── Continuation lines (newlines in the message become continuation rows) ──
        AICLI_LOG(Core, Warning, << "Package installation encountered multiple issues:\n"
            "  - Dependency 'vcredist' version 14.0.30704 not found in any configured source\n"
            "  - Insufficient disk space on C:\\ (requires 512 MB, available 203 MB)\n"
            "  - Installation directory is read-only: C:\\Program Files\\TestPackage\\1.0.0");

        // ── Long line (should require horizontal scroll or wrap in the viewer) ─
        AICLI_LOG(Workflow, Info, << "Resolving full package dependency graph: The following packages are required "
            "as dependencies and will be installed in sequence if not already present on the system: "
            "Microsoft.VCRedist.2015+.x64 (>= 14.0.30704), Microsoft.DotNet.Runtime.7 (>= 7.0.14), "
            "Microsoft.WebView2.Runtime (>= 113.0.1774.35), Microsoft.WindowsAppRuntime.1.4 (>= 1.4.231219000). "
            "Total estimated download size: 847 MB across 4 installers.");

        context.Reporter.Info() << "Log viewer test burst complete. Open the WinGet log file to review all viewer features." << std::endl;

        if (!context.Args.Contains(WINGET_DEBUG_LOG_VIEWER_FOLLOW))
        {
            return;
        }

        // ── Follow mode: stream log lines every 3 seconds ────────────────────
        context.Reporter.Info() << "Follow mode active (up to 100 iterations). Press Ctrl-C to stop." << std::endl;

        struct FollowEntry { AppInstaller::Logging::Channel Channel; AppInstaller::Logging::Level Level; std::string_view Message; };
        static constexpr FollowEntry s_entries[] =
        {
            { AppInstaller::Logging::Channel::CLI,      AppInstaller::Logging::Level::Info,    "Follow: searching for available package updates" },
            { AppInstaller::Logging::Channel::Repo,     AppInstaller::Logging::Level::Info,    "Follow: refreshing source index from remote endpoint" },
            { AppInstaller::Logging::Channel::SQL,      AppInstaller::Logging::Level::Verbose, "Follow: executing SELECT query on packages table" },
            { AppInstaller::Logging::Channel::Core,     AppInstaller::Logging::Level::Info,    "Follow: applying version comparison for upgrade eligibility" },
            { AppInstaller::Logging::Channel::YAML,     AppInstaller::Logging::Level::Verbose, "Follow: parsing manifest for Microsoft.TestPackage 2.1.0" },
            { AppInstaller::Logging::Channel::Workflow, AppInstaller::Logging::Level::Info,    "Follow: evaluating installer selection policy for current architecture" },
            { AppInstaller::Logging::Channel::Config,   AppInstaller::Logging::Level::Info,    "Follow: reading configuration resource state from DSC provider" },
            { AppInstaller::Logging::Channel::CLI,      AppInstaller::Logging::Level::Warning, "Follow: package is pinned to a specific version, skipping upgrade" },
            { AppInstaller::Logging::Channel::Core,     AppInstaller::Logging::Level::Info,    "Follow: SHA-256 hash verification passed for downloaded installer" },
            { AppInstaller::Logging::Channel::CLI,      AppInstaller::Logging::Level::Info,    "[SQL ] Follow: subchannel activity routed through CLI during follow" },
        };

        auto progress = context.Reporter.BeginAsyncProgress(true);

        for (int i = 1; i <= 100; ++i)
        {
            // Wait 3 seconds, checking for cancellation every 100 ms.
            for (int t = 0; t < 30; ++t)
            {
                if (progress->Callback().IsCancelledBy(CancelReason::Any)) { return; }
                std::this_thread::sleep_for(100ms);
            }

            // Emit the cycling entry for this iteration.
            const auto& e = s_entries[static_cast<size_t>(i) % ARRAYSIZE(s_entries)];
            const std::string msg = std::string(e.Message) + " [" + std::to_string(i) + "/100]";
            logger.Write(e.Channel, e.Level, msg);

            // Every 5 iterations: multi-line status summary (continuation lines).
            if (i % 5 == 0)
            {
                AICLI_LOG(Core, Info, << "Follow iteration " << i << " status summary:\n"
                    "  Packages checked: " << (i * 7) << "\n"
                    "  Updates available: " << (i % 3) << "\n"
                    "  Sources refreshed: 2");
            }

            // Every 20 iterations: simulated error with a stack trace (continuation lines + HRESULT).
            if (i % 20 == 0)
            {
                AICLI_LOG(Fail, Error, << "Simulated transient error at follow iteration " << i << " [HRESULT 0x80070005]:\n"
                    "  at AppInstaller::Repository::SourceList::OpenSource(std::string_view)\n"
                    "  at AppInstaller::CLI::Workflow::OpenSourcesForSearch(Context&)\n"
                    "  at AppInstaller::CLI::LogViewerTestCommand::ExecuteInternal(Context&)");
            }
        }

        context.Reporter.Info() << "Follow mode complete (100 iterations)." << std::endl;
    }
    
#define WINGET_DEBUG_DSC_RESOURCE_RESOURCE  Args::Type::SourceName
#define WINGET_DEBUG_DSC_RESOURCE_EXPORT    Args::Type::AllVersions

    std::vector<Argument> DebugDscResourceCommand::GetArguments() const
    {
        return {
            Argument{ "resource", 'r', WINGET_DEBUG_DSC_RESOURCE_RESOURCE, Resource::String::SourceListUpdatedNever, ArgumentType::Positional },
            Argument{ "export", 'e', WINGET_DEBUG_DSC_RESOURCE_EXPORT, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument::ForType(Args::Type::ConfigurationProcessorPath),
        };
    }

    Resource::LocString DebugDscResourceCommand::ShortDescription() const
    {
        return Utility::LocIndString("Run DSCv3 resource actions"sv);
    }

    Resource::LocString DebugDscResourceCommand::LongDescription() const
    {
        return Utility::LocIndString("Directly invokes DSCv3 resource functions through WinGet's infrastructure, without requiring a full configuration document."sv);
    }

    void DebugDscResourceCommand::ExecuteInternal(Execution::Context& context) const
    {
        using namespace winrt::Microsoft::Management::Configuration;
        using namespace winrt::Windows::Foundation::Collections;

        if (!context.Args.Contains(WINGET_DEBUG_DSC_RESOURCE_EXPORT))
        {
            OutputHelp(context.Reporter);
            return;
        }

        std::string resourceName{ context.Args.GetArg(WINGET_DEBUG_DSC_RESOURCE_RESOURCE) };

        context.Reporter.Info() << "Creating OOP DSCv3 processor factory..." << std::endl;

        IConfigurationSetProcessorFactory factory;
        if (Runtime::IsRunningWithLimitedToken())
        {
            factory = ConfigurationRemoting::CreateDynamicRuntimeFactory(ConfigurationRemoting::ProcessorEngine::DSCv3);
        }
        else
        {
            factory = ConfigurationRemoting::CreateOutOfProcessFactory(ConfigurationRemoting::ProcessorEngine::DSCv3);
        }

        auto factoryMap = factory.as<IMap<winrt::hstring, winrt::hstring>>();

        if (context.Args.Contains(Args::Type::ConfigurationProcessorPath))
        {
            factoryMap.Insert(ConfigurationRemoting::ToHString(ConfigurationRemoting::PropertyName::DscExecutablePath), Utility::ConvertToUTF16(context.Args.GetArg(Args::Type::ConfigurationProcessorPath)));
        }
        else
        {
            // Run the state machine to locate dsc.exe.
            for (;;)
            {
                winrt::hstring nextTransition = factoryMap.Lookup(ConfigurationRemoting::ToHString(ConfigurationRemoting::PropertyName::FindDscStateMachine));
                AICLI_LOG(CLI, Info, << "FindDscStateMachine: " << Utility::ConvertToUTF8(nextTransition));

                if (nextTransition == L"Found")
                {
                    break;
                }
                else if (nextTransition == L"NotFound")
                {
                    context.Reporter.Error() << Resource::String::ConfigurationInstallDscPackageFailed << std::endl;
                    AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
                }
                else
                {
                    // InstallStable/InstallPreview etc. are not supported in debug mode; install DSCv3 manually.
                    context.Reporter.Error() << "DSCv3 unavailable (" << Utility::ConvertToUTF8(nextTransition) << "); use --processor-path or install DSCv3." << std::endl;
                    AICLI_TERMINATE_CONTEXT(E_NOTIMPL);
                }
            }
        }

        if (Logging::Log().IsEnabled(Logging::Channel::Config, Logging::Level::Verbose))
        {
            factoryMap.Insert(ConfigurationRemoting::ToHString(ConfigurationRemoting::PropertyName::DiagnosticTraceEnabled), L"True");
        }

        // Build and configure the processor.
        ConfigurationProcessor processor{ factory };
        processor.Caller(L"winget-debug");

        if (Logging::Log().IsEnabled(Logging::Channel::Config, Logging::Level::Verbose))
        {
            processor.MinimumLevel(DiagnosticLevel::Verbose);
        }

        processor.Diagnostics([&context](const winrt::Windows::Foundation::IInspectable&, const IDiagnosticInformation& diag)
            {
                Logging::Level level = Logging::Level::Info;
                switch (diag.Level())
                {
                case DiagnosticLevel::Verbose: level = Logging::Level::Verbose; break;
                case DiagnosticLevel::Informational: level = Logging::Level::Info; break;
                case DiagnosticLevel::Warning: level = Logging::Level::Warning; break;
                case DiagnosticLevel::Error: level = Logging::Level::Error; break;
                case DiagnosticLevel::Critical: level = Logging::Level::Crit; break;
                }
                context.GetThreadGlobals().GetDiagnosticLogger().Write(Logging::Channel::Config, level, Utility::ConvertToUTF8(diag.Message()));
            });

        // Construct a minimal ConfigurationUnit for the named resource.
        ConfigurationUnit unit;
        unit.Type(Utility::ConvertToUTF16(resourceName));
        unit.Identifier(L"debug-item");
        unit.Intent(ConfigurationUnitIntent::Inform);

        context.Reporter.Info() << "Exporting resource: " << resourceName << std::endl;

        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationExportingUnit());

        GetAllConfigurationUnitsResult exportResult = nullptr;
        {
            auto exportAction = processor.GetAllUnitsAsync(unit);
            auto cancellationScope = progressScope->Callback().SetCancellationFunction([&]() { exportAction.Cancel(); });
            exportResult = exportAction.get();
        }

        progressScope.reset();

        HRESULT hr = exportResult.ResultInformation().ResultCode();
        if (FAILED(hr))
        {
            auto description = exportResult.ResultInformation().Description();
            context.Reporter.Error() << "Export failed (0x" << Logging::SetHRFormat << hr << "): ";
            if (!description.empty())
            {
                context.Reporter.Error() << Utility::ConvertToUTF8(description);
            }
            context.Reporter.Error() << std::endl;
            AICLI_TERMINATE_CONTEXT(hr);
        }

        auto units = exportResult.Units();
        context.Reporter.Info() << "Exported " << units.Size() << " instance(s):" << std::endl;

        for (const auto& resultUnit : units)
        {
            context.Reporter.Info() << "  Type:       " << Utility::ConvertToUTF8(resultUnit.Type()) << std::endl;
            context.Reporter.Info() << "  Identifier: " << Utility::ConvertToUTF8(resultUnit.Identifier()) << std::endl;

            auto settings = resultUnit.Settings();
            if (settings && settings.Size() > 0)
            {
                context.Reporter.Info() << "  Settings:" << std::endl;
                for (const auto& [key, value] : settings)
                {
                    auto prop = value.try_as<winrt::Windows::Foundation::IPropertyValue>();
                    if (prop)
                    {
                        std::string valueStr;
                        switch (prop.Type())
                        {
                        case winrt::Windows::Foundation::PropertyType::String:
                            valueStr = Utility::ConvertToUTF8(prop.GetString());
                            break;
                        case winrt::Windows::Foundation::PropertyType::Boolean:
                            valueStr = prop.GetBoolean() ? "true" : "false";
                            break;
                        case winrt::Windows::Foundation::PropertyType::Int32:
                            valueStr = std::to_string(prop.GetInt32());
                            break;
                        case winrt::Windows::Foundation::PropertyType::Int64:
                            valueStr = std::to_string(prop.GetInt64());
                            break;
                        default:
                            valueStr = "(unsupported type)";
                            break;
                        }
                        context.Reporter.Info() << "    " << Utility::ConvertToUTF8(key) << ": " << valueStr << std::endl;
                    }
                }
            }

            context.Reporter.Info() << std::endl;
        }
    }
}

#endif
