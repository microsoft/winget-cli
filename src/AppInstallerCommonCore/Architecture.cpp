// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerArchitecture.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    using namespace literals;
    namespace
    {
        // IsWow64GuestMachineSupported() is available starting on Windows 10, version 1709 (RS3).
        // We generally target a later version (version 1809, RS5), but the WinGetUtil is used in
        // Azure Functions that run on version 1607 (RS1) where it is not available. So, we load and
        // call this function only if available.
        using IsWow64GuestMachineSupportedPtr = decltype(&IsWow64GuestMachineSupported);

        struct IsWow64GuestMachineSupportedHelper
        {
            IsWow64GuestMachineSupportedHelper()
            {
                m_module.reset(LoadLibraryEx(L"api-ms-win-core-wow64-l1-1-2.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
                if (!m_module)
                {
                    AICLI_LOG(Core, Verbose, << "Could not load api-ms-win-core-wow64-l1-1-2.dll");
                    return;
                }

                m_isWow64GuestMachineSupported =
                    reinterpret_cast<IsWow64GuestMachineSupportedPtr>(GetProcAddress(m_module.get(), "IsWow64GuestMachineSupported"));
                if (!m_isWow64GuestMachineSupported)
                {
                    AICLI_LOG(Core, Verbose, << "Could not get proc address of IsWow64GuestMachineSupported");
                    return;
                }
            }

            void AddArchitectureIfGuestMachineSupported(std::vector<Architecture>& target, Architecture architecture, USHORT guestMachine)
            {
                if (m_isWow64GuestMachineSupported)
                {
                    BOOL supported = FALSE;
                    LOG_IF_FAILED(m_isWow64GuestMachineSupported(guestMachine, &supported));

                    if (supported)
                    {
                        target.push_back(architecture);
                    }
                }
            }

        private:
            wil::unique_hmodule m_module;
            IsWow64GuestMachineSupportedPtr m_isWow64GuestMachineSupported = nullptr;
        };

        void AddArchitectureIfGuestMachineSupported(std::vector<Architecture>& target, Architecture architecture, USHORT guestMachine)
        {
            IsWow64GuestMachineSupportedHelper helper;
            helper.AddArchitectureIfGuestMachineSupported(target, architecture, guestMachine);
        }

        // These types are defined in a future SDK and can be removed when we actually have them available.
        // The exception is that None was added (and this is an enum class).
        enum class MACHINE_ATTRIBUTES {
            None = 0,
            UserEnabled = 0x00000001,
            KernelEnabled = 0x00000002,
            Wow64Container = 0x00000004
        };

        DEFINE_ENUM_FLAG_OPERATORS(MACHINE_ATTRIBUTES);

        using GetMachineTypeAttributesPtr = HRESULT(WINAPI*)(USHORT Machine, MACHINE_ATTRIBUTES* MachineTypeAttributes);

        // GetMachineTypeAttributes can apparently replace IsWow64GuestMachineSupported, but no reason to do so right now.
        struct GetMachineTypeAttributesHelper
        {
            GetMachineTypeAttributesHelper()
            {
                m_module.reset(LoadLibraryEx(L"api-ms-win-core-processthreads-l1-1-7.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
                if (!m_module)
                {
                    AICLI_LOG(Core, Verbose, << "Could not load api-ms-win-core-processthreads-l1-1-7.dll");
                    return;
                }

                m_getMachineTypeAttributes =
                    reinterpret_cast<GetMachineTypeAttributesPtr>(GetProcAddress(m_module.get(), "GetMachineTypeAttributes"));
                if (!m_getMachineTypeAttributes)
                {
                    AICLI_LOG(Core, Verbose, << "Could not get proc address of GetMachineTypeAttributes");
                    return;
                }
            }

            void AddArchitectureIfMachineTypeAttributesUserEnabled(std::vector<Architecture>& target, Architecture architecture, USHORT guestMachine)
            {
                if (m_getMachineTypeAttributes)
                {
                    MACHINE_ATTRIBUTES attributes = MACHINE_ATTRIBUTES::None;
                    if (SUCCEEDED_LOG(m_getMachineTypeAttributes(guestMachine, &attributes)))
                    {
                        if (WI_IsFlagSet(attributes, MACHINE_ATTRIBUTES::UserEnabled))
                        {
                            target.push_back(architecture);
                        }
                    }
                }
            }

        private:
            wil::unique_hmodule m_module;
            GetMachineTypeAttributesPtr m_getMachineTypeAttributes = nullptr;
        };

        void AddArchitectureIfMachineTypeAttributesUserEnabled(std::vector<Architecture>& target, Architecture architecture, USHORT guestMachine)
        {
            GetMachineTypeAttributesHelper helper;
            helper.AddArchitectureIfMachineTypeAttributesUserEnabled(target, architecture, guestMachine);
        }

        // Gets the applicable architectures for the current machine.
        std::vector<Architecture> CreateApplicableArchitecturesVector()
        {
            std::vector<Architecture> applicableArchs;

            switch (GetSystemArchitecture())
            {
            case Architecture::Arm64:
            {
                applicableArchs.push_back(Architecture::Arm64);
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::Arm, IMAGE_FILE_MACHINE_ARMNT);
                AddArchitectureIfMachineTypeAttributesUserEnabled(applicableArchs, Architecture::X64, IMAGE_FILE_MACHINE_AMD64);
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::X86, IMAGE_FILE_MACHINE_I386);
                applicableArchs.push_back(Architecture::Neutral);
            }
                break;
            case Architecture::Arm:
                applicableArchs.push_back(Architecture::Arm);
                applicableArchs.push_back(Architecture::Neutral);
                break;
            case Architecture::X86:
                applicableArchs.push_back(Architecture::X86);
                applicableArchs.push_back(Architecture::Neutral);
                break;
            case Architecture::X64:
                applicableArchs.push_back(Architecture::X64);
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::X86, IMAGE_FILE_MACHINE_I386);
                applicableArchs.push_back(Architecture::Neutral);
                break;
            default:
                applicableArchs.push_back(Architecture::Neutral);
            }

            return applicableArchs;
        }
    }

    Architecture ConvertToArchitectureEnum(const std::string& archStr)
    {
        std::string arch = ToLower(archStr);
        if (arch == "x86")
        {
            return Architecture::X86;
        }
        else if (arch == "x64")
        {
            return Architecture::X64;
        }
        else if (arch == "arm")
        {
            return Architecture::Arm;
        }
        else if (arch == "arm64")
        {
            return Architecture::Arm64;
        }
        else if (arch == "neutral")
        {
            return Architecture::Neutral;
        }

        AICLI_LOG(YAML, Info, << "ConvertToArchitectureEnum: Unknown architecture: " << archStr);
        return Architecture::Unknown;
    }

    LocIndView ToString(Architecture architecture)
    {
        switch (architecture)
        {
        case Architecture::Neutral:
            return "Neutral"_liv;
        case Architecture::X86:
            return "X86"_liv;
        case Architecture::X64:
            return "X64"_liv;
        case Architecture::Arm:
            return "Arm"_liv;
        case Architecture::Arm64:
            return "Arm64"_liv;
        }

        return "Unknown"_liv;
    }

    Architecture GetSystemArchitecture()
    {
        Architecture systemArchitecture = Architecture::Unknown;

        SYSTEM_INFO systemInfo;
        ZeroMemory(&systemInfo, sizeof(SYSTEM_INFO));
        GetNativeSystemInfo(&systemInfo);

        switch (systemInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:
            systemArchitecture = Architecture::X64;
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            systemArchitecture = Architecture::Arm;
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            systemArchitecture = Architecture::Arm64;
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            systemArchitecture = Architecture::X86;
            break;
        }

        return systemArchitecture;
    }

    const std::vector<Architecture>& GetApplicableArchitectures()
    {
        static std::vector<Architecture> applicableArchs = CreateApplicableArchitecturesVector();
        return applicableArchs;
    }

    int IsApplicableArchitecture(Architecture arch)
    {
        return IsApplicableArchitecture(arch, GetApplicableArchitectures());
    }

    int IsApplicableArchitecture(Architecture arch, const std::vector<Architecture>& allowedArchitectures)
    {
        auto it = std::find(allowedArchitectures.begin(), allowedArchitectures.end(), arch);

        if (it != allowedArchitectures.end())
        {
            return static_cast<int>(std::distance(it, allowedArchitectures.end()));
        }
        else
        {
            return InapplicableArchitecture;
        }
    }
}