// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Debugging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerDateTime.h"

namespace AppInstaller::Debugging
{
    namespace
    {
        constexpr std::string_view c_minidumpPrefix = "Minidump";
        constexpr std::string_view c_minidumpExtension = ".mdmp";

        struct SelfInitiatedMinidumpHelper
        {
            SelfInitiatedMinidumpHelper() : m_keepFile(false)
            {
                m_filePath = Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation);
                m_filePath /= c_minidumpPrefix.data() + ('-' + Utility::GetCurrentTimeForFilename() + c_minidumpExtension.data());

                m_file.reset(CreateFile(m_filePath.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
                THROW_LAST_ERROR_IF(!m_file);

                SetUnhandledExceptionFilter(UnhandledExceptionCallback);
            }

            ~SelfInitiatedMinidumpHelper()
            {
                if (!m_keepFile)
                {
                    m_file.reset();
                    DeleteFile(m_filePath.wstring().c_str());
                }
            }

            static SelfInitiatedMinidumpHelper& Instance()
            {
                static SelfInitiatedMinidumpHelper instance;
                return instance;
            }

            static LONG WINAPI UnhandledExceptionCallback(EXCEPTION_POINTERS* ExceptionInfo)
            {
                MINIDUMP_EXCEPTION_INFORMATION exceptionInformation{};
                // The unhandled exception filter is executed in the context of the failing thread.
                exceptionInformation.ThreadId = GetCurrentThreadId();
                exceptionInformation.ExceptionPointers = ExceptionInfo;
                exceptionInformation.ClientPointers = FALSE;

                std::thread([&]() {
                    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), Instance().m_file.get(), MiniDumpNormal, &exceptionInformation, nullptr, nullptr);
                    Instance().m_keepFile = true;
                }).join();

                return EXCEPTION_CONTINUE_SEARCH;
            }

            void WriteMinidump()
            {
                std::thread([&]() {
                    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), Instance().m_file.get(), MiniDumpNormal, nullptr, nullptr, nullptr);
                    Instance().m_keepFile = true;
                    }).join();
            }

        private:
            std::filesystem::path m_filePath;
            wil::unique_handle m_file;
            std::atomic_bool m_keepFile;
        };
    }

    void EnableSelfInitiatedMinidump()
    {
        // Force object creation and thus enabling of the crash detection.
        SelfInitiatedMinidumpHelper::Instance();
    }

    void WriteMinidump()
    {
        SelfInitiatedMinidumpHelper::Instance().WriteMinidump();
    }
}
