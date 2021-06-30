#pragma once

//A version of CoCreatableCppWinRtClass that lets you pass in a uuid rather than getting it from a class property.
#define CoCreatableClassWithCLSIDWithFactory(className, instance, clsid, factory) \
    InternalWrlCreateCreatorMap(className##instance##_COM, clsid, nullptr, ::Microsoft::WRL::Details::CreateClassFactory<factory>, "minATL$__f")
#define CoCreatableCppWinRtClassWithCLSID(className, instance, clsid) CoCreatableClassWithCLSIDWithFactory(className, instance, clsid, ::wil::wrl_factory_for_winrt_com_class<className>)

#define WINGET_CATCH_RESULT_EXCEPTION_STORE(exceptionHR)   catch (const wil::ResultException& re) { exceptionHR = re.GetErrorCode(); }
#define WINGET_CATCH_HRESULT_EXCEPTION_STORE(exceptionHR)   catch (const winrt::hresult_error& hre) { exceptionHR = hre.code(); }
#define WINGET_CATCH_COMMAND_EXCEPTION_STORE(exceptionHR)   catch (const ::AppInstaller::CLI::CommandException&) { exceptionHR = APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS; }
#define WINGET_CATCH_POLICY_EXCEPTION_STORE(exceptionHR)   catch (const ::AppInstaller::Settings::GroupPolicyException&) { exceptionHR = APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS; }
#define WINGET_CATCH_STD_EXCEPTION_STORE(exceptionHR)   catch (const std::exception&) { exceptionHR = APPINSTALLER_CLI_ERROR_COMMAND_FAILED; }
#define WINGET_CATCH_ALL_EXCEPTION_STORE(exceptionHR)   catch (...) { exceptionHR = APPINSTALLER_CLI_ERROR_COMMAND_FAILED; }
#define WINGET_CATCH_STORE(exceptionHR) \
    WINGET_CATCH_RESULT_EXCEPTION_STORE(exceptionHR) \
    WINGET_CATCH_HRESULT_EXCEPTION_STORE(exceptionHR) \
    WINGET_CATCH_COMMAND_EXCEPTION_STORE(exceptionHR) \
    WINGET_CATCH_POLICY_EXCEPTION_STORE(exceptionHR) \
    WINGET_CATCH_STD_EXCEPTION_STORE(exceptionHR) \
    WINGET_CATCH_ALL_EXCEPTION_STORE(exceptionHR)

namespace winrt::Microsoft::Management::Deployment::implementation
{
    enum class Capability
    {
        PackageManagement,
        PackageQuery
    };

    HRESULT EnsureProcessHasCapability(Capability requiredCapability, DWORD callerProcessId);
    HRESULT EnsureComCallerHasCapability(Capability requiredCapability);
    std::optional<DWORD> GetCallerProcessId();
    std::wstring TryGetCallerProcessInfo(DWORD callerProcessId);
}