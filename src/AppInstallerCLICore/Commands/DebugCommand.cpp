// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#if _DEBUG
#include "DebugCommand.h"
#include <winrt/Microsoft.Management.Configuration.h>
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

    std::vector<Argument> ShowSixelCommand::GetArguments() const
    {
        return {
            Argument{ "file", 'f', Args::Type::Manifest, Resource::String::SourceListUpdatedNever, ArgumentType::Positional },
            Argument{ "aspect-ratio", 'a', Args::Type::AcceptPackageAgreements, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "transparent", 't', Args::Type::AcceptSourceAgreements, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "color-count", 'c', Args::Type::ConfigurationAcceptWarning, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "width", 'w', Args::Type::AdminSettingEnable, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "height", 'h', Args::Type::AllowReboot, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "stretch", 's', Args::Type::AllVersions, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "repeat", 'r', Args::Type::Name, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "out-file", 'o', Args::Type::BlockingPin, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
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
        std::unique_ptr<SixelImage> sixelImagePtr;

        std::string imageUrl{ context.Args.GetArg(Args::Type::Manifest) };

        if (Utility::IsUrlRemote(imageUrl))
        {
            auto imageStream = std::make_unique<std::stringstream>();
            ProgressCallback emptyCallback;
            Utility::DownloadToStream(imageUrl, *imageStream, Utility::DownloadType::Manifest, emptyCallback);

            sixelImagePtr = std::make_unique<SixelImage>(*imageStream, Manifest::IconFileTypeEnum::Unknown);
        }
        else
        {
            sixelImagePtr = std::make_unique<SixelImage>(Utility::ConvertToUTF16(imageUrl));
        }

        SixelImage& sixelImage = *sixelImagePtr.get();

        if (context.Args.Contains(Args::Type::AcceptPackageAgreements))
        {
            switch (context.Args.GetArg(Args::Type::AcceptPackageAgreements)[0])
            {
            case '1':
                sixelImage.AspectRatio(SixelAspectRatio::OneToOne);
                break;
            case '2':
                sixelImage.AspectRatio(SixelAspectRatio::TwoToOne);
                break;
            case '3':
                sixelImage.AspectRatio(SixelAspectRatio::ThreeToOne);
                break;
            case '5':
                sixelImage.AspectRatio(SixelAspectRatio::FiveToOne);
                break;
            }
        }

        sixelImage.Transparency(context.Args.Contains(Args::Type::AcceptSourceAgreements));

        if (context.Args.Contains(Args::Type::ConfigurationAcceptWarning))
        {
            sixelImage.ColorCount(std::stoul(std::string{ context.Args.GetArg(Args::Type::ConfigurationAcceptWarning) }));
        }

        if (context.Args.Contains(Args::Type::AdminSettingEnable) && context.Args.Contains(Args::Type::AllowReboot))
        {
            sixelImage.RenderSizeInCells(
                std::stoul(std::string{ context.Args.GetArg(Args::Type::AdminSettingEnable) }),
                std::stoul(std::string{ context.Args.GetArg(Args::Type::AllowReboot) }));
        }

        sixelImage.StretchSourceToFill(context.Args.Contains(Args::Type::AllVersions));

        sixelImage.UseRepeatSequence(context.Args.Contains(Args::Type::Name));

        if (context.Args.Contains(Args::Type::BlockingPin))
        {
            std::ofstream stream{ Utility::ConvertToUTF16(context.Args.GetArg(Args::Type::BlockingPin)) };
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

    std::vector<Argument> ProgressCommand::GetArguments() const
    {
        return {
            Argument{ "sixel", 's', Args::Type::Manifest, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "hide", 'h', Args::Type::AcceptPackageAgreements, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "time", 't', Args::Type::AcceptSourceAgreements, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "message", 'm', Args::Type::ConfigurationAcceptWarning, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
            Argument{ "percent", 'p', Args::Type::AllowReboot, Resource::String::SourceListUpdatedNever, ArgumentType::Flag },
            Argument{ "post", 0, Args::Type::AllVersions, Resource::String::SourceListUpdatedNever, ArgumentType::Standard },
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
        if (context.Args.Contains(Args::Type::Manifest))
        {
            context.Reporter.SetStyle(Settings::VisualStyle::Sixel);
        }

        auto progress = context.Reporter.BeginAsyncProgress(context.Args.Contains(Args::Type::AcceptPackageAgreements));

        if (context.Args.Contains(Args::Type::ConfigurationAcceptWarning))
        {
            progress->Callback().SetProgressMessage(context.Args.GetArg(Args::Type::ConfigurationAcceptWarning));
        }

        bool sendProgress = context.Args.Contains(Args::Type::AllowReboot);

        UINT timeInSeconds = 3600;
        if (context.Args.Contains(Args::Type::AcceptSourceAgreements))
        {
            timeInSeconds = std::stoul(std::string{ context.Args.GetArg(Args::Type::AcceptSourceAgreements) });
        }

        for (UINT i = 0; i < timeInSeconds; ++i)
        {
            if (sendProgress)
            {
                progress->Callback().OnProgress(i, timeInSeconds, ProgressType::Bytes);
            }

            if (progress->Callback().IsCancelledBy(CancelReason::Any))
            {
                sendProgress = false;
                break;
            }

            std::this_thread::sleep_for(1s);
        }

        if (sendProgress)
        {
            progress->Callback().OnProgress(timeInSeconds, timeInSeconds, ProgressType::Bytes);
        }

        progress.reset();

        if (context.Args.Contains(Args::Type::AllVersions))
        {
            context.Reporter.Info() << context.Args.GetArg(Args::Type::AllVersions) << std::endl;
        }
    }
}

#endif
