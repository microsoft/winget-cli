// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#if _DEBUG
#include "DebugCommand.h"
#include <winrt/Microsoft.Management.Configuration.h>
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>
#include "AppInstallerDownloader.h"
#include "Sixel.h"

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
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails2>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IGetAllSettingsConfigurationUnitProcessor>(context);
        OutputProxyStubInterfaceRegistration<winrt::Microsoft::Management::Configuration::IConfigurationStatics2>(context);
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
}

#endif
