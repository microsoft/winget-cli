// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/ILifetimeWatcher.h>
#include <winrt/Microsoft.Management.Configuration.Processor.h>
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::ConfigurationRemoting
{
    namespace details
    {
        // The layout of the memory being mapped.
        struct MappedMemoryValue
        {
            // The size of the memory block itself.
            static constexpr ULONG s_MemorySize = 4 << 10;

            HRESULT Result;
            ULONG FactorySize;
            uint8_t FactoryObject[1];

            // The maximum size of the marshalled object.
            static constexpr ULONG MaxFactorySize()
            {
                static_assert(s_MemorySize > offsetof(MappedMemoryValue, FactoryObject));
                return s_MemorySize - offsetof(MappedMemoryValue, FactoryObject);
            }
        };
    }

    namespace
    {
        // The name of the directory containing additional modules.
        constexpr std::wstring_view s_ExternalModulesName = L"ExternalModules";

        // The executable file name for the remote server process.
        constexpr std::wstring_view s_RemoteServerFileName = L"ConfigurationRemotingServer\\ConfigurationRemotingServer.exe";

        // The string used to divide the arguments sent to the remote server
        constexpr std::wstring_view s_ArgumentsDivider = L"\n---\n";

        // Represents a remote factory object that was created from a specific process.
        struct RemoteFactory : winrt::implements<RemoteFactory, IConfigurationSetProcessorFactory, SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties, winrt::cloaked<WinRT::ILifetimeWatcher>>, WinRT::LifetimeWatcherBase
        {
            RemoteFactory(bool useRunAs, const std::string& properties, const std::string& restrictions)
            {
                AICLI_LOG(Config, Verbose, << "Launching process for configuration processing...");

                // Create file mapping backed by page file.
                std::wstring fileMappingName = Utility::CreateNewGuidWString();
                wil::unique_handle memoryHandle{ CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, details::MappedMemoryValue::s_MemorySize, fileMappingName.c_str()) };
                THROW_LAST_ERROR_IF_NULL(memoryHandle);

                // Map the memory into the process.
                wil::unique_mapview_ptr<details::MappedMemoryValue> mappedMemory{ reinterpret_cast<details::MappedMemoryValue*>(MapViewOfFile(memoryHandle.get(), FILE_MAP_READ | FILE_MAP_WRITE , 0, 0, 0)) };
                THROW_LAST_ERROR_IF_NULL(mappedMemory);
                // Initialize the result to a failure in case the other process never comes through.
                mappedMemory->Result = E_FAIL;

                // Create an event that the remote process will signal to indicate it has completed creating the object.
                std::wstring initEventName = Utility::CreateNewGuidWString();
                wil::unique_event initEvent;
                initEvent.create(wil::EventOptions::None, initEventName.c_str());

                // Create the event that the remote process will wait on to keep the object alive.
                std::wstring completionEventName = Utility::CreateNewGuidWString();
                m_completionEvent.create(wil::EventOptions::None, completionEventName.c_str());
                auto completeEventIfFailureDuringConstruction = wil::scope_exit([&]() { m_completionEvent.SetEvent(); });

                // Arguments are:
                // server.exe <mapped memory name> <init event name> <completion event name> <this process id>
                std::wostringstream argumentsStream;
                argumentsStream << s_RemoteServerFileName << L' ' << fileMappingName << L' ' << initEventName << L' ' << completionEventName << L' ' << GetCurrentProcessId();

                if (!properties.empty() && !restrictions.empty())
                {
                    argumentsStream << s_ArgumentsDivider << Utility::ConvertToUTF16(properties) << s_ArgumentsDivider << Utility::ConvertToUTF16(restrictions);
                }

                std::wstring arguments = argumentsStream.str();
                // Per documentation, the maximum length is 32767 *counting* the null.
                THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW, arguments.length() > 32766);

                std::filesystem::path serverPath = Runtime::GetPathTo(Runtime::PathName::SelfPackageRoot);
                serverPath /= s_RemoteServerFileName;

                // TODO: Convert to ShellExecute and use runas when requested

                STARTUPINFOW startupInfo{};
                startupInfo.cb = sizeof(startupInfo);
                wil::unique_process_information processInformation;

                THROW_IF_WIN32_BOOL_FALSE(CreateProcessW(serverPath.c_str(), &arguments[0], nullptr, nullptr, TRUE, DETACHED_PROCESS, nullptr, nullptr, &startupInfo, &processInformation));
                AICLI_LOG(Config, Verbose, << "  Configuration remote PID is " << processInformation.dwProcessId);

                HANDLE waitHandles[2];
                waitHandles[0] = initEvent.get();
                waitHandles[1] = processInformation.hProcess;

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
                    if (waitResult == (WAIT_OBJECT_0 + 1) && GetExitCodeProcess(processInformation.hProcess, &processExitCode) && FAILED(processExitCode))
                    {
                        THROW_HR(static_cast<HRESULT>(processExitCode));
                    }
                    else
                    {
                        // The server timed out or didn't have a failed exit code.
                        THROW_HR(E_FAIL);
                    }
                }

                // Report on a failure in the server.
                THROW_IF_FAILED(mappedMemory->Result);

                THROW_HR_IF(E_NOT_SUFFICIENT_BUFFER, mappedMemory->FactorySize == 0);
                THROW_HR_IF(E_NOT_SUFFICIENT_BUFFER, mappedMemory->FactorySize > details::MappedMemoryValue::MaxFactorySize());

                wil::com_ptr<IStream> stream;
                THROW_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
                THROW_IF_FAILED(stream->Write(mappedMemory->FactoryObject, mappedMemory->FactorySize, nullptr));
                THROW_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));

                wil::com_ptr<::IUnknown> output;
                THROW_IF_FAILED(CoUnmarshalInterface(stream.get(), winrt::guid_of<IConfigurationSetProcessorFactory>(), reinterpret_cast<void**>(&output)));
                AICLI_LOG(Config, Verbose, << "... configuration processing connection established.");
                m_remoteFactory = IConfigurationSetProcessorFactory{ output.detach(), winrt::take_ownership_from_abi };

                // The additional modules path is a direct child directory to the package root
                std::filesystem::path externalModules = Runtime::GetPathTo(Runtime::PathName::SelfPackageRoot) / s_ExternalModulesName;
                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), !std::filesystem::is_directory(externalModules));
                m_internalAdditionalModulePaths.emplace_back(externalModules.wstring());
                m_remoteAdditionalModulePaths = winrt::single_threaded_vector<winrt::hstring>(std::vector<winrt::hstring>{ m_internalAdditionalModulePaths });

                auto factoryProperties = m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>();
                AICLI_LOG(Config, Verbose, << "Applying built in additional module path: " << externalModules.u8string());
                factoryProperties.AdditionalModulePaths(m_remoteAdditionalModulePaths.GetView());
                factoryProperties.ProcessorType(Processor::PowerShellConfigurationProcessorType::Hosted);

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

                // Combine with our own values
                std::vector<winrt::hstring> newRemotePaths{ newModulePaths };
                newRemotePaths.insert(newRemotePaths.end(), m_internalAdditionalModulePaths.begin(), m_internalAdditionalModulePaths.end());

                // Apply the new combined paths and pass to remote factory
                m_remoteAdditionalModulePaths = winrt::single_threaded_vector<winrt::hstring>(std::move(newRemotePaths));
                m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().AdditionalModulePaths(m_remoteAdditionalModulePaths.GetView());

                // Store the updated module paths that we were given
                m_additionalModulePaths = winrt::single_threaded_vector<winrt::hstring>(std::move(newModulePaths));
            }

            SetProcessorFactory::PwshConfigurationProcessorPolicy Policy() const
            {
                return Convert(m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().Policy());
            }

            void Policy(SetProcessorFactory::PwshConfigurationProcessorPolicy value)
            {
                m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().Policy(Convert(value));
            }

            SetProcessorFactory::PwshConfigurationProcessorLocation Location() const
            {
                return Convert(m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().Location());
            }

            void Location(SetProcessorFactory::PwshConfigurationProcessorLocation value)
            {
                m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().Location(Convert(value));
            }

            winrt::hstring CustomLocation() const
            {
                return m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().CustomLocation();
            }

            void CustomLocation(winrt::hstring value)
            {
                m_remoteFactory.as<Processor::IPowerShellConfigurationProcessorFactoryProperties>().CustomLocation(value);
            }

            HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher)
            {
                return WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
            }

        private:
            static SetProcessorFactory::PwshConfigurationProcessorPolicy Convert(Processor::PowerShellConfigurationProcessorPolicy policy)
            {
                // We have used the same values intentionally; if that changes, update this.
                return ToEnum<SetProcessorFactory::PwshConfigurationProcessorPolicy>(ToIntegral(policy));
            }

            static Processor::PowerShellConfigurationProcessorPolicy Convert(SetProcessorFactory::PwshConfigurationProcessorPolicy policy)
            {
                // We have used the same values intentionally; if that changes, update this.
                return ToEnum<Processor::PowerShellConfigurationProcessorPolicy>(ToIntegral(policy));
            }

            static SetProcessorFactory::PwshConfigurationProcessorLocation Convert(Processor::PowerShellConfigurationProcessorLocation location)
            {
                // We have used the same values intentionally; if that changes, update this.
                return ToEnum<SetProcessorFactory::PwshConfigurationProcessorLocation>(ToIntegral(location));
            }

            static Processor::PowerShellConfigurationProcessorLocation Convert(SetProcessorFactory::PwshConfigurationProcessorLocation location)
            {
                // We have used the same values intentionally; if that changes, update this.
                return ToEnum<Processor::PowerShellConfigurationProcessorLocation>(ToIntegral(location));
            }

            IConfigurationSetProcessorFactory m_remoteFactory;
            wil::unique_event m_completionEvent;
            Collections::IVector<winrt::hstring> m_additionalModulePaths{ winrt::single_threaded_vector<winrt::hstring>() };
            std::vector<winrt::hstring> m_internalAdditionalModulePaths;
            Collections::IVector<winrt::hstring> m_remoteAdditionalModulePaths{ winrt::single_threaded_vector<winrt::hstring>() };
        };
    }

    IConfigurationSetProcessorFactory CreateOutOfProcessFactory(bool useRunAs, const std::string& properties, const std::string& restrictions)
    {
        return winrt::make<RemoteFactory>(useRunAs, properties, restrictions);
    }
}

HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, uint64_t memoryHandleIntPtr, uint64_t initEventHandleIntPtr, uint64_t completionMutexHandleIntPtr, uint64_t parentProcessIntPtr) try
{
    using namespace AppInstaller::CLI::ConfigurationRemoting;

    RETURN_HR_IF(E_POINTER, !memoryHandleIntPtr);

    wil::unique_handle memoryHandle{ reinterpret_cast<HANDLE>(memoryHandleIntPtr) };
    wil::unique_mapview_ptr<details::MappedMemoryValue> mappedMemory{ reinterpret_cast<details::MappedMemoryValue*>(MapViewOfFile(memoryHandle.get(), FILE_MAP_WRITE, 0, 0, 0)) };
    RETURN_LAST_ERROR_IF_NULL(mappedMemory);

    mappedMemory->Result = result;
    mappedMemory->FactorySize = 0;

    if (SUCCEEDED(result))
    {
        wil::com_ptr<IStream> stream;
        RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));

        RETURN_IF_FAILED(CoMarshalInterface(stream.get(), winrt::guid_of<IConfigurationSetProcessorFactory>(), reinterpret_cast<::IUnknown*>(factory), MSHCTX_LOCAL, nullptr, MSHLFLAGS_NORMAL));

        ULARGE_INTEGER streamSize{};
        RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_CUR, &streamSize));
        RETURN_HR_IF(E_NOT_SUFFICIENT_BUFFER, streamSize.QuadPart > details::MappedMemoryValue::MaxFactorySize());

        ULONG bufferSize = static_cast<ULONG>(streamSize.QuadPart);

        RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));
        ULONG bytesRead = 0;
        RETURN_IF_FAILED(stream->Read(mappedMemory->FactoryObject, bufferSize, &bytesRead));
        RETURN_HR_IF(E_UNEXPECTED, bytesRead != bufferSize);

        mappedMemory->FactorySize = bufferSize;
    }

    wil::unique_event initEvent{ reinterpret_cast<HANDLE>(initEventHandleIntPtr) };
    initEvent.SetEvent();

    // Wait until the caller releases the object (signalling the event) or the parent process exits
    wil::unique_event completionEvent{ reinterpret_cast<HANDLE>(completionMutexHandleIntPtr) };
    wil::unique_process_handle parentProcess{ reinterpret_cast<HANDLE>(parentProcessIntPtr) };

    HANDLE waitHandles[2];
    waitHandles[0] = completionEvent.get();
    waitHandles[1] = parentProcess.get();

    std::ignore = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);

    return S_OK;
}
CATCH_RETURN();
