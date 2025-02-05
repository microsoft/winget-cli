// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/ILifetimeWatcher.h>
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::ConfigurationRemoting
{
    namespace
    {
        // The executable file name for the remote server process.
        constexpr std::wstring_view s_RemoteServerFileName = L"ConfigurationRemotingServer\\ConfigurationRemotingServer.exe";

        // The string used to divide the arguments sent to the remote server
        constexpr std::wstring_view s_ArgumentsDivider = L"\n~~~~~~\n";

        // A helper with a convenient function that we use to receive the remote factory object.
        struct RemoteFactoryCallback : winrt::implements<RemoteFactoryCallback, IConfigurationStatics>
        {
            RemoteFactoryCallback()
            {
                m_initEvent.create();
            }

            ConfigurationUnit CreateConfigurationUnit()
            {
                THROW_HR(E_NOTIMPL);
            }

            ConfigurationSet CreateConfigurationSet()
            {
                THROW_HR(E_NOTIMPL);
            }

            IAsyncOperation<IConfigurationSetProcessorFactory> CreateConfigurationSetProcessorFactoryAsync(winrt::hstring handler)
            {
                // TODO: Ensure calling process has same package identity
                std::wstringstream stringStream{ std::wstring{ static_cast<std::wstring_view>(handler) } };
                stringStream >> m_result;
                m_initEvent.SetEvent();
                return nullptr;
            }

            ConfigurationProcessor CreateConfigurationProcessor(IConfigurationSetProcessorFactory factory)
            {
                // TODO: Ensure calling process has same package identity
                m_factory = factory;
                m_initEvent.SetEvent();
                return nullptr;
            }

            bool IsConfigurationAvailable()
            {
                THROW_HR(E_NOTIMPL);
            }

            IAsyncActionWithProgress<uint32_t> EnsureConfigurationAvailableAsync()
            {
                THROW_HR(E_NOTIMPL);
            }

            IConfigurationSetProcessorFactory Wait(HANDLE process)
            {
                HANDLE waitHandles[2];
                waitHandles[0] = m_initEvent.get();
                waitHandles[1] = process;

                for (;;)
                {
                    // Wait up to 10 seconds for the server to complete initialization.
                    // This time is fairly arbitrary, although it does correspond with the maximum time for a COM fast rundown.
                    DWORD waitResult = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, 10000);
                    THROW_LAST_ERROR_IF(waitResult == WAIT_FAILED);

                    // The init event was signaled.
                    if (waitResult == WAIT_OBJECT_0)
                    {
                        break;
                    }

                    // Don't break things if the process is being debugged
                    if (waitResult == WAIT_TIMEOUT && IsDebuggerPresent())
                    {
                        continue;
                    }

                    // If the process exited, then try to use the exit code.
                    DWORD processExitCode = 0;
                    if (waitResult == (WAIT_OBJECT_0 + 1) && GetExitCodeProcess(process, &processExitCode) && FAILED(processExitCode))
                    {
                        THROW_HR(static_cast<HRESULT>(processExitCode));
                    }
                    else
                    {
                        // The server timed out or didn't have a failed exit code.
                        THROW_HR(E_FAIL);
                    }
                }

                THROW_IF_FAILED(m_result);

                // Double-check the result
                THROW_HR_IF(E_POINTER, !m_factory);
                return m_factory;
            }

        private:
            IConfigurationSetProcessorFactory m_factory;
            HRESULT m_result = S_OK;
            wil::unique_event m_initEvent;
        };

        // Represents a remote factory object that was created from a specific process.
        struct RemoteFactory : winrt::implements<RemoteFactory, IConfigurationSetProcessorFactory, SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties, winrt::cloaked<WinRT::ILifetimeWatcher>>, WinRT::LifetimeWatcherBase
        {
            RemoteFactory(bool useRunAs, const std::string& properties, const std::string& restrictions)
            {
                AICLI_LOG(Config, Verbose, << "Launching process for configuration processing...");

                // Create our callback and marshal it
                auto callback = winrt::make_self<RemoteFactoryCallback>();

                wil::com_ptr<IStream> stream;
                THROW_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));

                THROW_IF_FAILED(CoMarshalInterface(stream.get(), winrt::guid_of<IConfigurationStatics>(), reinterpret_cast<::IUnknown*>(winrt::get_abi(callback.as<IConfigurationStatics>())), MSHCTX_LOCAL, nullptr, MSHLFLAGS_NORMAL));

                ULARGE_INTEGER streamSize{};
                THROW_IF_FAILED(stream->Seek({}, STREAM_SEEK_CUR, &streamSize));

                ULONG bufferSize = static_cast<ULONG>(streamSize.QuadPart);
                std::vector<uint8_t> buffer;
                buffer.resize(bufferSize);

                THROW_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));
                ULONG bytesRead = 0;
                THROW_IF_FAILED(stream->Read(&buffer[0], bufferSize, &bytesRead));
                THROW_HR_IF(E_UNEXPECTED, bytesRead != bufferSize);

                std::wstring marshalledCallback = Utility::ConvertToUTF16(Utility::ConvertToHexString(buffer));

                // Create the event that the remote process will wait on to keep the object alive.
                std::wstring completionEventName = Utility::CreateNewGuidNameWString();
                m_completionEvent.create(wil::EventOptions::None, completionEventName.c_str());
                auto completeEventIfFailureDuringConstruction = wil::scope_exit([&]() { m_completionEvent.SetEvent(); });

                // This will be presented to the user so it must be formatted nicely.
                // Arguments are:
                // server.exe <marshalled callback object> <completion event name> <this process id>
                // 
                // Optionally, we may also place additional data that limits what the server may do as:
                // ~~~~~~
                // { "JSON properties" }
                // ~~~~~~
                // YAML configuration set definition
                std::wostringstream argumentsStream;
                argumentsStream << s_RemoteServerFileName << L' ' << marshalledCallback << L' ' << completionEventName << L' ' << GetCurrentProcessId();

                if (!properties.empty() && !restrictions.empty())
                {
                    argumentsStream << L' ' << s_ArgumentsDivider << Utility::ConvertToUTF16(properties) << s_ArgumentsDivider << Utility::ConvertToUTF16(restrictions);
                }

                std::wstring arguments = argumentsStream.str();

                std::filesystem::path serverPath = Runtime::GetPathTo(Runtime::PathName::SelfPackageRoot);
                serverPath /= s_RemoteServerFileName;
                std::wstring serverPathString = serverPath.wstring();

                // Per documentation, the maximum length is 32767 *counting* the null.
                THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW, serverPathString.length() > 32766);
                THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW, arguments.length() > 32766);
                // Overflow safe since we verify that each of the individual strings is also small.
                // +1 for the space between the path and args.
                THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW, serverPathString.length() + 1 + arguments.length() > 32766);

                SHELLEXECUTEINFOW execInfo = { 0 };
                execInfo.cbSize = sizeof(execInfo);
                execInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
                execInfo.lpFile = serverPath.c_str();
                execInfo.lpParameters = arguments.c_str();
                execInfo.nShow = SW_HIDE;

                if (useRunAs)
                {
                    execInfo.lpVerb = L"runas";
                }

                THROW_LAST_ERROR_IF(!ShellExecuteExW(&execInfo) || !execInfo.hProcess);

                wil::unique_process_handle process{ execInfo.hProcess };
                AICLI_LOG(Config, Verbose, << "  Configuration remote PID is " << GetProcessId(process.get()));

                m_remoteFactory = callback->Wait(process.get());
                AICLI_LOG(Config, Verbose, << "... configuration processing connection established.");

                completeEventIfFailureDuringConstruction.release();
            }

            ~RemoteFactory()
            {
                m_completionEvent.SetEvent();
            }

            IConfigurationSetProcessor CreateSetProcessor(const ConfigurationSet& configurationSet)
            {
                return m_remoteFactory.CreateSetProcessor(configurationSet);
            }

            winrt::event_token Diagnostics(const EventHandler<IDiagnosticInformation>& handler)
            {
                return m_remoteFactory.Diagnostics(handler);
            }

            void Diagnostics(const winrt::event_token& token) noexcept
            {
                m_remoteFactory.Diagnostics(token);
            }

            DiagnosticLevel MinimumLevel()
            {
                return m_remoteFactory.MinimumLevel();
            }

            void MinimumLevel(DiagnosticLevel value)
            {
                m_remoteFactory.MinimumLevel(value);
            }

            Collections::IVectorView<winrt::hstring> AdditionalModulePaths() const
            {
                return m_additionalModulePaths.GetView();
            }

            void AdditionalModulePaths(const Collections::IVectorView<winrt::hstring>& value)
            {
                // Extract all values from incoming view
                std::vector<winrt::hstring> newModulePaths{ value.Size() };
                value.GetMany(0, newModulePaths);

                // Create a copy for remote and set remote module paths
                std::vector<winrt::hstring> newRemotePaths{ newModulePaths };
                m_remoteAdditionalModulePaths = winrt::single_threaded_vector<winrt::hstring>(std::move(newRemotePaths));
                m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().AdditionalModulePaths(m_remoteAdditionalModulePaths.GetView());

                // Store the updated module paths that we were given
                m_additionalModulePaths = winrt::single_threaded_vector<winrt::hstring>(std::move(newModulePaths));
            }

            SetProcessorFactory::PwshConfigurationProcessorPolicy Policy() const
            {
                return m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().Policy();
            }

            void Policy(SetProcessorFactory::PwshConfigurationProcessorPolicy value)
            {
                m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().Policy(value);
            }

            SetProcessorFactory::PwshConfigurationProcessorLocation Location() const
            {
                return m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().Location();
            }

            void Location(SetProcessorFactory::PwshConfigurationProcessorLocation value)
            {
                m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().Location(value);
            }

            winrt::hstring CustomLocation() const
            {
                return m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().CustomLocation();
            }

            void CustomLocation(winrt::hstring value)
            {
                m_remoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>().CustomLocation(value);
            }

            HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher)
            {
                return WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
            }

        private:
            IConfigurationSetProcessorFactory m_remoteFactory;
            wil::unique_event m_completionEvent;
            Collections::IVector<winrt::hstring> m_additionalModulePaths{ winrt::single_threaded_vector<winrt::hstring>() };
            Collections::IVector<winrt::hstring> m_remoteAdditionalModulePaths{ winrt::single_threaded_vector<winrt::hstring>() };
        };
    }

    IConfigurationSetProcessorFactory CreateOutOfProcessFactory(bool useRunAs, const std::string& properties, const std::string& restrictions)
    {
        return winrt::make<RemoteFactory>(useRunAs, properties, restrictions);
    }
}

HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, LPWSTR staticsCallback, LPWSTR completionEventName, DWORD parentProcessId) try
{
    {
        wil::com_ptr<IGlobalOptions> globalOptions;
        RETURN_IF_FAILED(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_UNMARSHALING_POLICY, COMGLB_UNMARSHALING_POLICY_STRONG));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_EXCEPTION_HANDLING, COMGLB_EXCEPTION_DONOT_HANDLE_ANY));
    }

    using namespace AppInstaller;
    using namespace AppInstaller::CLI::ConfigurationRemoting;

    RETURN_HR_IF(E_POINTER, !staticsCallback);

    auto callbackBytes = Utility::ParseFromHexString(Utility::ConvertToUTF8(staticsCallback));
    RETURN_HR_IF(E_INVALIDARG, callbackBytes.size() > (1 << 15));

    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
    RETURN_IF_FAILED(stream->Write(&callbackBytes[0], static_cast<ULONG>(callbackBytes.size()), nullptr));
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));

    wil::com_ptr<::IUnknown> output;
    RETURN_IF_FAILED(CoUnmarshalInterface(stream.get(), winrt::guid_of<IConfigurationStatics>(), reinterpret_cast<void**>(&output)));

    IConfigurationStatics callback{ output.detach(), winrt::take_ownership_from_abi };

    if (FAILED(result))
    {
        std::ignore = callback.CreateConfigurationSetProcessorFactoryAsync(std::to_wstring(result));
    }
    else
    {
        IConfigurationSetProcessorFactory factoryObject;
        winrt::copy_from_abi(factoryObject, factory);
        std::ignore = callback.CreateConfigurationProcessor(factoryObject);
    }

    // Wait until the caller releases the object (signalling the event) or the parent process exits
    wil::unique_event completionEvent;
    completionEvent.open(completionEventName);
    wil::unique_process_handle parentProcess{ OpenProcess(SYNCHRONIZE, FALSE, parentProcessId) };

    HANDLE waitHandles[2];
    waitHandles[0] = completionEvent.get();
    waitHandles[1] = parentProcess.get();

    std::ignore = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);

    return S_OK;
}
CATCH_RETURN();
