// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetProcessorFactoryRemoting.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::Workflow::ConfigurationRemoting
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
        // The executable file name for the remote server process.
        constexpr std::wstring_view s_RemoteServerFileName = L"ConfigurationRemotingServer\\ConfigurationRemotingServer.exe";

        // Represents a remote factory object that was created from a specific process.
        struct RemoteFactory : winrt::implements<RemoteFactory, IConfigurationSetProcessorFactory>
        {
            RemoteFactory()
            {
                AICLI_LOG(Config, Verbose, << "Launching process for configuration processing...");

                // Security attributes to set handles as inherited.
                SECURITY_ATTRIBUTES securityAttributes{};
                securityAttributes.nLength = sizeof(securityAttributes);
                securityAttributes.bInheritHandle = TRUE;
                securityAttributes.lpSecurityDescriptor = nullptr;

                // Create file mapping backed by page file.
                wil::unique_handle memoryHandle{ CreateFileMappingW(INVALID_HANDLE_VALUE, &securityAttributes, PAGE_READWRITE, 0, details::MappedMemoryValue::s_MemorySize, nullptr) };
                THROW_LAST_ERROR_IF_NULL(memoryHandle);

                // Map the memory into the process.
                wil::unique_mapview_ptr<details::MappedMemoryValue> mappedMemory{ reinterpret_cast<details::MappedMemoryValue*>(MapViewOfFile(memoryHandle.get(), FILE_MAP_READ | FILE_MAP_WRITE , 0, 0, 0)) };
                THROW_LAST_ERROR_IF_NULL(mappedMemory);
                // Initialize the result to a failure in case the other process never comes through.
                mappedMemory->Result = E_FAIL;

                // Create an event that the remote process will signal to indicate it has completed creating the object.
                wil::unique_event initEvent;
                initEvent.create(wil::EventOptions::None, nullptr, &securityAttributes);

                // Create the mutex that the remote process will wait on to keep the object alive.
                m_completionMutex.create(nullptr, CREATE_MUTEX_INITIAL_OWNER, MUTEX_ALL_ACCESS, &securityAttributes);

                // Arguments are:
                // server.exe <mapped memory handle> <event handle> <mutex handle>
                std::wostringstream argumentsStream;
                argumentsStream << s_RemoteServerFileName << L' ' << reinterpret_cast<INT_PTR>(memoryHandle.get()) << L' ' << reinterpret_cast<INT_PTR>(initEvent.get()) << L' ' << reinterpret_cast<INT_PTR>(m_completionMutex.get());
                std::wstring arguments = argumentsStream.str();

                std::filesystem::path serverPath = Runtime::GetPathTo(Runtime::PathName::SelfPackageRoot);
                serverPath /= s_RemoteServerFileName;

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
            }

            IConfigurationSetProcessor CreateSetProcessor(const ConfigurationSet& configurationSet)
            {
                return m_remoteFactory.CreateSetProcessor(configurationSet);
            }

            winrt::event_token Diagnostics(const EventHandler<DiagnosticInformation>& handler)
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

        private:
            IConfigurationSetProcessorFactory m_remoteFactory;
            wil::unique_mutex m_completionMutex;
        };
    }

    IConfigurationSetProcessorFactory CreateOutOfProcessFactory()
    {
        return winrt::make<RemoteFactory>();
    }
}

HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, uint64_t memoryHandleIntPtr, uint64_t initEventHandleIntPtr, uint64_t completionMutexHandleIntPtr) try
{
    using namespace AppInstaller::CLI::Workflow::ConfigurationRemoting;

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

    // Wait until the caller releases the object
    wil::unique_mutex completionMutex{ reinterpret_cast<HANDLE>(completionMutexHandleIntPtr) };
    std::ignore = completionMutex.acquire();

    return S_OK;
}
CATCH_RETURN();
