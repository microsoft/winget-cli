// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscProcessor.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    namespace
    {
        void WriteInput(HANDLE handle, std::string input)
        {
            THROW_LAST_ERROR_IF(!WriteFile(handle, input.data(), (DWORD)input.length(), nullptr, nullptr));
        }

        std::string ReadOutput(HANDLE handle)
        {
            char buffer[1024];
            std::string output;

            for (;;)
            {
                DWORD bytesRead;
                if (!ReadFile(handle, buffer, sizeof(buffer), &bytesRead, nullptr))
                {
                    // The write handle has been closed.
                    THROW_LAST_ERROR_IF(GetLastError() != ERROR_BROKEN_PIPE);
                    break;
                }

                if (bytesRead == 0)
                {
                    break;
                }

                // This going to change something like each new line, create a resource
                // result and store it for stdOut.
                std::string_view data{ buffer, bytesRead };
                output += data;
            }

            return output;
        }
    }

    DscProcessor::DscProcessor()
    {
    }

    DscProcessorResult DscProcessor::Execute()
    {
        SECURITY_ATTRIBUTES securityAttributes{};
        securityAttributes.nLength = sizeof(securityAttributes);
        securityAttributes.bInheritHandle = TRUE;

        wil::unique_handle stdInReadPipe;
        wil::unique_handle stdInWritePipe;
        THROW_LAST_ERROR_IF(!CreatePipe(&stdInReadPipe, &stdInWritePipe, &securityAttributes, 0));
        THROW_LAST_ERROR_IF(!SetHandleInformation(stdInWritePipe.get(), HANDLE_FLAG_INHERIT, 0));

        wil::unique_handle stdOutReadPipe;
        wil::unique_handle stdOutWritePipe;
        THROW_LAST_ERROR_IF(!CreatePipe(&stdOutReadPipe, &stdOutWritePipe, &securityAttributes, 0));
        THROW_LAST_ERROR_IF(!SetHandleInformation(stdOutReadPipe.get(), HANDLE_FLAG_INHERIT, 0));

        wil::unique_handle stdErrReadPipe;
        wil::unique_handle stdErrWritePipe;
        THROW_LAST_ERROR_IF(!CreatePipe(&stdErrReadPipe, &stdErrWritePipe, &securityAttributes, 0));
        THROW_LAST_ERROR_IF(!SetHandleInformation(stdErrReadPipe.get(), HANDLE_FLAG_INHERIT, 0));

        STARTUPINFO startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.hStdInput = stdInReadPipe.get();
        startupInfo.hStdOutput = stdOutWritePipe.get();
        startupInfo.hStdError = stdErrWritePipe.get();
        startupInfo.dwFlags = STARTF_USESTDHANDLES;

        // Write the input in the stdIn. If used in the cmdline it will fail 
        // with stdIn and -i cannot be used at the same time
        // even when hStdInput is INVALID_HANDLE_VALUE
        std::string stdIn = "{ 'resources': [ { 'name': 'test', 'type': 'xE2ETestResource/E2EFileResource', 'properties': { 'path': 'D:\\Dev\\DscV3\\testv3.txt', 'content': 'test' } } ] }";
        WriteInput(stdInWritePipe.get(), stdIn);

        std::wstring cmdline = L"dsc.exe resource test -r DSC/PowerShellGroup";

        // This requires dsc to be in PATH.
        wil::unique_process_information process;
        THROW_LAST_ERROR_IF(!CreateProcessW(
            NULL,
            cmdline.data(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &startupInfo,
            &process));

        DscProcessorResult result{};
        result.exitCode = 0;

        // Close handle for them to see EOF.
        CloseHandle(stdInWritePipe.get());

        THROW_LAST_ERROR_IF(WaitForSingleObject(process.hProcess, INFINITE) != WAIT_OBJECT_0);
        GetExitCodeProcess(process.hProcess, &result.exitCode);

        // Close handle to see EOF.
        CloseHandle(stdOutWritePipe.get());
        CloseHandle(stdErrWritePipe.get());

        // At some point make move this to a different thread.
        result.stdOut = ReadOutput(stdOutReadPipe.get());
        result.stdErr = ReadOutput(stdErrReadPipe.get());

        return result;
    }
}