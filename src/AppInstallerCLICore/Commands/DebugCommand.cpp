// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#if _DEBUG
#include "DebugCommand.h"
#include <winrt/Microsoft.Management.Configuration.h>

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
}

#endif
